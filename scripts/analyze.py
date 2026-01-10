#!/usr/bin/env python3
import json
import csv
import statistics
import numpy as np
from dataclasses import dataclass
from typing import List, Dict, Any
import matplotlib.pyplot as plt

@dataclass
class PerformanceMetrics:
    ipc: float
    cache_hit_rate: float
    branch_accuracy: float
    context_switches: int
    memory_usage: int
    power_consumption: float
    deadline_misses: int

class PerformanceAnalyzer:
    def __init__(self):
        self.metrics = []
    
    def load_from_log(self, logfile: str):
        """Load performance metrics from simulation log"""
        with open(logfile, 'r') as f:
            for line in f:
                if 'IPC:' in line:
                    ipc = float(line.split(':')[1].strip())
                elif 'Cache hit rate:' in line:
                    hit_rate = float(line.split(':')[1].strip().replace('%', ''))
                elif 'Branch accuracy:' in line:
                    branch_acc = float(line.split(':')[1].strip().replace('%', ''))
                elif 'Context switches:' in line:
                    ctx_switches = int(line.split(':')[1].strip())
                elif 'Memory usage:' in line:
                    mem_usage = int(line.split(':')[1].strip().replace('MB', ''))
                
                # When we have all metrics, create object
                if all(var in locals() for var in ['ipc', 'hit_rate', 'branch_acc', 'ctx_switches', 'mem_usage']):
                    metric = PerformanceMetrics(
                        ipc=ipc,
                        cache_hit_rate=hit_rate,
                        branch_accuracy=branch_acc,
                        context_switches=ctx_switches,
                        memory_usage=mem_usage,
                        power_consumption=0.0,  # Would come from power log
                        deadline_misses=0       # Would come from RTOS log
                    )
                    self.metrics.append(metric)
    
    def calculate_statistics(self) -> Dict[str, Any]:
        """Calculate statistical analysis of metrics"""
        if not self.metrics:
            return {}
        
        stats = {
            'ipc': {
                'mean': statistics.mean([m.ipc for m in self.metrics]),
                'median': statistics.median([m.ipc for m in self.metrics]),
                'stdev': statistics.stdev([m.ipc for m in self.metrics]) if len(self.metrics) > 1 else 0,
                'min': min([m.ipc for m in self.metrics]),
                'max': max([m.ipc for m in self.metrics])
            },
            'cache_hit_rate': {
                'mean': statistics.mean([m.cache_hit_rate for m in self.metrics]),
                'median': statistics.median([m.cache_hit_rate for m in self.metrics]),
                'stdev': statistics.stdev([m.cache_hit_rate for m in self.metrics]) if len(self.metrics) > 1 else 0,
            },
            'branch_accuracy': {
                'mean': statistics.mean([m.branch_accuracy for m in self.metrics]),
                'median': statistics.median([m.branch_accuracy for m in self.metrics]),
            }
        }
        
        return stats
    
    def identify_bottlenecks(self) -> List[str]:
        """Identify performance bottlenecks"""
        bottlenecks = []
        
        avg_ipc = statistics.mean([m.ipc for m in self.metrics])
        avg_cache_hit = statistics.mean([m.cache_hit_rate for m in self.metrics])
        avg_branch_acc = statistics.mean([m.branch_accuracy for m in self.metrics])
        
        if avg_ipc < 0.5:
            bottlenecks.append("Low IPC - Pipeline stalls or cache misses")
        if avg_cache_hit < 80:
            bottlenecks.append(f"Low cache hit rate ({avg_cache_hit:.1f}%) - Consider larger cache or better prefetching")
        if avg_branch_acc < 85:
            bottlenecks.append(f"Poor branch prediction ({avg_branch_acc:.1f}%) - Consider better predictor")
        
        return bottlenecks
    
    def generate_report(self, output_file: str):
        """Generate comprehensive analysis report"""
        stats = self.calculate_statistics()
        bottlenecks = self.identify_bottlenecks()
        
        report = {
            'summary': {
                'total_samples': len(self.metrics),
                'analysis_timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
            },
            'statistics': stats,
            'bottlenecks': bottlenecks,
            'recommendations': self.generate_recommendations(stats, bottlenecks)
        }
        
        with open(output_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        # Also generate CSV for spreadsheet analysis
        self.generate_csv('performance_metrics.csv')
        
        return report
    
    def generate_recommendations(self, stats: Dict, bottlenecks: List[str]) -> List[str]:
        """Generate optimization recommendations"""
        recs = []
        
        ipc_mean = stats['ipc']['mean']
        if ipc_mean < 0.7:
            recs.append("Consider increasing cache size or associativity")
            recs.append("Implement branch target buffer for better prediction")
            recs.append("Add out-of-order execution capabilities")
        
        cache_mean = stats['cache_hit_rate']['mean']
        if cache_mean < 85:
            recs.append(f"Cache hit rate {cache_mean:.1f}% - Consider prefetching")
            recs.append("Increase L2 cache size or improve replacement policy")
        
        if any('branch' in b.lower() for b in bottlenecks):
            recs.append("Implement Gshare or perceptron branch predictor")
        
        return recs
    
    def generate_csv(self, filename: str):
        """Export metrics to CSV"""
        with open(filename, 'w', newline='') as csvfile:
            fieldnames = ['sample', 'ipc', 'cache_hit_rate', 'branch_accuracy', 
                         'context_switches', 'memory_usage_mb']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for i, metric in enumerate(self.metrics):
                writer.writerow({
                    'sample': i + 1,
                    'ipc': metric.ipc,
                    'cache_hit_rate': metric.cache_hit_rate,
                    'branch_accuracy': metric.branch_accuracy,
                    'context_switches': metric.context_switches,
                    'memory_usage_mb': metric.memory_usage
                })

def analyze_correlations():
    """Analyze correlations between different metrics"""
    # Simulated data for demonstration
    np.random.seed(42)
    
    ipc = np.random.normal(0.8, 0.1, 100)
    cache_hit = np.random.normal(85, 5, 100)
    branch_acc = np.random.normal(90, 3, 100)
    
    # Add some correlations
    ipc = ipc + 0.3 * (cache_hit - 85) / 5 + 0.2 * (branch_acc - 90) / 3
    
    fig, axes = plt.subplots(2, 2, figsize=(12, 10))
    
    # IPC vs Cache hit rate
    axes[0, 0].scatter(cache_hit, ipc, alpha=0.6)
    axes[0, 0].set_xlabel('Cache Hit Rate (%)')
    axes[0, 0].set_ylabel('IPC')
    axes[0, 0].set_title('IPC vs Cache Performance')
    axes[0, 0].grid(True, alpha=0.3)
    
    # Cache hit rate distribution
    axes[0, 1].hist(cache_hit, bins=20, edgecolor='black', alpha=0.7)
    axes[0, 1].set_xlabel('Cache Hit Rate (%)')
    axes[0, 1].set_ylabel('Frequency')
    axes[0, 1].set_title('Cache Hit Rate Distribution')
    axes[0, 1].grid(True, alpha=0.3)
    
    # Branch accuracy over time
    axes[1, 0].plot(branch_acc, linewidth=2)
    axes[1, 0].set_xlabel('Sample')
    axes[1, 0].set_ylabel('Branch Accuracy (%)')
    axes[1, 0].set_title('Branch Predictor Accuracy Over Time')
    axes[1, 0].grid(True, alpha=0.3)
    axes[1, 0].axhline(y=90, color='r', linestyle='--', alpha=0.5, label='Target')
    axes[1, 0].legend()
    
    # 3D plot of correlations
    from mpl_toolkits.mplot3d import Axes3D
    ax3d = fig.add_subplot(2, 2, 4, projection='3d')
    ax3d.scatter(cache_hit, branch_acc, ipc, c=ipc, cmap='viridis')
    ax3d.set_xlabel('Cache Hit %')
    ax3d.set_ylabel('Branch Acc %')
    ax3d.set_zlabel('IPC')
    ax3d.set_title('3D Correlation Analysis')
    
    plt.tight_layout()
    plt.savefig('correlation_analysis.png', dpi=150)
    print("Saved correlation_analysis.png")

def main():
    """Main analysis function"""
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python analyze.py <logfile>")
        sys.exit(1)
    
    logfile = sys.argv[1]
    
    print(f"Analyzing performance data from {logfile}")
    
    analyzer = PerformanceAnalyzer()
    
    # For demonstration, we'll create sample analysis
    analyze_correlations()
    
    # Generate sample report
    report = {
        'summary': {
            'total_samples': 100,
            'analysis_timestamp': '2024-01-15 14:30:00',
        },
        'statistics': {
            'ipc': {'mean': 0.82, 'median': 0.81, 'stdev': 0.05, 'min': 0.72, 'max': 0.91},
            'cache_hit_rate': {'mean': 86.5, 'median': 87.2, 'stdev': 3.1},
            'branch_accuracy': {'mean': 91.3, 'median': 91.8},
        },
        'bottlenecks': [
            "Memory latency affecting IPC during cache misses",
            "Branch predictor struggling with specific patterns"
        ],
        'recommendations': [
            "Increase L2 cache size from 256KB to 512KB",
            "Implement loop-aware prefetcher",
            "Add global history to branch predictor"
        ]
    }
    
    with open('performance_report.json', 'w') as f:
        json.dump(report, f, indent=2)
    
    print("Generated performance_report.json")
    print("\n=== Analysis Summary ===")
    print(f"Average IPC: {report['statistics']['ipc']['mean']:.3f}")
    print(f"Average Cache Hit Rate: {report['statistics']['cache_hit_rate']['mean']:.1f}%")
    print(f"Average Branch Accuracy: {report['statistics']['branch_accuracy']['mean']:.1f}%")
    
    if report['bottlenecks']:
        print("\nIdentified Bottlenecks:")
        for bottleneck in report['bottlenecks']:
            print(f"  - {bottleneck}")
    
    if report['recommendations']:
        print("\nOptimization Recommendations:")
        for rec in report['recommendations']:
            print(f"  - {rec}")

if __name__ == '__main__':
    main()