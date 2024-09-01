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
HLSL_SHADER_STAGE["vertex"] = "vs_6_6"
HLSL_SHADER_STAGE["fragment"] = "ps_6_6"
HLSL_SHADER_STAGE["compute"] = "cs_6_6"

SHADER_STAGE_REGEX = re.compile(r'#pragma\s+(vertex|fragment|compute)\s+(\w+)')

def parse_entry_points(hlsl_file: str):
    entry_points = {}
    with open(hlsl_file, 'r') as file:
        for line in file:
            match = SHADER_STAGE_REGEX.match(line.strip())
            if match:
                stage = match.group(1)
                entry_point = match.group(2)
                if stage not in entry_points:
                    entry_points[stage] = []
                entry_points[stage].append(entry_point)
    return entry_points

def compile_shader(hlsl_file: str, entry_point: str, shader_stage: str, output_dir: str, force_include: str = None):
    filename = os.path.basename(hlsl_file)
    if force_include:
        with tempfile.NamedTemporaryFile(suffix=".hlsl", delete=False) as temp_file:
            temp_file.write(f'#include "{force_include}"\n'.encode('utf-8'))
            with open(hlsl_file, 'rb') as original_file:
                temp_file.write(original_file.read())
            hlsl_file = temp_file.name
    
    compile_command = [DXC, hlsl_file,
                       "-HV", "2021",
                       "-D", RENDERER_API,
                       "-T", HLSL_SHADER_STAGE[shader_stage],
                       "-I", RUNTIME_INCLUDE_DIRECTORY,
                       "-E", entry_point,
                       "-Fo", f"{output_dir}/{entry_point}.dxil"]
    
    try:
        print(f"Compiling HLSL file {filename} for {shader_stage} stage, entry point: {entry_point}")
        cmd(compile_command, stderr=subprocess.PIPE, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Shader compilation failed for {filename}:\n {e.stderr.decode('utf-8')}")
        raise RuntimeError(f"Shader compilation failed for {filename}")
    finally:
        if force_include:
            os.remove(hlsl_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compile HLSL shaders using DirectXShaderCompiler.")
    parser.add_argument("-d", "--directory", type=str, help="Directory to search for HLSL files.")
    parser.add_argument("-f", "--files", type=str, nargs='+', help="Specific HLSL files to compile.")
    parser.add_argument("-i", "--include", type=str, help="Forced include file (.hlsli) to be used during compilation.")
    args = parser.parse_args()

    output_dir = f"{SCRIPT_DIRECTORY}/../Assets/Shaders"
    if (os.path.exists(output_dir) == False):
        os.mkdir(output_dir)
    
    hlsl_files = []
    if args.directory:
        hlsl_files = glob.glob(os.path.join(args.directory, '**/*.hlsl'), recursive=True)
    elif args.files:
        hlsl_files = args.files
    else:
        print("Error: You must specify either a directory or HLSL files to compile.")
        sys.exit(1)
    
    if not hlsl_files:
        print("No HLSL files found.")
        sys.exit(1)

    for hlsl_file in hlsl_files:
        parsed_entry_points = parse_entry_points(hlsl_file)
        for shader_stage, entry_points in parsed_entry_points.items():
            for entry_point in entry_points:
                compile_shader(hlsl_file, entry_point, shader_stage, output_dir, args.include)
