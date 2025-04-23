# Task-scheduling-Algorithm-Project-2-ECE-7205
This project implements a custom task scheduling algorithm designed to optimize energy consumption while maintaining execution performance in a multicore system environment. It leverages dynamic programming and workload profiling to decide whether tasks should be executed locally or offloaded to the cloud, adapting to varying system constraints and workloads.

# Project Overview
**Objective**: Reduce energy consumption and optimize execution time by intelligently scheduling tasks based on runtime conditions.
**Approach**: Employ dynamic thresholds to make real-time decisions on task offloading versus local execution.
**Implementation Language**: C++
**Platform**: Simulated task graphs using pre-defined data input structures.

# Core Features
**Energy-Aware Scheduling**: Balances local computation and offloading to the cloud for optimal energy usage.
**Dynamic Programming**: Identifies optimal task distributions to maximize performance under constraints.
**Performance Evaluation**: Benchmarked against various workloads with both synthetic and real-world inspired data.
**Scalable Design**: Supports large input sets and adapts to varying processing capacities.

# Files in This Repository
- project-2 last and final submission.cpp — Main source code implementing the scheduling algorithm.
- Project 2 (1).pdf — the outline of the task to be done and executed.
- final_project_1-1.pdf — Final report including results, graphs, and conclusions.

# How to Run
Clone the repository:
```bash
git clone https://github.com/PRAYAG2000n/Task-scheduling-Algorithm.git
cd Task-scheduling-Algorithm
```
# Compile the source code:

```bash
g++ -o scheduler "project-2 last and final submission.cpp"
Run the executable:

```bash
./scheduler
```
**Input**: The program reads task graphs and system parameters (embedded in the code/ provided via standard input).

# Results & Achievements:
* Achieved up to 20% energy reduction without degrading task throughput.
* Validated against multiple task graph scenarios for reliability and scalability.
* Demonstrated applicability to HPC and enterprise systems.
