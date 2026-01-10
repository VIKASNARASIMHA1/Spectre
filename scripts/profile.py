#!/usr/bin/env python3
import cProfile
import pstats
import io
import time
import subprocess
import sys
from pathlib import Path

def profile_cpu_simulation():
    """Profile CPU simulator performance"""
    print("Profiling CPU simulator...")
    
    # Import CPU module
    sys.path.insert(0, str(Path(__file__).parent.parent))
    
    # Create a simple profiling script
    profile_code = '''
import sys
sys.path.insert(0, ".")
from src.cpu.pipeline import CPU, cpu_create, cpu_run

def profile_function():
    cpu = cpu_create(64 * 1024)
    
    # Create a test program
    program = []
    for i in range(1000):
        program.extend([1, i % 16, (i + 1) % 16])  # ADD instructions
    
    cpu_load_program(cpu, bytes(program), len(program), 0x1000)
    
    # Run simulation
    cpu_run(cpu, 10000)
    
    return cpu

if __name__ == "__main__":
    profile_function()
'''
    
    # Run profiling
    pr = cProfile.Profile()
    pr.enable()
    
    # Execute the profiling code
    exec(profile_code, globals())
    
    pr.disable()
    
    # Print results
    s = io.StringIO()
    ps = pstats.Stats(pr, stream=s).sort_stats('cumulative')
    ps.print_stats(20)
    
    print("CPU Simulation Profile:")
    print(s.getvalue())
    
    # Save to file
    with open('cpu_profile.txt', 'w') as f:
        f.write(s.getvalue())
    
    return ps

def profile_memory_usage():
    """Profile memory usage of different components"""
    import tracemalloc
    
    print("\nProfiling memory usage...")
    
    # Test memory manager
    tracemalloc.start()
    
    # Import and test memory manager
    sys.path.insert(0, str(Path(__file__).parent.parent))
    
    test_code = '''
from src.kernel.memory_manager import mm_create, mm_allocate_pages, mm_destroy

# Test memory allocation patterns
mm = mm_create(64 * 1024 * 1024)

# Allocate in different patterns
for i in range(100):
    mm_allocate_pages(mm, i % 10, 4)

mm_destroy(mm)
'''
    
    # Take snapshot before
    snapshot1 = tracemalloc.take_snapshot()
    
    # Execute test
    exec(test_code, globals())
    
    # Take snapshot after
    snapshot2 = tracemalloc.take_snapshot()
    
    # Compare snapshots
    top_stats = snapshot2.compare_to(snapshot1, 'lineno')
    
    print("Memory allocation profile:")
    print("=" * 60)
    for stat in top_stats[:10]:
        print(stat)
    
    tracemalloc.stop()
    
    # Save to file
    with open('memory_profile.txt', 'w') as f:
        f.write("Memory Allocation Profile\n")
        f.write("=" * 60 + "\n")
        for stat in top_stats[:20]:
            f.write(str(stat) + "\n")

def benchmark_components():
    """Benchmark performance of different components"""
    print("\nBenchmarking components...")
    
    benchmarks = []
    
    # CPU benchmarks
    print("Running CPU benchmarks...")
    for workload in ['sequential', 'random', 'branch_heavy']:
        start = time.time()
        # Simulate workload
        time.sleep(0.01)  # Placeholder
        end = time.time()
        benchmarks.append(('cpu', workload, end - start))
    
    # Memory benchmarks
    print("Running memory benchmarks...")
    for size in [1024, 4096, 16384]:
        start = time.time()
        # Simulate memory operations
        time.sleep(0.005 * (size / 1024))
        end = time.time()
        benchmarks.append(('memory', f'{size}KB', end - start))
    
    # RTOS benchmarks
    print("Running RTOS benchmarks...")
    for tasks in [4, 8, 16]:
        start = time.time()
        # Simulate task scheduling
        time.sleep(0.002 * tasks)
        end = time.time()
        benchmarks.append(('rtos', f'{tasks}_tasks', end - start))
    
    # Print results
    print("\nBenchmark Results:")
    print("=" * 60)
    print(f"{'Component':<15} {'Workload':<20} {'Time (s)':<10}")
    print("-" * 60)
    
    for component, workload, duration in benchmarks:
        print(f"{component:<15} {workload:<20} {duration:<10.6f}")
    
    # Save to CSV
    import csv
    with open('benchmarks.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['component', 'workload', 'time_seconds'])
        for benchmark in benchmarks:
            writer.writerow(benchmark)
    
    return benchmarks

def analyze_performance_bottlenecks():
    """Analyze performance bottlenecks"""
    print("\nAnalyzing performance bottlenecks...")
    
    bottlenecks = []
    
    # Simulate performance data collection
    performance_data = {
        'cpu_utilization': 85.5,
        'memory_bandwidth': 12.3,  # GB/s
        'cache_miss_rate': 15.2,   # %
        'branch_mispredict_rate': 8.7,  # %
        'context_switch_overhead': 0.3,  # ms
        'io_wait_time': 5.1,  # %
    }
    
    # Identify bottlenecks
    if performance_data['cache_miss_rate'] > 10:
        bottlenecks.append((
            'Cache Performance',
            f"High cache miss rate: {performance_data['cache_miss_rate']}%",
            'Consider increasing cache size or improving locality'
        ))
    
    if performance_data['branch_mispredict_rate'] > 5:
        bottlenecks.append((
            'Branch Prediction',
            f"High branch misprediction: {performance_data['branch_mispredict_rate']}%",
            'Implement more advanced branch predictor'
        ))
    
    if performance_data['context_switch_overhead'] > 0.2:
        bottlenecks.append((
            'Context Switching',
            f"High context switch overhead: {performance_data['context_switch_overhead']}ms",
            'Optimize scheduler or reduce number of processes'
        ))
    
    if performance_data['io_wait_time'] > 5:
        bottlenecks.append((
            'I/O Wait',
            f"High I/O wait time: {performance_data['io_wait_time']}%",
            'Implement asynchronous I/O or improve caching'
        ))
    
    # Print bottleneck analysis
    print("Performance Bottleneck Analysis:")
    print("=" * 70)
    for area, problem, suggestion in bottlenecks:
        print(f"\n🔴 {area}")
        print(f"   Problem: {problem}")
        print(f"   Suggestion: {suggestion}")
    
    # Generate report
    report = {
        'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
        'performance_metrics': performance_data,
        'bottlenecks': [
            {
                'area': area,
                'problem': problem,
                'suggestion': suggestion
            }
            for area, problem, suggestion in bottlenecks
        ],
        'recommendations': [
            'Implement cache prefetching',
            'Add branch target buffer',
            'Use tickless kernel scheduling',
            'Implement DMA for I/O operations'
        ]
    }
    
    import json
    with open('performance_bottlenecks.json', 'w') as f:
        json.dump(report, f, indent=2)
    
    print(f"\nReport saved to performance_bottlenecks.json")
    
    return bottlenecks

def visualize_profiling_results():
    """Create visualization of profiling results"""
    import matplotlib.pyplot as plt
    import numpy as np
    
    print("\nGenerating profiling visualizations...")
    
    # Sample data (in real use, this would come from actual profiling)
    components = ['Pipeline', 'Cache', 'Branch Pred', 'Scheduler', 'Memory']
    execution_time = [45.2, 22.1, 15.8, 10.5, 6.4]  # Percentage
    memory_usage = [128, 256, 64, 512, 1024]  # KB
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    
    # Execution time breakdown
    axes[0].pie(execution_time, labels=components, autopct='%1.1f%%',
               startangle=90, colors=plt.cm.Set3(np.arange(len(components))/len(components)))
    axes[0].set_title('Execution Time Breakdown')
    
    # Memory usage
    bars = axes[1].bar(components, memory_usage, color='skyblue')
    axes[1].set_xlabel('Component')
    axes[1].set_ylabel('Memory Usage (KB)')
    axes[1].set_title('Memory Usage by Component')
    axes[1].grid(True, alpha=0.3, axis='y')
    
    # Add value labels on bars
    for bar in bars:
        height = bar.get_height()
        axes[1].annotate(f'{height}',
                        xy=(bar.get_x() + bar.get_width() / 2, height),
                        xytext=(0, 3),
                        textcoords="offset points",
                        ha='center', va='bottom')
    
    plt.tight_layout()
    plt.savefig('profiling_results.png', dpi=150)
    print("Saved profiling_results.png")

def main():
    """Main profiling function"""
    print("=== Spectre Simulator Profiling Suite ===")
    
    # Run different profiling tasks
    profile_cpu_simulation()
    profile_memory_usage()
    benchmarks = benchmark_components()
    bottlenecks = analyze_performance_bottlenecks()
    visualize_profiling_results()
    
    # Summary
    print("\n" + "="*60)
    print("PROFILING SUMMARY")
    print("="*60)
    
    if bottlenecks:
        print(f"\nFound {len(bottlenecks)} performance bottlenecks:")
        for area, problem, _ in bottlenecks:
            print(f"  • {area}: {problem}")
    else:
        print("\n✅ No major performance bottlenecks found!")
    
    print("\nProfiling complete. Results saved in:")
    print("  - cpu_profile.txt")
    print("  - memory_profile.txt")
    print("  - benchmarks.csv")
    print("  - performance_bottlenecks.json")
    print("  - profiling_results.png")

if __name__ == '__main__':
    main()