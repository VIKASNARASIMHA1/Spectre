# Spectre
A comprehensive software simulator implementing Computer Architecture, Operating Systems, and Embedded Systems concepts

## Overview
Spectre is a complete, portfolio-worthy project that demonstrates deep understanding across three core computer science domains through a unified software simulator:

- 🧠 Computer Architecture: Cycle-accurate x86-64 pipeline simulator with caches and branch prediction

- 🖥️ Operating Systems: Microkernel with process scheduling, virtual memory, and IPC

- 🔧 Embedded Development: Real-time operating system with virtual hardware simulation

##  Features

### Computer Architecture
✅ 5-stage pipeline (Fetch, Decode, Execute, Memory, Writeback)

✅ Cache hierarchy with L1/L2 caches (configurable policies)

✅ Branch predictors (Always Taken, Bimodal, Gshare)

✅ Out-of-order execution (Tomasulo algorithm)

✅ Performance counters (CPI, cache hits, branch accuracy)

✅ Virtual memory with paging and TLBs

### Operating Systems
✅ Microkernel with modular design

✅ MLFQ scheduler with 16 priority levels

✅ Virtual memory manager with demand paging

✅ Inter-process communication via message queues

✅ Virtual filesystem with file operations

✅ System call interface (20+ syscalls)

### Embedded Development
✅ Real-time operating system (RTOS)

✅ Rate Monotonic Analysis for schedulability

✅ Virtual hardware (GPIO, UART, Timers, Sensors)

✅ Power management with sleep states

✅ Hardware abstraction layer (HAL)

✅ Sensor simulation (temperature, humidity, accelerometer)

### Demo Applications
🚦 Traffic Light Controller - Real-time embedded system

📡 Sensor Monitoring - Multi-sensor data collection

📊 Performance Benchmarks - Comparative analysis

🎮 Interactive Simulator - CLI with visualization

## Quick Start

### Prerequisites
```
# Ubuntu/Debian
sudo apt install build-essential python3-matplotlib

# macOS
brew install gcc python3

# Windows (WSL2)
# Install Ubuntu from Microsoft Store
```

### Build & Run
```
# Clone the repository
git clone https://github.com/yourusername/spectre.git
cd spectre

# Build everything
make

# Run the main simulator (interactive menu)
./spectre

# Run all unit tests
make test
./spectre_test

# Run demo applications
make demo
./spectre_demo

# Generate performance visualizations
python3 scripts/visualize.py
```

## Architecture

### System Overview
┌─────────────────────────────────────────────────┐
│               Application Layer                  │
│  Traffic Light | Sensor Monitor | Benchmarks    │
├─────────────────────────────────────────────────┤
│              Simulation Engine                  │
│  CPU Simulator  │  Microkernel  │  RTOS        │
├─────────────────────────────────────────────────┤
│             Virtual Hardware Layer              │
│  Memory  │  Cache  │  Peripherals  │  Sensors  │
└─────────────────────────────────────────────────┘

### Project Structure
spectre/
├── include/              # Header files
│   ├── cpu.h            # CPU simulator interface
│   ├── kernel.h         # Microkernel interface
│   ├── embedded.h       # RTOS interface
│   ├── common.h         # Common utilities
│   └── config.h         # Configuration options
├── src/
│   ├── cpu/             # Computer Architecture
│   │   ├── pipeline.c   # 5-stage pipeline
│   │   ├── cache.c      # Cache hierarchy
│   │   ├── branch_predictor.c
│   │   ├── instruction_set.c
│   │   └── tomasulo.c   # Out-of-order execution
│   ├── kernel/          # Operating Systems
│   │   ├── kernel.c     # Microkernel main
│   │   ├── scheduler.c  # MLFQ scheduler
│   │   ├── memory_manager.c
│   │   ├── ipc.c        # Message queues
│   │   ├── vfs.c        # Virtual filesystem
│   │   └── syscalls.c   # System call interface
│   ├── embedded/        # Embedded Development
│   │   ├── rtos.c       # Real-time OS
│   │   ├── sensors.c    # Virtual sensors
│   │   ├── timers.c     # Hardware timers
│   │   └── power_management.c
│   └── apps/            # Demo applications
│       ├── traffic_light.c
│       ├── sensor_monitor.c
│       ├── benchmark.c
│       └── performance_test.c
├── tests/               # Test suite
│   ├── unit_tests.c
│   └── integration_tests.c
├── scripts/             # Utility scripts
│   ├── visualize.py    # Performance plots
│   ├── analyze.py      # Data analysis
│   ├── build.py        # Build system
│   └── profile.py      # Performance profiling
├── Makefile
└── README.md

## Documentation

### CPU Pipeline Simulation
```
// Create and configure CPU
CPU* cpu = cpu_create(64 * KiB);
cpu->l1_cache = cache_create(CACHE_SET_ASSOC, 32*KiB, 64, 8);
cpu->bp = bp_create(PREDICTOR_GSHARE, 12, 4096);

// Execute program
cpu_load_program(cpu, program, size, 0x1000);
cpu_run(cpu, 1000);

// Analyze performance
cpu_print_stats(cpu);
```

### Microkernel Process Management
```
// Create microkernel
Microkernel* kernel = kernel_create(64 * MiB);

// Create processes
uint32_t pid1 = kernel_create_process(kernel, entry_point1);
uint32_t pid2 = kernel_create_process(kernel, entry_point2);

// Run scheduler
kernel_run(kernel, 10000);

// IPC example
Message msg = {.src_pid=pid1, .dst_pid=pid2, .data="Hello"};
kernel_send_message(kernel, 0, &msg);
```

### RTOS Real-time Tasks
```
// Create RTOS
RTOS* rtos = rtos_create();

// Create periodic task
uint32_t task_id = rtos_create_task(rtos, sensor_task, NULL,
                                   PRIO_HIGH, 100, 5); // 100ms period

// Check schedulability
if (rtos_schedulable(rtos)) {
    rtos_start(rtos);
}

// Virtual hardware access
gpio_write(&rtos->gpio, LED_PIN, 1);
float temp = rtos->sensors[0].temperature;
```

## Demo Applications

### 🚦 Traffic Light Controller
A complete embedded system simulation with:

- 4-phase traffic light control

- Pedestrian crossing support

- Emergency vehicle detection

- Real-time scheduling guarantees

### 📡 Sensor Monitoring System
Multi-sensor data collection with:

- Temperature/humidity/pressure sensors

- Accelerometer for motion detection

- Threshold-based alerts

- Data logging to virtual filesystem

### 📊 Performance Analysis Suite
Comprehensive benchmarking:

- Cache performance across configurations

- Branch predictor accuracy comparison

- Scheduler overhead analysis

- Power consumption profiling

## Testing

### Unit Tests
```
# Run all unit tests
make test
./spectre_test

# Individual component tests
./spectre_test --test-cpu
./spectre_test --test-kernel
./spectre_test --test-rtos
```

### Integration Tests
```
# Test complete system integration
make integration
./integration_test

# Performance regression testing
python3 scripts/analyze.py benchmarks.log
```

### Coverage Report
```
# Generate code coverage report
make coverage
open coverage/index.html
```

## Performance Analysis

### Visualization Tools
```
# Generate performance plots
python3 scripts/visualize.py

# Analyze simulation results
python3 scripts/analyze.py simulation.log

# Profile CPU-intensive operations
python3 scripts/profile.py
```

## Build System

### Custom Build Options
```
# Build with specific configuration
python3 scripts/build.py all --config=release
python3 scripts/build.py all --config=debug
python3 scripts/build.py all --config=profile

# Build individual components
python3 scripts/build.py library
python3 scripts/build.py executable
python3 scripts/build.py tests

# Clean build artifacts
python3 scripts/build.py clean
```

## Learning Resources

### Key Concepts Demonstrated
- Pipeline Hazards - Data, control, structural hazards

- Cache Coherence - Write-back vs write-through policies

- Virtual Memory - Paging, TLBs, page faults

- Process Scheduling - MLFQ, real-time priorities

- IPC Mechanisms - Message passing, shared memory

- Power Management - Sleep states, clock gating

## Acknowledgments
- Hennessy & Patterson - Computer Architecture textbook

- Andrew S. Tanenbaum - Operating Systems concepts

- Jean J. Labrosse - μC/OS RTOS inspiration

- The MIT xv6 team - Educational kernel design
