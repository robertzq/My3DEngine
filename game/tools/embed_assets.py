import os

# 配置：资源目录 -> 资源 id 前缀
# assets/ 下的文件 id 直接是相对路径；src/shaders/ 下的文件 id 带 "shaders/" 前缀
# （shader 源码属于源码，不放在 assets 里）
SOURCES = [
    ("./assets", ""),
    ("./src/shaders", "shaders/"),
]
OUTPUT_DIR = "./src"
OUTPUT_FILE = "EmbeddedAssets.h"


def iter_files():
    for src_dir, prefix in SOURCES:
        if not os.path.exists(src_dir):
            continue
        for root, dirs, files in os.walk(src_dir):
            for file in files:
                if file.startswith("."):
                    continue
                path = os.path.join(root, file)
                rel = os.path.relpath(path, src_dir).replace("\\", "/")
                yield path, prefix + rel


def main():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)
    out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)

    # 增量检查
    latest_mtime = 0
    entries = list(iter_files())
    for path, _ in entries:
        latest_mtime = max(latest_mtime, os.path.getmtime(path))

    if os.path.exists(out_path) and os.path.getmtime(out_path) >= latest_mtime:
        print("资源未变化，跳过生成 EmbeddedAssets.h")
        return

    print("正在生成 EmbeddedAssets.h ...")

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("#pragma once\n")
        f.write("#include <map>\n")
        f.write("#include <string>\n")
        f.write("#include <vector>\n")
        f.write("#include \"Engine/ResourceManager.h\" // EmbeddedResource 类型定义\n\n")
        f.write("// 这个文件是由 tools/embed_assets.py 自动生成的，请勿手动修改\n\n")

        assets_map = {}

        for path, res_id in entries:
            var_name = "RES_" + res_id.replace(".", "_").replace("/", "_").replace("-", "_").upper()
            print(f"  -> 处理: {res_id}")
            with open(path, "rb") as bf:
                data = bf.read()

            f.write(f"// Source: {res_id}\n")
            f.write(f"inline const unsigned char {var_name}[] = {{")
            for i, b in enumerate(data):
                if i % 16 == 0:
                    f.write("\n    ")
                f.write(f"0x{b:02X}, ")
            f.write("\n};\n\n")

            assets_map[res_id] = (var_name, len(data))

        f.write("inline const std::map<std::string, EmbeddedResource> EMBEDDED_ASSETS = {\n")
        for res_id, (var, size) in assets_map.items():
            f.write(f"    {{ \"{res_id}\", {{ {var}, {size} }} }},\n")
        f.write("};\n")

    print(f"成功! 已生成 {out_path}")


if __name__ == "__main__":
    main()
