CXX ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Werror -pedantic -Isrc

.PHONY: test compile clean

test: build/test_monitor_logic
	./build/test_monitor_logic

build/test_monitor_logic: tests/test_monitor_logic.cpp src/monitor_logic.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) tests/test_monitor_logic.cpp -o $@

# Compile-check the sketch for an Arduino Uno (needs arduino-cli + arduino:avr core)
compile:
	./scripts/compile_sketch.sh

clean:
	rm -rf build .pio
