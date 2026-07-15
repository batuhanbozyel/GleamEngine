import os
import re
import sys
import glob
import shutil
import argparse
import subprocess

cmd = subprocess.run

SCRIPT_DIRECTORY = os.path.dirname(os.path.realpath(__file__))
OUTPUT_DIRECTORY = os.path.join(SCRIPT_DIRECTORY, "..", "Assets", "Shaders", "Native")
STAGE_QUALIFIERS = ("vertex", "fragment", "kernel", "tile")

# Examples matched:
#   kernel void transformDispatchRaysIndirectArgs(uint id [[thread_position_in_grid]])
#   vertex VertexOut fullscreenTriangle(uint vid [[vertex_id]])
#   fragment metal::float4 compositeShadows(VertexOut in [[stage_in]])
#   [[patch(quad, 4)]] vertex PatchOut tessellate(...)
ENTRY_POINT_REGEX = re.compile(
    r'^[ \t]*'                                        # start of a (possibly indented) line
    r'(?:\[\[[^\]]*\]\][ \t]*)*'                      # optional leading [[attribute]] blocks
    r'\b(?:' + "|".join(STAGE_QUALIFIERS) + r')\b'    # stage qualifier
    r'(?:\s*\[\[[^\]]*\]\])*'                         # zero or more intervening [[attribute]] blocks
    r'\s+'                                            # whitespace / newlines
    r'(?:[\w:]+\s+)*?'                                # optional qualifiers / return type
    r'(\w+)'                                          # function name
    r'\s*\('                                          # opening paren of parameter list
    , re.MULTILINE
)

def parse_entry_points(metal_file: str):
    with open(metal_file, 'r') as f:
        return ENTRY_POINT_REGEX.findall(f.read())

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compile native Metal (.metal) shaders into per-entry-point metallibs.")
    parser.add_argument("-d", "--directory", type=str, help="Directory to search for .metal files.")
    parser.add_argument("-f", "--files", type=str, nargs='+', help="Specific .metal files to compile.")
    parser.add_argument("--debug", action="store_true", help="Embed shader sources for debugging.")
    args = parser.parse_args()

    os.makedirs(OUTPUT_DIRECTORY, exist_ok=True)

    if args.directory:
        metal_files = glob.glob(os.path.join(args.directory, '**/*.metal'), recursive=True)
    elif args.files:
        metal_files = args.files
    else:
        sys.stderr.write("Error: You must specify either a directory or .metal files to compile.\n")
        sys.exit(1)

    if not metal_files:
        sys.stderr.write("No .metal files found.\n")
        sys.exit(1)

    compilation_failed = False

    for metal_file in metal_files:
        filename = os.path.basename(metal_file)
        entry_points = parse_entry_points(metal_file)
        if not entry_points:
            sys.stderr.write(f"Warning: no shader entry points found in {filename}\n")
            continue

        primary_output = os.path.join(OUTPUT_DIRECTORY, entry_points[0] + ".metallib")
        cmdline = ["xcrun", "-sdk", "macosx", "metal", metal_file, "-o", primary_output]
        if args.debug:
            cmdline.append("-frecord-sources")

        try:
            sys.stderr.write(f"Compiling {filename} -> {', '.join(entry_points)}\n")
            cmd(cmdline, stderr=subprocess.PIPE, check=True)

            for entry_point in entry_points[1:]:
                shutil.copyfile(primary_output, os.path.join(OUTPUT_DIRECTORY, entry_point + ".metallib"))
        except subprocess.CalledProcessError as e:
            sys.stderr.write(e.stderr.decode() + "\n")
            compilation_failed = True

    sys.exit(1 if compilation_failed else 0)
