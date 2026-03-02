# Technical Design Rationale: Spectre  
**Author:** Vikas Narasimha  
**Project:** Unified Computer Architecture, Operating Systems & Embedded Systems Simulator  
**Date:** January 2026  

---

## 1. Problem Statement

Modern computing systems tightly integrate hardware execution, operating systems, and real-time embedded control. However, these domains are often studied independently. **Spectre** was designed as a unified simulator to demonstrate how CPU architecture, OS mechanisms, and embedded scheduling interact within a single cohesive system.

The goal was to build a technically rigorous yet modular platform for experimentation, performance analysis, and systems-level learning.

---

## 2. Architectural Overview

Spectre follows a layered architecture:

1. **CPU Simulation Layer**  
2. **Microkernel (Operating System) Layer**  
3. **Real-Time Embedded (RTOS) Layer**

This separation ensures modular clarity while preserving realistic cross-layer interactions.

---

## 3. Computer Architecture Design

### 3.1 5-Stage Pipeline

A classic 5-stage pipeline (Fetch, Decode, Execute, Memory, Writeback) was implemented to model:

- Data, control, and structural hazards  
- Forwarding and stalls  
- Cycle-accurate instruction flow  

This balances realism with conceptual clarity.

### 3.2 Cache Hierarchy

Configurable L1/L2 set-associative caches were implemented to enable:

- Performance experimentation  
- CPI analysis  
- Evaluation of replacement and write policies  

### 3.3 Tomasulo’s Algorithm

Dynamic scheduling via Tomasulo’s algorithm demonstrates:

- Register renaming  
- Reservation stations  
- Instruction-level parallelism (ILP)  

This elevates the simulator beyond a basic in-order pipeline model.

---

## 4. Operating System Design

### 4.1 Microkernel Architecture

A microkernel design was chosen to modularize:

- Scheduler  
- Memory manager  
- IPC subsystem  
- Virtual filesystem  

This improves extensibility and separation of concerns.

### 4.2 MLFQ Scheduler

A 16-level Multi-Level Feedback Queue (MLFQ) scheduler balances:

- Responsiveness  
- Fairness  
- Starvation avoidance  

### 4.3 Virtual Memory

Paging with TLB simulation enables:

- Address translation modeling  
- Page fault handling  
- Process isolation  

---

## 5. Embedded & Real-Time System Design

### 5.1 RTOS with Rate Monotonic Scheduling

The RTOS implements Rate Monotonic Scheduling (RMS) to provide:

- Deterministic periodic task execution  
- Formal schedulability analysis  

### 5.2 Virtual Hardware Layer

Simulated GPIO, timers, UART, and sensors form a Hardware Abstraction Layer (HAL), mirroring real embedded architectures and enabling safe experimentation.

---

## 6. Performance Instrumentation

Spectre includes built-in metrics for:

- CPI measurement  
- Cache hit/miss rates  
- Branch prediction accuracy  
- Scheduler overhead  
- Power-state modeling  

This transforms the system into a performance analysis platform rather than a static simulator.

---

## 7. Conclusion

Spectre demonstrates strong proficiency in:

- Computer Architecture  
- Operating Systems  
- Embedded & Real-Time Systems  

By integrating hardware simulation, kernel mechanisms, and real-time scheduling into a unified platform, Spectre reflects advanced systems-level design and implementation capability.
