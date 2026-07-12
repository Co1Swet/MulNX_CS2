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
    
    main_extensions = ('.cpp', '.hpp')                 # 主项目代码扩展名
    third_party_extensions = ('.cpp', '.hpp', '.h', '.c')  # 第三方目录中需要统计的扩展名
    excluded_keyword = ('MulNXThirdParty', 'ThirdParty', 'build', 'MulNXDrop')
    
    total_lines = 0           # 主项目代码总行数
    total_files = 0           # 主项目文件数
    excluded_total_lines = 0  # 第三方代码总行数
    excluded_file_count = 0   # 第三方文件数
    
    processed_files = []      # (相对路径, 行数)
    excluded_files_info = []  # (相对路径, 行数)  第三方文件详情
    error_files = []          # (相对路径, 错误信息)
    
    print(f"当前工作目录: {current_dir}")
    print(f"统计目标目录: {parent_dir}")
    print(f"主项目统计扩展名: {', '.join(main_extensions)}")
    print(f"第三方统计扩展名: {', '.join(third_party_extensions)}")
    print(f"排除关键字: {', '.join(excluded_keyword)}")
    print(f"{'='*60}")
    print("开始扫描目录...\n")
    
    start_time = time.time()
    dir_count = 0
    
    for root, dirs, files in os.walk(parent_dir):
        dir_count += 1
        relative_root = Path(root).relative_to(parent_dir) if Path(root) != parent_dir else Path('.')
        
        # 不再跳过目录，而是让 os.walk 继续递归，我们通过文件路径关键字区分主/第三方
        
        # 收集当前目录下需要处理的文件（主项目或第三方）
        all_files = [f for f in files if f.endswith(main_extensions + third_party_extensions)]
        if all_files:
            # 简单提示当前扫描的目录（可选）
            # 为了减少输出噪音，可以只在有主项目文件时显示，或者统一显示
            pass
        
        for file in all_files:
            file_path = Path(root) / file
            file_path_str = str(file_path)
            
            # 判断是否属于第三方路径（包含排除关键字）
            is_third_party = any(keyword in file_path_str for keyword in excluded_keyword)
            
            # 根据类型确定应使用的扩展名集合
            if is_third_party:
                if not file.endswith(third_party_extensions):
                    continue  # 第三方目录中只统计 .h/.c/.hpp/.cpp
            else:
                if not file.endswith(main_extensions):
                    continue  # 主项目只统计 .cpp/.hpp
            
            # 尝试读取文件
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    line_count = sum(1 for _ in f)
                
                relative_path = file_path.relative_to(parent_dir)
                
                if is_third_party:
                    excluded_total_lines += line_count
                    excluded_file_count += 1
                    excluded_files_info.append((str(relative_path), line_count))
                    print(f"  [第三方] {relative_path}: {line_count} 行")
                else:
                    total_lines += line_count
                    total_files += 1
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
    
    return (total_lines, total_files, parent_dir, processed_files,
            excluded_total_lines, excluded_file_count, excluded_files_info,
            error_files, dir_count, elapsed_time)

def main():
    print("=" * 60)
    print("C++代码行数统计工具 - 详细版（含第三方文件统计）")
    print("=" * 60)
    
    try:
        (total_lines, total_files, parent_dir, processed_files,
         excluded_total_lines, excluded_file_count, excluded_files_info,
         error_files, dir_count, elapsed_time) = count_cpp_lines()
        
        print(f"\n{'='*60}")
        print("统计摘要")
        print(f"{'='*60}")
        print(f"统计目录          : {parent_dir}")
        print(f"扫描目录数量      : {dir_count}")
        print(f"扫描耗时          : {elapsed_time:.2f} 秒")
        print(f"{'─'*40}")
        print(f"【主项目代码】")
        print(f"  文件总数        : {total_files}")
        print(f"  总代码行数      : {total_lines:,}")
        if total_files > 0:
            print(f"  平均每文件行数  : {total_lines / total_files:,.2f}")
        print(f"{'─'*40}")
        print(f"【第三方代码（不纳入主代码总量）】")
        print(f"  文件总数        : {excluded_file_count}")
        print(f"  总代码行数      : {excluded_total_lines:,}")
        if excluded_file_count > 0:
            print(f"  平均每文件行数  : {excluded_total_lines / excluded_file_count:,.2f}")
        print(f"{'─'*40}")
        print(f"读取错误文件数    : {len(error_files)}")
        
        # 主项目文件 Top 10
        if processed_files:
            processed_files_sorted = sorted(processed_files, key=lambda x: x[1], reverse=True)
            print(f"\n{'='*60}")
            print("主项目代码行数最多的 10 个文件:")
            print(f"{'='*60}")
            for i, (file_path, lines) in enumerate(processed_files_sorted[:10]):
                print(f"{i+1:2d}. {file_path:<60} : {lines:>8,} 行")
        
        # 第三方文件 Top 10
        if excluded_files_info:
            excluded_sorted = sorted(excluded_files_info, key=lambda x: x[1], reverse=True)
            print(f"\n{'='*60}")
            print("第三方代码行数最多的 10 个文件:")
            print(f"{'='*60}")
            for i, (file_path, lines) in enumerate(excluded_sorted[:10]):
                print(f"{i+1:2d}. {file_path:<60} : {lines:>8,} 行")
        
        # 显示读取错误的文件（前10个）
        if error_files:
            print(f"\n{'='*60}")
            print(f"读取错误的文件 ({len(error_files)} 个):")
            print(f"{'='*60}")
            for i, (file_path, error_msg) in enumerate(error_files[:10]):
                print(f"{i+1:2d}. {file_path}")
                print(f"    错误: {error_msg}")
            if len(error_files) > 10:
                print(f"... 以及 {len(error_files) - 10} 个其他错误")
        
        # 文件大小分布（仅主项目）
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
            print("主项目文件大小分布:")
            print(f"{'='*60}")
            for group, count in size_groups.items():
                percentage = (count / len(processed_files)) * 100
                print(f"{group:<12}: {count:>4} 个文件 ({percentage:>5.1f}%)")
        
        # 第三方文件大小分布
        if excluded_files_info:
            tp_size_groups = {
                "0-50行": 0,
                "51-200行": 0,
                "201-500行": 0,
                "501-1000行": 0,
                "1001+行": 0
            }
            for _, lines in excluded_files_info:
                if lines <= 50:
                    tp_size_groups["0-50行"] += 1
                elif lines <= 200:
                    tp_size_groups["51-200行"] += 1
                elif lines <= 500:
                    tp_size_groups["201-500行"] += 1
                elif lines <= 1000:
                    tp_size_groups["501-1000行"] += 1
                else:
                    tp_size_groups["1001+行"] += 1
            
            print(f"\n{'='*60}")
            print("第三方文件大小分布:")
            print(f"{'='*60}")
            for group, count in tp_size_groups.items():
                percentage = (count / len(excluded_files_info)) * 100
                print(f"{group:<12}: {count:>4} 个文件 ({percentage:>5.1f}%)")
        
        print(f"\n{'='*60}")
        print("统计完成！")
        print(f"{'='*60}")
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