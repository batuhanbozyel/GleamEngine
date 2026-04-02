import os
import re
import sys
import glob
import tempfile
import argparse
import platform
import subprocess
cmd = subprocess.run

SCRIPT_DIRECTORY = os.path.dirname(os.path.realpath(__file__))
RUNTIME_INCLUDE_DIRECTORY = f"{SCRIPT_DIRECTORY}/../Engine/Source/Runtime/src/Renderer/Shaders"

if platform.system() == "Darwin":
    global DXC
    global RENDERER_API
    DXC = f"{SCRIPT_DIRECTORY}/dxc/bin/dxc"
    RENDERER_API = "USE_METAL_RENDERER"
else:
    DXC = f"{SCRIPT_DIRECTORY}/dxc/bin/dxc.exe"
    RENDERER_API = "USE_DIRECTX_RENDERER"

HLSL_SHADER_STAGE = {}
HLSL_SHADER_STAGE["vertex"]          = "vs_6_6"
HLSL_SHADER_STAGE["pixel"]           = "ps_6_6"
HLSL_SHADER_STAGE["compute"]         = "cs_6_6"
# All RT stages compile as lib_6_6; -exports restricts each compilation to a
# single exported entry point so the output .dxil mirrors vs/ps/cs binaries.
HLSL_SHADER_STAGE["raygeneration"]   = "lib_6_6"
HLSL_SHADER_STAGE["miss"]            = "lib_6_6"
HLSL_SHADER_STAGE["closesthit"]      = "lib_6_6"
HLSL_SHADER_STAGE["anyhit"]          = "lib_6_6"
HLSL_SHADER_STAGE["intersection"]    = "lib_6_6"
HLSL_SHADER_STAGE["callable"]        = "lib_6_6"
HLSL_SHADER_STAGE["mesh"]            = "ms_6_6"
HLSL_SHADER_STAGE["amplification"]   = "as_6_6"

SHADER_TARGET_DEFINE = {}
SHADER_TARGET_DEFINE["vertex"]          = "SHADER_TARGET_VERTEX"
SHADER_TARGET_DEFINE["pixel"]           = "SHADER_TARGET_PIXEL"
SHADER_TARGET_DEFINE["compute"]         = "SHADER_TARGET_COMPUTE"
SHADER_TARGET_DEFINE["raygeneration"]   = "SHADER_TARGET_RAYGENERATION"
SHADER_TARGET_DEFINE["miss"]            = "SHADER_TARGET_MISS"
SHADER_TARGET_DEFINE["closesthit"]      = "SHADER_TARGET_CLOSESTHIT"
SHADER_TARGET_DEFINE["anyhit"]          = "SHADER_TARGET_ANYHIT"
SHADER_TARGET_DEFINE["intersection"]    = "SHADER_TARGET_INTERSECTION"
SHADER_TARGET_DEFINE["callable"]        = "SHADER_TARGET_CALLABLE"
SHADER_TARGET_DEFINE["mesh"]            = "SHADER_TARGET_MESH"
SHADER_TARGET_DEFINE["amplification"]   = "SHADER_TARGET_AMPLIFICATION"

RT_STAGES = frozenset({"raygeneration", "miss", "closesthit", "anyhit", "intersection", "callable"})

ALL_STAGES = "|".join(HLSL_SHADER_STAGE.keys())

# Matches a [shader("stage")] attribute followed (on the same or next non-blank
# line) by the function signature, capturing the function name.
#
# Examples matched:
#   [shader("vertex")]
#   void MyVert(...)
#
#   [shader("closesthit")]
#   void MyClosestHit(inout Payload p, BuiltInTriangleIntersectionAttributes a)
#
# The regex works on the full file content (re.MULTILINE | re.DOTALL).
SHADER_ATTR_REGEX = re.compile(
    r'\[shader\("(' + ALL_STAGES + r')"\)\]'   # [shader("stage")]
    r'(?:\s*\[[^\]]*\])*'                       # zero or more intervening [attrib(...)] blocks
    r'\s+'                                      # whitespace / newlines
    r'(?:\w+\s+)*?'                             # optional qualifiers (e.g. export, inline)
    r'(\w+)'                                    # function name
    r'\s*\(',                                   # opening paren of parameter list
    re.MULTILINE
)

def read_include_file(include_file: str, include_dirs: list[str]):
    contents = ""
    for directory in include_dirs:
        include_file_path = os.path.join(directory, os.path.basename(include_file))
        if os.path.exists(include_file_path):
            with open(include_file_path, 'r') as f:
                contents = f.read()
            break
    return contents

def parse_entry_points(hlsl_file: str):
    """
    Returns { stage: [entry_point, ...] } by scanning for [shader("stage")]
    attributes in the source.
    """
    entry_points = {}
    with open(hlsl_file, 'r') as f:
        content = f.read()

    for match in SHADER_ATTR_REGEX.finditer(content):
        stage = match.group(1)
        entry_point = match.group(2)
        if stage not in entry_points:
            entry_points[stage] = []
        entry_points[stage].append(entry_point)

    return entry_points

def rename_entry_point(hlsl_file: str, old_entry_point: str, new_entry_point: str):
    with open(hlsl_file, 'r') as f:
        content = f.read()

    content = content.replace(old_entry_point, new_entry_point)

    with tempfile.NamedTemporaryFile(suffix=".hlsl", delete=False, mode='w') as temp_file:
        temp_file.write(content)
        return temp_file.name

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compile HLSL shaders using DirectXShaderCompiler.")
    parser.add_argument("-d", "--directory", type=str, help="Directory to search for HLSL files.")
    parser.add_argument("-f", "--files", type=str, nargs='+', help="Specific HLSL files to compile.")
    parser.add_argument("-i", "--include", type=str, help="Forced include file (.hlsli) to be used during compilation.")
    parser.add_argument("-o", "--output", type=str, help="Output DXIL filename.")
    parser.add_argument("--entry", type=str, action="append", dest="entries", metavar="entry=export",
                        help="Filter and rename a specific entry point (repeatable). "
                             "Format: entry_name=export_name. "
                             "When omitted, all discovered entry points are compiled.")
    parser.add_argument("--debug", action="store_true", help="Enable debug information.")
    args = parser.parse_args()

    entry_map = {}
    if args.entries:
        for spec in args.entries:
            src, _, dst = spec.partition("=")
            entry_map[src.strip()] = dst.strip()

    output_dir = f"{SCRIPT_DIRECTORY}/../Assets/Shaders"
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

    hlsl_files = []
    if args.directory:
        hlsl_files = glob.glob(os.path.join(args.directory, '**/*.hlsl'), recursive=True)
    elif args.files:
        hlsl_files = args.files
    else:
        sys.stderr.write("Error: You must specify either a directory or HLSL files to compile.\n")
        sys.exit(1)

    if not hlsl_files:
        sys.stderr.write("No HLSL files found.\n")
        sys.exit(1)

    compilation_failed = False

    for hlsl_file in hlsl_files:
        filename = os.path.basename(hlsl_file)
        include_dirs = [os.path.dirname(hlsl_file), RUNTIME_INCLUDE_DIRECTORY]

        if args.include:
            include_dirs.append(os.path.dirname(args.include))
            with tempfile.NamedTemporaryFile(suffix=".hlsl", delete=False, mode="w") as temp_file:
                temp_file.write(read_include_file(args.include, include_dirs))
                with open(hlsl_file, 'r') as original_file:
                    temp_file.write(original_file.read())
                hlsl_file = temp_file.name

        parsed_entry_points = parse_entry_points(hlsl_file)
        for shader_stage, entry_points in parsed_entry_points.items():
            for entry_point in entry_points:
                # When --entry filters are provided, skip entries not listed
                if entry_map and entry_point not in entry_map:
                    continue

                export_name = entry_map[entry_point] if entry_map else None
                try:
                    output_label = export_name if export_name else entry_point
                    sys.stderr.write(f"Compiling HLSL file {filename} for {shader_stage} stage, entry point: {entry_point} -> {output_label}\n")

                    current_hlsl = hlsl_file
                    current_entry = entry_point

                    if export_name:
                        # Use DXC rename directly — no need to rewrite the source file
                        output_file = f"{output_dir}/{export_name}.dxil"
                    elif args.output:
                        current_hlsl = rename_entry_point(current_hlsl, current_entry, args.output)
                        if current_hlsl != hlsl_file:
                            os.remove(hlsl_file)
                            hlsl_file = current_hlsl
                        current_entry = args.output
                        output_file = f"{output_dir}/{current_entry}.dxil"
                    else:
                        output_file = f"{output_dir}/{current_entry}.dxil"

                    compile_command = [DXC, current_hlsl,
                        "-HV", "2021",
                        "-D", RENDERER_API,
                        "-D", SHADER_TARGET_DEFINE[shader_stage],
                        "-T", HLSL_SHADER_STAGE[shader_stage]]

                    if shader_stage in RT_STAGES:
                        # lib_6_6: DXC -exports format is ExportName=InternalName
                        if export_name:
                            compile_command.extend(["-exports", f"{export_name}={entry_point}"])
                        else:
                            compile_command.extend(["-exports", current_entry])
                    else:
                        # vs/ps/cs: traditional single entry point
                        compile_command.extend(["-E", current_entry])

                    compile_command.extend(["-Fo", output_file])

                    if args.debug:
                        compile_command.extend(["-Zi", "-Qembed_debug"])

                    for directory in include_dirs:
                        compile_command.extend(["-I", directory])

                    cmd(compile_command, stderr=subprocess.PIPE, check=True)
                except subprocess.CalledProcessError as e:
                    sys.stderr.write(f"Shader compilation failed for {filename}:\n{e.stderr.decode('utf-8')}\n")
                    compilation_failed = True
                finally:
                    if not export_name and (args.include or args.output):
                        os.remove(hlsl_file)

    sys.exit(1 if compilation_failed else 0)