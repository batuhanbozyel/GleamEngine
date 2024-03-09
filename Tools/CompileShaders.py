import os
import sys
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

def compile_shader(hlsl_file: str, entry_point: str, shader_stage: str, output_dir: str):
    compile_command = [DXC, hlsl_file,
                       "-HV", "2021",
                       "-D", RENDERER_API,
                       "-T", HLSL_SHADER_STAGE[shader_stage],
                       "-I", RUNTIME_INCLUDE_DIRECTORY,
                       "-E", entry_point,
                       "-Fo", f"{output_dir}/{entry_point}.dxil"]
    try:
        print(f"Compiling HLSL file {os.path.basename(hlsl_file)} for {shader_stage} stage, entry point: {entry_point}")
        cmd(compile_command, stderr=subprocess.PIPE, check=True)
    except subprocess.CalledProcessError:
        raise RuntimeError(f"Compilation stopped due to error in shader")

if __name__ == "__main__":
    if len(sys.argv) < 5 or (len(sys.argv) - 2) % 3 != 0:
        print("Usage: python CompileShaders.py <hlsl_file> <entry_point> <shader_stage> [...]")
        sys.exit(1)

    output_dir = sys.argv[1]
    if (os.path.exists(output_dir) == False):
        os.mkdir(output_dir)

    for i in range(2, len(sys.argv), 3):
        hlsl_file = sys.argv[i]
        entry_point = sys.argv[i + 1]
        shader_stage = sys.argv[i + 2].lower()
        compile_shader(hlsl_file, entry_point, shader_stage, output_dir)
