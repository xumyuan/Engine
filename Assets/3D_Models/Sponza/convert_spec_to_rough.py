"""
Sponza Specular -> Roughness 贴图转换脚本

原理：
  Specular 贴图的亮度可以近似为 Glossiness（光滑度）
  Roughness（粗糙度）= 1.0 - Glossiness

  具体做法：
  1. 读取 spec 贴图
  2. 转为灰度（取亮度）
  3. 反转（1.0 - value）得到 roughness
  4. 保存为同目录下的 *_rough.tga

使用方法：
  pip install Pillow
  python convert_spec_to_rough.py
"""

import os
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("错误: 需要安装 Pillow 库")
    print("请运行: pip install Pillow")
    sys.exit(1)


def convert_spec_to_roughness(spec_path: Path, output_path: Path):
    """将 specular 贴图转换为 roughness 贴图"""
    img = Image.open(spec_path)

    # 转为灰度图 (使用标准亮度公式: 0.299*R + 0.587*G + 0.114*B)
    grayscale = img.convert("L")

    # 反转: roughness = 1.0 - glossiness
    # PIL 的 point() 方法对每个像素应用函数, 像素值范围 [0, 255]
    roughness = grayscale.point(lambda x: 255 - x)

    # 保存为 TGA 格式
    roughness.save(output_path, format="TGA")
    print(f"  [OK] {spec_path.name} -> {output_path.name}")


def main():
    textures_dir = Path(__file__).parent / "textures"

    if not textures_dir.exists():
        print(f"错误: 找不到 textures 目录: {textures_dir}")
        sys.exit(1)

    # 查找所有 *_spec.tga 文件
    spec_files = sorted(textures_dir.glob("*_spec.tga"))

    if not spec_files:
        print("没有找到 *_spec.tga 文件")
        sys.exit(0)

    print(f"找到 {len(spec_files)} 个 specular 贴图，开始转换...\n")

    converted = 0
    skipped = 0

    for spec_file in spec_files:
        # 生成输出文件名: *_spec.tga -> *_rough.tga
        rough_name = spec_file.name.replace("_spec.tga", "_rough.tga")
        rough_path = textures_dir / rough_name

        if rough_path.exists():
            print(f"  [SKIP] {rough_name} already exists, skipped")
            skipped += 1
            continue

        try:
            convert_spec_to_roughness(spec_file, rough_path)
            converted += 1
        except Exception as e:
            print(f"  [FAIL] convert {spec_file.name} failed: {e}")

    print(f"\n转换完成! 成功: {converted}, 跳过: {skipped}, 总计: {len(spec_files)}")

    # 列出没有对应 spec 贴图的材质（这些需要用默认粗糙度值）
    print("\n--- 特殊情况 ---")
    print("以下材质没有 specular 贴图，将使用引擎默认粗糙度值 (0.5):")
    no_spec_materials = [
        "lion (Material__25)",
        "background (Material__298)",
        "chain_texture (chain)",
        "vase_hanging (vase_hanging)",
        "vase_dif (vase)",
    ]
    for m in no_spec_materials:
        print(f"  - {m}")


if __name__ == "__main__":
    main()
