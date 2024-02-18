import os
import sys
import platform
import subprocess
cmd = subprocess.run

if platform.system() == "Darwin":
    global DXC
    SCRIPT_DIRECTORY = os.path.dirname(os.path.realpath(__file__))
    DXC = f"{SCRIPT_DIRECTORY}/bin/dxc"
else:
    VULKAN_SDK = os.getenv("VULKAN_SDK")
    if VULKAN_SDK is None:
        DXC = "dxc"
    else:
        DXC = f"{VULKAN_SDK}/bin/dxc"

HLSL_SHADER_STAGE = {}
HLSL_SHADER_STAGE["vertex"] = "vs_6_0"
HLSL_SHADER_STAGE["fragment"] = "ps_6_0"
HLSL_SHADER_STAGE["compute"] = "cs_6_0"
RUNTIME_INCLUDE_DIRECTORY = f"{SCRIPT_DIRECTORY}/../Engine/Source/Runtime/src/Renderer/Shaders"

def compile_shader(hlsl_file: str, entry_point: str, shader_stage: str, output_dir: str):
    output_file = ""
    extra_commands = []
    if platform.system() == "Darwin":
        # Generate DXIL
        output_file = f"{output_dir}/{entry_point}.dxil"
    else:
        # Generate SPIR-V
        output_file = f"{output_dir}/{entry_point}.spv"
        extra_commands = ["-spirv", "-fvk-use-scalar-layout", "-fvk-t-shift", "1000", "0", "-fvk-u-shift", "2000", "0"]
    
    compile_command = [DXC, "-T", HLSL_SHADER_STAGE[shader_stage], "-I", RUNTIME_INCLUDE_DIRECTORY, "-E", entry_point, hlsl_file, "-Fo", output_file]
    compile_command.extend(extra_commands)
    cmd(compile_command)

    try:
        cmd(compile_command, stderr=subprocess.PIPE, check=True)
    except subprocess.CalledProcessError:
        raise RuntimeError(f"Compilation stopped due to error in shader: {entry_point}")

if __name__ == "__main__":
    if len(sys.argv) < 5 or (len(sys.argv) - 2) % 3 != 0:
        print("Usage: python compile_shaders.py <hlsl_file> <entry_point> <shader_stage> [...]")
        sys.exit(1)

    output_dir = sys.argv[1]
    if (os.path.exists(output_dir) == False):
        os.mkdir(output_dir)

    for i in range(2, len(sys.argv), 3):
        hlsl_file = sys.argv[i]
        entry_point = sys.argv[i + 1]
        shader_stage = sys.argv[i + 2].lower()
        compile_shader(hlsl_file, entry_point, shader_stage, output_dir)
