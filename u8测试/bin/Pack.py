#!/usr/bin/env python3
"""
MulNX 项目打包脚本
====================

功能：
1. 检查 Output 文件夹是否存在，不存在则报错退出。
2. 清空 Output 文件夹中的所有内容（保留 Output 文件夹本身）。
3. 将 bin/MulNX 文件夹完整复制到 Output/MulNX，复制过程中跳过所有 .pdb 文件。
4. 将 Output/MulNX 文件夹压缩为 MulNX.7z（使用 7z 格式，高压缩率）。
5. 打印压缩包内容概览。

依赖：
- Python 3.6+
- py7zr 库（请使用 pip install py7zr 安装）

使用方法：
- 将本脚本放置在项目的 bin 目录下，确保 bin 目录中存在 MulNX 文件夹和 Output 文件夹。
- 直接运行 python bin/Pack.py 即可。
- 脚本会自动清空 Output，重新复制并压缩。
"""

import os
import shutil
import sys
import io
from pathlib import Path
import py7zr  # 第三方库，用于创建 7z 压缩包

# 将标准输出包装为 UTF-8，避免在 Windows 控制台出现编码错误
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')


def clear_output_folder(output_path: Path):
    """
    清空指定文件夹中的所有内容，但保留该文件夹本身。
    
    参数：
        output_path: 需要清空的文件夹路径（Path 对象）
    
    行为：
        - 如果文件夹存在，删除其中所有文件和子文件夹。
        - 如果文件夹不存在，打印提示信息（不报错）。
    """
    if output_path.exists() and output_path.is_dir():
        print(f"正在清空Output文件夹: {output_path}")
        for item in output_path.iterdir():
            if item.is_file() or item.is_symlink():
                item.unlink()          # 删除文件或符号链接
            elif item.is_dir():
                shutil.rmtree(item)    # 递归删除子目录
        print("  ✓ Output文件夹已清空")
    else:
        print(f"  ! Output文件夹不存在或不是目录: {output_path}")


def copy_MulNX_without_pdb(source_dir: Path, output_dir: Path) -> Path:
    """
    将 source_dir 完整复制到 output_dir/MulNX，但跳过所有 .pdb 文件。
    
    参数：
        source_dir: 源文件夹路径（即 bin/MulNX）
        output_dir: 目标根目录路径（即 bin/Output）
    
    返回：
        复制后的 MulNX 文件夹路径（即 output_dir/MulNX）
    
    说明：
        - 使用 os.walk 遍历源目录，保持目录结构。
        - 复制文件时使用 shutil.copy2 保留文件元数据（如时间戳）。
        - 跳过所有扩展名为 .pdb 的文件（大小写不敏感）。
        - 统计并打印复制结果。
    """
    print(f"正在复制MulNX文件夹: {source_dir} -> {output_dir}")

    # 在 Output 下创建目标 MulNX 目录
    MulNX_output_dir = output_dir / "MulNX"
    MulNX_output_dir.mkdir(parents=True, exist_ok=True)

    copied_files = 0    # 复制的文件数量
    skipped_files = 0   # 跳过的 .pdb 文件数量
    copied_dirs = 0     # 创建的目录数量（包括根目录）

    # 遍历源目录
    for root, dirs, files in os.walk(source_dir):
        # 计算当前目录相对于源目录的路径，用于在目标中重建相同的层级
        rel_path = Path(root).relative_to(source_dir)
        target_root = MulNX_output_dir / rel_path
        target_root.mkdir(parents=True, exist_ok=True)
        copied_dirs += 1

        for file in files:
            source_file = Path(root) / file
            target_file = target_root / file

            # 跳过所有 .pdb 文件（忽略大小写）
            if file.lower().endswith('.pdb'):
                skipped_files += 1
                continue

            # 复制文件（保留元数据）
            shutil.copy2(source_file, target_file)
            copied_files += 1

    print(f"  ✓ 复制完成: 创建{copied_dirs}个目录, 复制{copied_files}个文件, 跳过{skipped_files}个.pdb文件")
    return MulNX_output_dir


def compress_MulNX_folder(MulNX_folder: Path, output_7z_path: Path) -> Path:
    """
    将 MulNX_folder 压缩为 7z 格式的压缩包，并保存在 output_7z_path。
    
    参数：
        MulNX_folder: 要压缩的文件夹路径（绝对路径，例如 .../Output/MulNX）
        output_7z_path: 输出 7z 文件路径（例如 .../MulNX.7z）
    
    返回：
        生成的 7z 文件路径
    
    说明：
        - 使用 py7zr 库，默认压缩算法为 LZMA2，高压缩率。
        - 使用 archive.writeall(..., 'MulNX') 将整个 MulNX_folder 添加到归档，
          并在归档内保留一个名为 "MulNX" 的根文件夹。
        - 该方法会自动包含空目录和所有子目录。
        - 压缩完成后打印压缩包大小和包含的文件/空目录数量。
    """
    print(f"正在压缩MulNX文件夹: {MulNX_folder} -> {output_7z_path}")

    # 确保输出目录存在
    output_7z_path.parent.mkdir(parents=True, exist_ok=True)

    # 使用 py7zr 创建 7z 文件
    with py7zr.SevenZipFile(output_7z_path, 'w') as archive:
        # writeall 的第一个参数是要压缩的文件夹路径
        # 第二个参数是归档内的根目录名（这里使用 'MulNX'，即解压后会出现 MulNX 文件夹）
        archive.writeall(MulNX_folder, 'MulNX')

    # 统计压缩包内容信息（用于打印）
    total_files = 0
    empty_dirs = 0
    for root, dirs, files in os.walk(MulNX_folder):
        total_files += len(files)
        # 判断空目录：os.listdir(root) 返回空列表表示该目录为空
        if not os.listdir(root):
            empty_dirs += 1

    size_bytes = output_7z_path.stat().st_size
    size_mb = size_bytes / 1024 / 1024
    print(f"  ✓ 压缩完成: {output_7z_path.name} ({size_mb:.2f} MB)")
    print(f"    包含 {total_files} 个文件, {empty_dirs} 个空目录")
    return output_7z_path


def main():
    """
    主函数：执行打包流程。
    
    步骤：
        0. 定位脚本所在目录（应为 bin 目录），检查必要的文件夹。
        1. 清空 Output 文件夹。
        2. 复制 MulNX 到 Output/MulNX（跳过 .pdb）。
        3. 压缩 Output/MulNX 为 MulNX.7z。
        4. 打印结果和压缩包内容概览。
    
    返回：
        bool：成功返回 True，失败返回 False（供 sys.exit 使用）。
    """
    print("=" * 60)
    print("MulNX项目打包工具")
    print("=" * 60)

    # 获取脚本所在目录（即 bin 目录）
    script_dir = Path(__file__).parent.resolve()
    print(f"脚本目录: {script_dir}")

    # 提示：如果脚本不在 bin 目录下，可能路径会出错，需要用户确认
    if script_dir.name.lower() != "bin":
        print(f"警告: 脚本目录不是'bin'目录 (实际: {script_dir.name})")
        in_ci = os.environ.get('CI') == 'true'  # 检测是否在 CI 环境（如 GitHub Actions）
        if in_ci:
            print("检测到 CI 环境，自动继续")
        else:
            # 非 CI 环境，询问用户是否继续
            response = input("是否继续? (y/N): ")
            if response.lower() != 'y':
                print("操作已取消")
                return False

    # 定义关键路径
    output_path = script_dir / "Output"          # 输出目录
    MulNX_source = script_dir / "MulNX"          # 源文件夹
    MulNX_7z_path = script_dir / "MulNX.7z"      # 最终压缩包路径

    # ---- 步骤 0：检查 Output 文件夹是否存在 ----
    if not output_path.exists():
        print(f"错误: Output文件夹不存在: {output_path}")
        print("请确保脚本位于bin目录中，且存在Output文件夹")
        return False

    # ---- 步骤 1：清空 Output 文件夹 ----
    print("\n步骤1: 清空Output文件夹")
    print("-" * 40)
    clear_output_folder(output_path)

    # ---- 步骤 2：复制 MulNX 到 Output ----
    print("\n步骤2: 复制MulNX文件夹到Output")
    print("-" * 40)
    if not MulNX_source.exists():
        print(f"错误: MulNX文件夹不存在: {MulNX_source}")
        return False

    MulNX_output_dir = copy_MulNX_without_pdb(MulNX_source, output_path)

    # ---- 步骤 3：压缩为 7z ----
    print("\n步骤3: 创建MulNX.7z")
    print("-" * 40)
    if not MulNX_output_dir.exists():
        print(f"错误: MulNX文件夹不存在: {MulNX_output_dir}")
        return False

    compress_result = compress_MulNX_folder(MulNX_output_dir, MulNX_7z_path)

    # ---- 步骤 4：打印结果 ----
    print("\n" + "=" * 60)
    print("打包完成!")
    print(f"输出位置: {compress_result}")

    # 展示压缩包内容结构
    print("\n输出内容结构:")
    print("-" * 40)
    with py7zr.SevenZipFile(compress_result, 'r') as archive:
        all_items = archive.getnames()  # 获取所有条目名称（文件或目录）
        dirs = set()
        files = []
        for item in all_items:
            if item.endswith('/'):       # 以 / 结尾的是目录
                dirs.add(item)
            else:
                files.append(item)

        print("目录:")
        for d in sorted(dirs):
            print(f"  {d}")

        print("\n文件 (前20个):")
        for f in sorted(files)[:20]:
            print(f"  {f}")
        if len(files) > 20:
            print(f"  ... 和 {len(files) - 20} 个其他文件")

    return True


if __name__ == "__main__":
    try:
        success = main()
        if not success:
            sys.exit(1)
    except KeyboardInterrupt:
        print("\n\n操作被用户中断")
        sys.exit(0)
    except Exception as e:
        print(f"\n错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)