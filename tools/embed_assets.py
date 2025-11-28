import os
import sys

# 配置路径
ASSETS_DIR = "./assets"
OUTPUT_DIR = "./src/Core"
OUTPUT_FILE = "EmbeddedAssets.h"

def main():
    if not os.path.exists(ASSETS_DIR):
        print(f"错误: 找不到 {ASSETS_DIR} 目录")
        return

    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)

    print(f"正在扫描 {ASSETS_DIR}...")

    # 开始写入头文件
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("#pragma once\n")
        f.write("#include <map>\n")
        f.write("#include <string>\n")
        f.write("#include <vector>\n\n")
        f.write("// 这个文件是由 tools/embed_assets.py 自动生成的，请勿手动修改\n\n")

        assets_map = {} # 存储文件名 -> 变量名的映射

        # 1. 遍历 assets 目录
        for root, dirs, files in os.walk(ASSETS_DIR):
            for file in files:
                # 忽略隐藏文件
                if file.startswith("."): continue

                file_path = os.path.join(root, file)
                # 计算相对路径作为 ID (例如 "level1.map", "ui/button.png")
                rel_path = os.path.relpath(file_path, ASSETS_DIR).replace("\\", "/")

                # 生成合法的 C++ 变量名 (把 . / - 等符号换成 _)
                var_name = "RES_" + rel_path.replace(".", "_").replace("/", "_").replace("-", "_").upper()

                print(f"  -> 处理: {rel_path}")

                # 读取二进制数据
                with open(file_path, "rb") as bf:
                    data = bf.read()

                # 写入数组定义
                # const unsigned char RES_DIRT_PNG[] = { 0x89, 0x50, ... };
                f.write(f"// Source: {rel_path}\n")
                f.write(f"const unsigned char {var_name}[] = {{")

                # 将字节转为 16 进制字符串
                hex_data = [f"0x{b:02X}" for b in data]
                # 每 16 个字节换一行，防止一行太长
                for i, hex_byte in enumerate(hex_data):
                    if i % 16 == 0:
                        f.write("\n    ")
                    f.write(hex_byte + ", ")

                f.write("\n};\n\n")

                # 记录大小和变量名
                assets_map[rel_path] = (var_name, len(data))

        # 2. 生成查找表
        f.write("struct EmbeddedResource {\n")
        f.write("    const unsigned char* data;\n")
        f.write("    size_t size;\n")
        f.write("};\n\n")

        f.write("inline const std::map<std::string, EmbeddedResource> EMBEDDED_ASSETS = {\n")

        for path, (var, size) in assets_map.items():
            f.write(f"    {{ \"{path}\", {{ {var}, {size} }} }},\n")

        f.write("};\n")

    print(f"成功! 已生成 {out_path}")

if __name__ == "__main__":
    main()