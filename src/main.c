#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "cpu.h"
#include "kernel.h"
#include "embedded.h"

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
    printf("\nShutting down...\n");
}

void print_menu() {
    printf("\n=== Spectre Simulator ===\n");
    printf("1. Run CPU simulator demo\n");
    printf("2. Run microkernel demo\n");
    printf("3. Run embedded RTOS demo\n");
    printf("4. Run traffic light controller\n");
    printf("5. Run all benchmarks\n");
    printf("6. Interactive mode\n");
    printf("0. Exit\n");
    printf("Choice: ");
}

void interactive_mode() {
    printf("\n=== Interactive Mode ===\n");
    printf("Type commands (help for list):\n");
    
    CPU* cpu = cpu_create(64 * KiB);
    Microkernel* kernel = kernel_create(64 * MiB);
    RTOS* rtos = rtos_create();
    
    char command[256];
    while (running) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL) break;
        
        // Remove newline
        command[strcspn(command, "\n")] = 0;
        
        if (strcmp(command, "help") == 0) {
            printf("Commands:\n");
            printf("  cpu stats      - Show CPU statistics\n");
            printf("  cpu step N     - Run N CPU cycles\n");
            printf("  kernel stats   - Show kernel statistics\n");
            printf("  rtos stats     - Show RTOS statistics\n");
            printf("  traffic        - Run traffic light demo\n");
            printf("  exit           - Exit interactive mode\n");
        }
        else if (strncmp(command, "cpu step", 8) == 0) {
            int cycles = atoi(command + 9);
            if (cycles > 0) {
                cpu_run(cpu, cycles);
                printf("Executed %d cycles\n", cycles);
            }
        }
        else if (strcmp(command, "cpu stats") == 0) {
            cpu_print_stats(cpu);
        }
        else if (strcmp(command, "kernel stats") == 0) {
            // kernel_print_stats(kernel);
        }
        else if (strcmp(command, "rtos stats") == 0) {
            rtos_print_stats(rtos);
        }
        else if (strcmp(command, "traffic") == 0) {
            demo_traffic_light();
        }
        else if (strcmp(command, "exit") == 0) {
            break;
        }
        else {
            printf("Unknown command. Type 'help' for list.\n");
        }
    }
    
    cpu_destroy(cpu);
    kernel_destroy(kernel);
    rtos_destroy(rtos);
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);
    
    int choice;
    while (running) {
        print_menu();
        if (scanf("%d", &choice) != 1) break;
        getchar();  // Consume newline
        
        switch (choice) {
            case 0:
                running = 0;
                break;
            case 1: {
                CPU* cpu = cpu_create(64 * KiB);
                if (cpu) {
                    cpu_run(cpu, 1000);
                    cpu_print_stats(cpu);
                    cpu_destroy(cpu);
                }
                break;
            }
            case 2:
                benchmark_scheduler();
                break;
            case 3: {
                RTOS* rtos = rtos_create();
                if (rtos) {
                    // Create some sample tasks
                    for (int i = 0; i < 3; i++) {
                        rtos_create_task(rtos, NULL, NULL, 
                                        PRIO_NORMAL, 1000, 10);
                    }
                    rtos_print_stats(rtos);
                    rtos_destroy(rtos);
                }
                break;
            }
            case 4:
                demo_traffic_light();
                break;
            case 5:
                benchmark_cpu();
                benchmark_cache();
                benchmark_scheduler();
                break;
            case 6:
                interactive_mode();
                break;
            default:
                printf("Invalid choice\n");
        }
    }
    
    printf("Goodbye!\n");
    return 0;
}