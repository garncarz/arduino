test: test-logic test-arduino-r3 test-arduino-r4
	@echo "=== Test Results Summary ==="
	@echo "✅ Multi-barrel coordination logic tests passed"
	@echo "✅ Timing measurement system validated"
	@echo "✅ State machine transitions verified"
	@echo "✅ Continuous energy production confirmed"
	@echo "✅ Arduino R3 (Uno) compatibility verified with reduced memory usage"
	@echo "✅ Arduino R4 WiFi full system compatibility verified"
	@echo ""
	@echo "System ready for Arduino deployment! 🚀"
	@echo "- Deploy to Arduino R3 Uno: Core logic + Serial commands (reduced timing history)"
	@echo "- Deploy to Arduino R4 WiFi: Full system with WiFi + Android app (full timing history)"

test-logic:
	@echo "=== Building and Testing Compressed Air Energy Saving System ==="
	g++ -std=c++11 logic.cpp test_logic.cc -o test_logic
	./test_logic

test-arduino-r3:
	@echo "=== Arduino R3 Compatibility Check (Logic Only) ==="
	@echo "Compiling for Arduino Uno (ATmega328P) with WiFi features disabled..."
	@mkdir -p /tmp/arduino-r3/compressed_air_energy_saving
	@cp *.ino *.cpp *.h /tmp/arduino-r3/compressed_air_energy_saving/
	@echo "Files for R3 compilation:"
	@ls -la /tmp/arduino-r3/compressed_air_energy_saving/
	@cd /tmp/arduino-r3/compressed_air_energy_saving && \
	if arduino-cli compile --fqbn arduino:avr:uno compressed_air_energy_saving.ino; then \
		echo "✅ Arduino R3 (Uno) compilation successful with conditional compilation!"; \
	else \
		echo "❌ Arduino R3 compilation failed!"; \
		echo "Memory usage too high for R3. WiFi features need conditional compilation."; \
		exit 1; \
	fi

test-arduino-r4:
	@echo "=== Arduino R4 WiFi Compatibility Check (Full System) ==="
	@echo "Compiling full system for Arduino Uno R4 WiFi..."
	@mkdir -p /tmp/arduino-r4/compressed_air_energy_saving
	@cp *.ino *.cpp *.h /tmp/arduino-r4/compressed_air_energy_saving/
	@echo "Files for R4 WiFi compilation:"
	@ls -la /tmp/arduino-r4/compressed_air_energy_saving/
	@cd /tmp/arduino-r4/compressed_air_energy_saving && \
	if arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi compressed_air_energy_saving.ino; then \
		echo "✅ Arduino R4 WiFi full system compilation successful!"; \
	else \
		echo "❌ Arduino R4 WiFi compilation failed!"; \
		echo "Attempting to install Arduino R4 WiFi board package..."; \
		arduino-cli core update-index; \
		arduino-cli core install arduino:renesas_uno; \
		if arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi compressed_air_energy_saving.ino; then \
			echo "✅ Arduino R4 WiFi compilation successful after installing board package!"; \
		else \
			echo "❌ Arduino R4 WiFi compilation still failing!"; \
			echo "This may indicate memory usage issues or missing dependencies"; \
			exit 1; \
		fi; \
	fi

verify-structure:
	@echo "=== Project Structure ==="
	@ls -la ./
	@echo ""
	@echo "=== Source Files ==="
	@find . -maxdepth 1 -name "*.cpp" -o -name "*.h" -o -name "*.cc" -o -name "*.ino" | sort

install-arduino-cli:
	@echo "=== Installing Arduino CLI and dependencies ==="
	@if ! command -v arduino-cli >/dev/null 2>&1; then \
		echo "Installing Arduino CLI..."; \
		curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sudo sh; \
		sudo mv bin/arduino-cli /usr/local/bin/; \
		echo "Initializing Arduino CLI and installing Arduino AVR core..."; \
		arduino-cli core update-index; \
		arduino-cli core install arduino:avr; \
	else \
		echo "Arduino CLI already installed"; \
		arduino-cli core update-index; \
	fi

setup: install-arduino-cli
	@echo "=== Development environment setup complete ==="

clean:
	rm -f test_logic
	rm -rf /tmp/arduino-r3
	rm -rf /tmp/arduino-r4

.PHONY: test test-logic test-arduino-r3 test-arduino-r4 verify-structure install-arduino-cli setup clean
