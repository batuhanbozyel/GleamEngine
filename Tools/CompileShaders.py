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

HLSL_SHADER_STAGE = {
    "vertex": "vs_6_6",
    "pixel": "ps_6_6",
    "compute": "cs_6_6",
    "raygeneration": "lib_6_6",
    "miss": "lib_6_6",
    "closesthit": "lib_6_6",
    "anyhit": "lib_6_6",
    "intersection": "lib_6_6",
    "callable": "lib_6_6",
    "mesh": "ms_6_6",
    "amplification": "as_6_6",
}

SHADER_TARGET_DEFINE = {
    "vertex": "SHADER_TARGET_VERTEX",
    "pixel": "SHADER_TARGET_PIXEL",
    "compute": "SHADER_TARGET_COMPUTE",
    "raygeneration": "SHADER_TARGET_RAYGENERATION",
    "miss": "SHADER_TARGET_MISS",
    "closesthit": "SHADER_TARGET_CLOSESTHIT",
    "anyhit": "SHADER_TARGET_ANYHIT",
    "intersection": "SHADER_TARGET_INTERSECTION",
    "callable": "SHADER_TARGET_CALLABLE",
    "mesh": "SHADER_TARGET_MESH",
    "amplification": "SHADER_TARGET_AMPLIFICATION",
}

RT_STAGES = frozenset({
    "raygeneration", "miss", "closesthit",
    "anyhit", "intersection", "callable"
})

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
    r'(?:[\w:]+\s+)*?'                           # optional qualifiers (e.g. export, inline, Gleam::Type)
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
        entry_points.setdefault(stage, []).append(entry_point)

    return entry_points

def rename_entry_point(hlsl_file: str, old_entry_point: str, new_entry_point: str):
    with open(hlsl_file, 'r') as f:
        content = f.read()

    content = content.replace(old_entry_point, new_entry_point)

    temp_file = tempfile.NamedTemporaryFile(suffix=".hlsl", delete=False, mode='w')
    temp_file.write(content)
    temp_file.close()
    return temp_file.name

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compile HLSL shaders using DirectXShaderCompiler.")
    parser.add_argument("-d", "--directory", type=str, help="Directory to search for HLSL files.")
    parser.add_argument("-f", "--files", type=str, nargs='+', help="Specific HLSL files to compile.")
    parser.add_argument("-i", "--include", type=str, help="Forced include file (.hlsli) to be used during compilation.")
    parser.add_argument("--entry", type=str, action="append", dest="entries", metavar="entry=export",
                        help="Filter and rename a specific entry point (repeatable). "
                             "Format: entry_name=export_name. "
                             "When omitted, all discovered entry points are compiled.")
    parser.add_argument("-D", "--defines", type=str, action="append", default=[], metavar="NAME[=VALUE]",
                        help="Preprocessor defines forwarded to dxc (repeatable).")
    parser.add_argument("--debug", action="store_true", help="Enable debug information.")
    args = parser.parse_args()

    entry_map = None
    if args.entries:
        entry_map = {}
        for spec in args.entries:
            src, _, dst = spec.partition("=")
            entry_map[src.strip()] = dst.strip()

    output_dir = f"{SCRIPT_DIRECTORY}/../Assets/Shaders"
    os.makedirs(output_dir, exist_ok=True)

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

        base_file = hlsl_file
        temp_files = []

        if args.include:
            include_dirs.append(os.path.dirname(args.include))
            temp = tempfile.NamedTemporaryFile(suffix=".hlsl", delete=False, mode="w")
            temp.write(read_include_file(args.include, include_dirs))
            with open(hlsl_file, 'r') as original:
                temp.write(original.read())
            temp.close()
            base_file = temp.name
            temp_files.append(base_file)

        parsed_entry_points = parse_entry_points(base_file)
        for shader_stage, entry_points in parsed_entry_points.items():
            for entry_point in entry_points:
                # When --entry filters are provided, skip entries not listed
                if entry_map and entry_point not in entry_map:
                    continue

                export_name = entry_map[entry_point] if entry_map else entry_point
                current_file = base_file

                try:
                    if entry_map and shader_stage not in RT_STAGES:
                        current_file = rename_entry_point(base_file, entry_point, export_name)
                        temp_files.append(current_file)

                    output_file = f"{output_dir}/{export_name}.dxil"

                    sys.stderr.write(
                        f"Compiling {filename} [{shader_stage}] {entry_point} -> {export_name}\n"
                    )

                    cmdline = [
                        DXC, current_file,
                        "-HV", "2021",
                        "-D", RENDERER_API,
                        "-D", SHADER_TARGET_DEFINE[shader_stage],
                        "-T", HLSL_SHADER_STAGE[shader_stage]
                    ]

                    for define in args.defines:
                        cmdline.extend(["-D", define])

                    if shader_stage in RT_STAGES:
                        cmdline.extend(["-exports", f"{export_name}={entry_point}"])
                    else:
                        cmdline.extend(["-E", export_name])

                    cmdline.extend(["-Fo", output_file])

                    if args.debug:
                        cmdline.extend(["-Zi", "-Qembed_debug"])

                    for d in include_dirs:
                        cmdline.extend(["-I", d])

                    cmd(cmdline, stderr=subprocess.PIPE, check=True)

                except subprocess.CalledProcessError as e:
                    sys.stderr.write(e.stderr.decode() + "\n")
                    compilation_failed = True

        for f in temp_files:
            if os.path.exists(f):
                os.remove(f)

    sys.exit(1 if compilation_failed else 0)