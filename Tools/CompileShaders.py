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

def compile_shader(hlsl_file: str, entry_point: str, shader_stage: str, output_dir: str):
    if platform.system() == "Darwin":
        # Generate DXIL
        dxil_file = f"{output_dir}/{entry_point}.dxil"
        cmd([DXC, "-T", HLSL_SHADER_STAGE[shader_stage], "-E", entry_point, hlsl_file, "-Fo", dxil_file])
    else:
        # Generate SPIR-V
        spirv_file = f"{output_dir}/{entry_point}.spv"
        spriv_gen_command = [DXC, "-spirv", "-fvk-use-scalar-layout"]
        spriv_gen_command.extend(["-fvk-t-shift", "1000", "0", "-fvk-u-shift", "2000", "0", "-T", HLSL_SHADER_STAGE[shader_stage], "-E", entry_point, hlsl_file, "-Fo", spirv_file])
        cmd(spriv_gen_command)

if __name__ == "__main__":
    if len(sys.argv) < 5 or (len(sys.argv) - 2) % 3 != 0:
        print("Usage: python compile_shaders.py <hlsl_file> <entry_point> <shader_stage> [...]")
        sys.exit(1)

    output_dir = sys.argv[1]
    for i in range(2, len(sys.argv), 3):
        hlsl_file = sys.argv[i]
        entry_point = sys.argv[i + 1]
        shader_stage = sys.argv[i + 2].lower()
        compile_shader(hlsl_file, entry_point, shader_stage, output_dir)
