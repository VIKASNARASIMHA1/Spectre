CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -pthread -lm
INCLUDES = -I./include

TARGET = spectre
TEST_TARGET = spectre_test
DEMO_TARGET = spectre_demo

# All source files
CPU_SOURCES = src/cpu/pipeline.c src/cpu/cache.c src/cpu/branch_predictor.c
KERNEL_SOURCES = src/kernel/kernel.c src/kernel/scheduler.c \
                 src/kernel/memory_manager.c src/kernel/ipc.c src/kernel/vfs.c
EMBEDDED_SOURCES = src/embedded/rtos.c src/embedded/sensors.c src/embedded/timers.c
APP_SOURCES = src/apps/traffic_light.c src/apps/benchmark.c \
              src/apps/sensor_monitor.c src/apps/performance_test.c
MAIN_SOURCE = src/main.c

SOURCES = $(MAIN_SOURCE) $(CPU_SOURCES) $(KERNEL_SOURCES) \
          $(EMBEDDED_SOURCES) $(APP_SOURCES)

OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ -lm

# Test executable
test: $(CPU_SOURCES:.c=.o) $(KERNEL_SOURCES:.c=.o) \
      $(EMBEDDED_SOURCES:.c=.o) src/apps/performance_test.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TEST_TARGET) $^ -lm

# Demo executable
demo: $(CPU_SOURCES:.c=.o) $(EMBEDDED_SOURCES:.c=.o) \
      src/apps/traffic_light.c src/apps/sensor_monitor.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $(DEMO_TARGET) $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(DEMO_TARGET) $(OBJECTS)

run: $(TARGET)
	./$(TARGET)

run_test: test
	./$(TEST_TARGET)

run_demo: demo
	./$(DEMO_TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

format:
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i

docs:
	doxygen Doxyfile

.PHONY: all clean run test demo valgrind format docs