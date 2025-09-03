#!/usr/bin/env python3
"""
Plot timing results from DMR comprehensive timing tests
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import os
import sys

# Set style for better plots
plt.style.use('seaborn-v0_8')
sns.set_palette("husl")

def plot_test1_results(csv_file):
    """Plot results from Test 1: Varying initial processes"""
    if not os.path.exists(csv_file):
        print(f"Warning: {csv_file} not found")
        return
    
    df = pd.read_csv(csv_file)
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle('Test 1: Fixed Expansion/Shrinking with Varying Initial Processes', fontsize=16)
    
    # Plot 1: Expand time vs initial processes
    axes[0,0].plot(df['initial_procs'], df['expand_time'], 'o-', label='Expand Time')
    axes[0,0].set_xlabel('Initial Processes')
    axes[0,0].set_ylabel('Time (seconds)')
    axes[0,0].set_title('Expansion Time vs Initial Processes')
    axes[0,0].grid(True)
    axes[0,0].legend()
    
    # Plot 2: Shrink time vs initial processes  
    axes[0,1].plot(df['initial_procs'], df['shrink_time'], 'o-', label='Shrink Time', color='orange')
    axes[0,1].set_xlabel('Initial Processes')
    axes[0,1].set_ylabel('Time (seconds)')
    axes[0,1].set_title('Shrinking Time vs Initial Processes')
    axes[0,1].grid(True)
    axes[0,1].legend()
    
    # Plot 3: Total time vs initial processes
    axes[1,0].plot(df['initial_procs'], df['total_time'], 'o-', label='Total Time', color='green')
    axes[1,0].set_xlabel('Initial Processes')
    axes[1,0].set_ylabel('Time (seconds)')
    axes[1,0].set_title('Total Time vs Initial Processes')
    axes[1,0].grid(True)
    axes[1,0].legend()
    
    # Plot 4: Overhead percentage vs initial processes
    axes[1,1].plot(df['initial_procs'], df['overhead_percent'], 'o-', label='Overhead %', color='red')
    axes[1,1].set_xlabel('Initial Processes')
    axes[1,1].set_ylabel('Overhead (%)')
    axes[1,1].set_title('Overhead Percentage vs Initial Processes')
    axes[1,1].grid(True)
    axes[1,1].legend()
    
    plt.tight_layout()
    plt.savefig('results/test1_plots.png', dpi=300, bbox_inches='tight')
    plt.show()

def plot_test2_results(csv_file):
    """Plot results from Test 2: Varying expansion/shrinking"""
    if not os.path.exists(csv_file):
        print(f"Warning: {csv_file} not found")
        return
    
    df = pd.read_csv(csv_file)
    
    # Create expansion and shrinking amount columns
    df['expand_amount'] = df['expand_procs'] - df['initial_procs']
    df['shrink_amount'] = df['initial_procs'] - df['shrink_procs']
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle('Test 2: Fixed Initial Processes with Varying Expansion/Shrinking', fontsize=16)
    
    # Plot 1: Expand time vs expansion amount
    expand_data = df.groupby('expand_amount')['expand_time'].mean()
    axes[0,0].bar(expand_data.index, expand_data.values, alpha=0.7)
    axes[0,0].set_xlabel('Expansion Amount (processes)')
    axes[0,0].set_ylabel('Average Expand Time (seconds)')
    axes[0,0].set_title('Expansion Time vs Expansion Amount')
    axes[0,0].grid(True, alpha=0.3)
    
    # Plot 2: Shrink time vs shrinking amount
    shrink_data = df.groupby('shrink_amount')['shrink_time'].mean()
    axes[0,1].bar(shrink_data.index, shrink_data.values, alpha=0.7, color='orange')
    axes[0,1].set_xlabel('Shrinking Amount (processes)')
    axes[0,1].set_ylabel('Average Shrink Time (seconds)')
    axes[0,1].set_title('Shrinking Time vs Shrinking Amount')
    axes[0,1].grid(True, alpha=0.3)
    
    # Plot 3: Heatmap of total time
    pivot_total = df.pivot_table(values='total_time', index='shrink_amount', 
                                columns='expand_amount', aggfunc='mean')
    sns.heatmap(pivot_total, annot=True, fmt='.4f', cmap='viridis', ax=axes[1,0])
    axes[1,0].set_title('Total Time Heatmap (Shrink vs Expand)')
    axes[1,0].set_ylabel('Shrinking Amount')
    axes[1,0].set_xlabel('Expansion Amount')
    
    # Plot 4: Overhead percentage heatmap
    pivot_overhead = df.pivot_table(values='overhead_percent', index='shrink_amount', 
                                   columns='expand_amount', aggfunc='mean')
    sns.heatmap(pivot_overhead, annot=True, fmt='.1f', cmap='RdYlBu_r', ax=axes[1,1])
    axes[1,1].set_title('Overhead % Heatmap (Shrink vs Expand)')
    axes[1,1].set_ylabel('Shrinking Amount')
    axes[1,1].set_xlabel('Expansion Amount')
    
    plt.tight_layout()
    plt.savefig('results/test2_plots.png', dpi=300, bbox_inches='tight')
    plt.show()

def plot_test3_results(csv_file):
    """Plot results from Test 3: Shrink only"""
    if not os.path.exists(csv_file):
        print(f"Warning: {csv_file} not found")
        return
    
    df = pd.read_csv(csv_file)
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle('Test 3: Multiple Shrinking Without Expansion', fontsize=16)
    
    # Plot 1: Shrink time vs target processes
    axes[0,0].plot(df['shrink_to_procs'], df['shrink_time'], 'o-', label='Shrink Time')
    axes[0,0].set_xlabel('Target Processes After Shrinking')
    axes[0,0].set_ylabel('Shrink Time (seconds)')
    axes[0,0].set_title('Shrinking Time vs Target Process Count')
    axes[0,0].grid(True)
    axes[0,0].legend()
    
    # Plot 2: Computation time comparison
    axes[0,1].plot(df['shrink_to_procs'], df['computation_before'], 'o-', 
                  label='Before Shrinking', alpha=0.7)
    axes[0,1].plot(df['shrink_to_procs'], df['computation_after'], 's-', 
                  label='After Shrinking', alpha=0.7)
    axes[0,1].set_xlabel('Target Processes After Shrinking')
    axes[0,1].set_ylabel('Computation Time (seconds)')
    axes[0,1].set_title('Computation Time: Before vs After Shrinking')
    axes[0,1].grid(True)
    axes[0,1].legend()
    
    # Plot 3: Total time vs target processes
    axes[1,0].plot(df['shrink_to_procs'], df['total_time'], 'o-', 
                  label='Total Time', color='green')
    axes[1,0].set_xlabel('Target Processes After Shrinking')
    axes[1,0].set_ylabel('Total Time (seconds)')
    axes[1,0].set_title('Total Time vs Target Process Count')
    axes[1,0].grid(True)
    axes[1,0].legend()
    
    # Plot 4: Efficiency analysis
    df['efficiency_gain'] = (df['computation_before'] - df['computation_after']) / df['computation_before'] * 100
    axes[1,1].bar(df['shrink_to_procs'], df['efficiency_gain'], alpha=0.7, color='purple')
    axes[1,1].set_xlabel('Target Processes After Shrinking')
    axes[1,1].set_ylabel('Computation Efficiency Gain (%)')
    axes[1,1].set_title('Computation Efficiency Gain from Shrinking')
    axes[1,1].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('results/test3_plots.png', dpi=300, bbox_inches='tight')
    plt.show()

def create_summary_plot():
    """Create a summary comparison plot"""
    fig, ax = plt.subplots(1, 1, figsize=(12, 8))
    
    # This will be populated with actual data when CSV files exist
    test_names = ['Varying Initial\nProcesses', 'Varying Expand/\nShrink Amounts', 'Shrink Only\nOperations']
    
    # Placeholder data - will be replaced with actual averages from CSV files
    avg_times = []
    
    # Try to get actual data from CSV files
    csv_files = [
        'results/test1_varying_initial.csv',
        'results/test2_varying_expand_shrink.csv', 
        'results/test3_shrink_only.csv'
    ]
    
    for csv_file in csv_files:
        if os.path.exists(csv_file):
            df = pd.read_csv(csv_file)
            if 'total_time' in df.columns:
                avg_times.append(df['total_time'].mean())
            elif 'shrink_time' in df.columns:  # For test3
                avg_times.append(df['shrink_time'].mean())
            else:
                avg_times.append(0)
        else:
            avg_times.append(0)
    
    if any(t > 0 for t in avg_times):
        bars = ax.bar(test_names, avg_times, alpha=0.7, color=['skyblue', 'lightcoral', 'lightgreen'])
        ax.set_ylabel('Average Time (seconds)')
        ax.set_title('Average Performance Comparison Across All Tests')
        ax.grid(True, alpha=0.3)
        
        # Add value labels on bars
        for bar, time in zip(bars, avg_times):
            if time > 0:
                ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.001,
                       f'{time:.4f}s', ha='center', va='bottom')
    else:
        ax.text(0.5, 0.5, 'No data available yet.\nRun the timing tests first.', 
               ha='center', va='center', transform=ax.transAxes, fontsize=14)
        ax.set_title('Summary Plot - No Data Available')
    
    plt.tight_layout()
    plt.savefig('results/summary_plot.png', dpi=300, bbox_inches='tight')
    plt.show()

def main():
    # Create results directory if it doesn't exist
    os.makedirs('results', exist_ok=True)
    
    print("DMR Timing Results Plotter")
    print("=" * 30)
    
    # Plot results from each test
    print("Plotting Test 1 results...")
    plot_test1_results('results/test1_varying_initial.csv')
    
    print("Plotting Test 2 results...")
    plot_test2_results('results/test2_varying_expand_shrink.csv')
    
    print("Plotting Test 3 results...")
    plot_test3_results('results/test3_shrink_only.csv')
    
    print("Creating summary plot...")
    create_summary_plot()
    
    print("\nAll plots saved to results/ directory:")
    print("- test1_plots.png")
    print("- test2_plots.png") 
    print("- test3_plots.png")
    print("- summary_plot.png")

if __name__ == "__main__":
    main()
