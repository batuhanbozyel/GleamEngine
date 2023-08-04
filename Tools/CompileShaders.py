import os
import sys
import glob
import platform
import subprocess
cmd = subprocess.run

VULKAN_SDK = os.getenv("VULKAN_SDK")
if VULKAN_SDK is None:
    global DXC
    DXC = "dxc"
else:
    DXC = f"{VULKAN_SDK}/bin/dxc"

MSC = "metal-shaderconverter"
METAL_PACK = "xcrun -sdk macosx metal-pack"

HLSL_SHADER_STAGE = {}
HLSL_SHADER_STAGE["vertex"] = "vs_6_0"
HLSL_SHADER_STAGE["fragment"] = "ps_6_0"
HLSL_SHADER_STAGE["compute"] = "cs_6_0"

def compile_shader(hlsl_file: str, entry_point: str, shader_stage: str, output_dir: str):
    if platform.system() == "Darwin":
        dxil_file = f"{output_dir}/{entry_point}.dxil"
        metallib_file = f"{output_dir}/{entry_point}.metallib"

        # Generate DXIL
        cmd([DXC, "-T", HLSL_SHADER_STAGE[shader_stage], "-E", entry_point, hlsl_file, "-Fo", dxil_file])

        # Generate metallib
        cmd([MSC, "--minimum-os-build-version", "13.0.0", dxil_file, "-o", metallib_file])

        # Remove DXIL
        os.remove(dxil_file)
    else:
        spirv_file = f"{output_dir}/{entry_point}.spv"

        # Generate SPIR-V
        if shader_stage == "vertex":
            cmd([DXC, "-spirv", "-fvk-use-gl-layout", "-fvk-invert-y", "-T", HLSL_SHADER_STAGE[shader_stage], "-E", entry_point, hlsl_file, "-Fo", spirv_file])
        else:
            cmd([DXC, "-spirv", "-fvk-use-gl-layout", "-T", HLSL_SHADER_STAGE[shader_stage], "-E", entry_point, hlsl_file, "-Fo", spirv_file])

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

    # Pack metallibs into a single file
    if platform.system() == "Darwin":
        metallib_files: list[str] = []
        for file in os.listdir(output_dir):
            print(file)
            if file != "PrecompiledShaders.metallib" and file.endswith(".metallib"):
                metallib_files.append(f"{output_dir}/{file}")

        packed_metallib = f"{output_dir}/PrecompiledShaders.metallib"
        cmd(METAL_PACK.split() + metallib_files + ["-o", packed_metallib])

        # Remove intermediate metallib files
        for file in metallib_files:
            os.remove(file)
