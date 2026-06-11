#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import sys
from pathlib import Path
import time

def count_cpp_lines():
    # 获取上一级目录
    current_dir = Path.cwd()
    parent_dir = current_dir.parent
    
    extensions = ('.cpp', '.hpp')
    excluded_keyword = ('MulNXThirdParty',"ThirdParty", "build", "MulNXDrop")
    total_lines = 0
    total_files = 0
    processed_files = []
    excluded_files = []
    error_files = []
    
    print(f"当前工作目录: {current_dir}")
    print(f"统计目标目录: {parent_dir}")
    print(f"统计文件类型: {', '.join(extensions)}")
    print(f"排除关键字: '{excluded_keyword}'")
    print(f"{'='*60}")
    print("开始扫描目录...\n")
    
    start_time = time.time()
    dir_count = 0
    
    # 扫描目录
    for root, dirs, files in os.walk(parent_dir):
        dir_count += 1
        relative_root = Path(root).relative_to(parent_dir) if Path(root) != parent_dir else Path('.')
        
        # 跳过包含排除关键字的目录
        if any(keyword in root for keyword in excluded_keyword):
            continue
            
        # 统计当前目录下的cpp/hpp文件
        cpp_files = [f for f in files if f.endswith(extensions)]
        if cpp_files:
            print(f"[扫描目录] {relative_root} ({len(cpp_files)} 个文件)")
        
        for file in cpp_files:
            file_path = Path(root) / file
            file_path_str = str(file_path)
            
            # 检查路径是否包含排除关键字
            if any(keyword in file_path_str for keyword in excluded_keyword):
                excluded_files.append(str(file_path.relative_to(parent_dir)))
                continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    line_count = sum(1 for _ in f)
                
                total_lines += line_count
                total_files += 1
                relative_path = file_path.relative_to(parent_dir)
                processed_files.append((str(relative_path), line_count))
                
                print(f"  √ {relative_path}: {line_count} 行")
                
            except PermissionError:
                error_files.append((str(file_path.relative_to(parent_dir)), "权限不足"))
                print(f"  × {file_path.relative_to(parent_dir)}: 权限不足，跳过")
            except Exception as e:
                error_files.append((str(file_path.relative_to(parent_dir)), str(e)))
                print(f"  × {file_path.relative_to(parent_dir)}: 读取错误，跳过")
    
    end_time = time.time()
    elapsed_time = end_time - start_time
    
    return total_lines, total_files, parent_dir, processed_files, excluded_files, error_files, dir_count, elapsed_time

def main():
    print("=" * 60)
    print("C++代码行数统计工具 - 详细版")
    print("=" * 60)
    
    try:
        total_lines, total_files, parent_dir, processed_files, excluded_files, error_files, dir_count, elapsed_time = count_cpp_lines()
        
        print(f"\n{'='*60}")
        print("统计摘要:")
        print(f"{'='*60}")
        print(f"统计目录: {parent_dir}")
        print(f"扫描目录数量: {dir_count}")
        print(f"扫描耗时: {elapsed_time:.2f} 秒")
        print(f"找到的C++文件总数: {total_files}")
        print(f"排除的文件数: {len(excluded_files)}")
        print(f"读取错误的文件数: {len(error_files)}")
        print(f"成功统计的文件数: {len(processed_files)}")
        print(f"总代码行数: {total_lines:,}")
        
        if total_files > 0:
            avg_lines = total_lines / total_files
            print(f"平均每个文件: {avg_lines:,.2f} 行")
            
            # 找出最大和最小的文件
            if processed_files:
                processed_files_sorted = sorted(processed_files, key=lambda x: x[1], reverse=True)
                
                print(f"\n{'='*60}")
                print("行数最多的10个文件:")
                print(f"{'='*60}")
                for i, (file_path, lines) in enumerate(processed_files_sorted[:10]):
                    print(f"{i+1:2d}. {file_path:<60} : {lines:>8,} 行")
                
                print(f"\n{'='*60}")
                print("行数最少的10个文件:")
                print(f"{'='*60}")
                for i, (file_path, lines) in enumerate(processed_files_sorted[-10:]):
                    print(f"{i+1:2d}. {file_path:<60} : {lines:>8,} 行")
        
        # 显示排除的文件
        if excluded_files:
            print(f"\n{'='*60}")
            print(f"排除的文件 ({len(excluded_files)} 个):")
            print(f"{'='*60}")
            for i, file_path in enumerate(excluded_files[:20]):  # 最多显示20个
                print(f"{i+1:2d}. {file_path}")
            if len(excluded_files) > 20:
                print(f"... 以及 {len(excluded_files) - 20} 个其他文件")
        
        # 显示读取错误的文件
        if error_files:
            print(f"\n{'='*60}")
            print(f"读取错误的文件 ({len(error_files)} 个):")
            print(f"{'='*60}")
            for i, (file_path, error_msg) in enumerate(error_files[:10]):  # 最多显示10个
                print(f"{i+1:2d}. {file_path}")
                print(f"    错误: {error_msg}")
            if len(error_files) > 10:
                print(f"... 以及 {len(error_files) - 10} 个其他错误")
        
        # 文件大小分布
        if processed_files:
            size_groups = {
                "0-50行": 0,
                "51-200行": 0,
                "201-500行": 0,
                "501-1000行": 0,
                "1001+行": 0
            }
            
            for _, lines in processed_files:
                if lines <= 50:
                    size_groups["0-50行"] += 1
                elif lines <= 200:
                    size_groups["51-200行"] += 1
                elif lines <= 500:
                    size_groups["201-500行"] += 1
                elif lines <= 1000:
                    size_groups["501-1000行"] += 1
                else:
                    size_groups["1001+行"] += 1
            
            print(f"\n{'='*60}")
            print("文件大小分布:")
            print(f"{'='*60}")
            for group, count in size_groups.items():
                percentage = (count / len(processed_files)) * 100
                print(f"{group:<12}: {count:>4} 个文件 ({percentage:>5.1f}%)")
        
        print(f"\n{'='*60}")
        print("统计完成！")
        print(f"{'='*60}")
        
        # 等待用户手动关闭
        input("\n按 Enter 键退出...")
        
    except KeyboardInterrupt:
        print("\n\n用户中断操作")
    except Exception as e:
        print(f"\n发生错误: {e}")
        import traceback
        traceback.print_exc()
        input("\n按 Enter 键退出...")

if __name__ == "__main__":
    main()