# Advanced Multithreading Guide for Power Users and AI

> **Note**: This document provides detailed technical guidance on ESP32 dual-core programming and thread synchronization. It's intended for experienced developers and AI assistants helping with complex implementations. For basic usage, see the main README.md.

## Threading Architecture and Best Practices

### Understanding the Dual-Thread Design

This library uses ESP32's dual-core architecture to separate networking operations from user interface and time-sensitive tasks. This separation is crucial for maintaining responsive applications when network operations experience delays.

### Why Separate Threads?

MQTT operations, particularly `mqttClient.publish()`, are **blocking calls** that don't return until:
- The message is successfully transmitted and acknowledged, OR
- A network timeout occurs (which can take several seconds)

If your network connection becomes unstable or drops entirely, a single `publish()` call can block your main thread for 5-10 seconds or more. This would make any user interface completely unresponsive and could cause watchdog timer resets.

### Thread Functions Explained

#### connectedLoop() - Main Thread (Core 0)
- Runs on the primary networking thread
- Handles MQTT publishing and sensor data collection
- **Can and will block** during network operations
- Use this for:
  - Reading sensor data
  - Publishing to MQTT topics  
  - Handling periodic tasks
  - Network-dependent operations

#### setup1() - UI Thread Setup (Core 1) 
- Runs once on the second core during startup
- Executes concurrently with main networking setup
- Use for:
  - Display initialization
  - UI component setup
  - Hardware configuration
  - Non-network dependent initialization

#### loop1() - UI Thread Loop (Core 1)
- Runs repeatedly on the second core
- **Remains responsive** even when main thread blocks
- Never blocks for network operations
- Use for:
  - UI updates and display management
  - Button handling and user input
  - Status indicators (LEDs, displays)
  - Time-critical background tasks

## Thread Safety Considerations

When both threads access the same variables or hardware resources, you must implement proper synchronization to prevent race conditions and data corruption.

### When You Need Synchronization

Use mutexes or semaphores when:
- Both threads read/write the same global variables
- Sharing hardware resources (SPI displays, I2C sensors)
- Passing data between threads
- Accessing shared data structures

### Example: Using a Mutex

```cpp
#include "freertos/semphr.h"

// Global variables
SemaphoreHandle_t dataMutex;
float sharedTemperature = 0.0;
bool newDataAvailable = false;

void setup() {
  // Create mutex for shared data
  dataMutex = xSemaphoreCreateMutex();
}

// Main thread: Update sensor data
void connectedLoop() {
  static unsigned long lastReading = 0;
  
  if (millis() - lastReading > 10000) {
    float temp = readTemperatureSensor();
    
    // Safely update shared data
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10))) {
      sharedTemperature = temp;
      newDataAvailable = true;
      xSemaphoreGive(dataMutex);
    }
    
    // This may block for several seconds
    char payload[20];
    snprintf(payload, sizeof(payload), "%.1f", temp);
    mqttClient.publish("sensors/temperature", payload, true);
    
    lastReading = millis();
    feed_watchdog();
  }
}

// UI thread: Display updates (stays responsive)
void loop1() {
  static unsigned long lastDisplay = 0;
  
  if (millis() - lastDisplay > 500) { // Update display every 500ms
    // Safely read shared data
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(1))) {
      if (newDataAvailable) {
        updateDisplay(sharedTemperature);
        newDataAvailable = false;
      }
      xSemaphoreGive(dataMutex);
    }
    lastDisplay = millis();
  }
  
  delay(50); // Small delay to prevent tight loop
}
```

### Recommended Pattern: Shared Data with Change Flags

Instead of using queues, a simpler and more efficient approach is to use a template that pairs variables with change flags, managed by a single application-wide mutex:

```cpp
#include "freertos/semphr.h"

// Global mutex for all shared data
SemaphoreHandle_t sharedDataMutex;

// Template for thread-safe shared variables
template<typename T>
class SharedVariable {
private:
  T value;
  bool changed;

public:
  SharedVariable() : changed(false) {}
  
  // Set value (call from any thread)
  void set(const T& newValue) {
    if (xSemaphoreTake(sharedDataMutex, pdMS_TO_TICKS(10))) {
      value = newValue;
      changed = true;
      xSemaphoreGive(sharedDataMutex);
    }
  }
  
  // Get value and check if changed (call from publishing thread)
  bool getIfChanged(T& output) {
    bool wasChanged = false;
    if (xSemaphoreTake(sharedDataMutex, pdMS_TO_TICKS(10))) {
      if (changed) {
        output = value;
        changed = false;
        wasChanged = true;
      }
      xSemaphoreGive(sharedDataMutex);
    }
    return wasChanged;
  }
  
  // Get current value without changing flag (call from UI thread)
  T get() {
    T result;
    if (xSemaphoreTake(sharedDataMutex, pdMS_TO_TICKS(10))) {
      result = value;
      xSemaphoreGive(sharedDataMutex);
    }
    return result;
  }
};

// Example usage
SharedVariable<float> temperature;
SharedVariable<float> humidity;
SharedVariable<bool> motionDetected;

void setup() {
  sharedDataMutex = xSemaphoreCreateMutex();
}

// UI thread: Update values when they change
void loop1() {
  // Read sensors and update shared variables
  if (digitalRead(MOTION_PIN) != motionDetected.get()) {
    motionDetected.set(digitalRead(MOTION_PIN));
  }
  
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead > 5000) {
    temperature.set(readTemperatureSensor());
    humidity.set(readHumiditySensor());
    lastSensorRead = millis();
  }
  
  delay(100);
}

// Main thread: Publish only changed values
void connectedLoop() {
  float temp, humid;
  bool motion;
  
  // Only publish values that have changed
  if (temperature.getIfChanged(temp)) {
    char payload[20];
    snprintf(payload, sizeof(payload), "%.1f", temp);
    mqttClient.publish("sensors/temperature", payload, true);
  }
  
  if (humidity.getIfChanged(humid)) {
    char payload[20];
    snprintf(payload, sizeof(payload), "%.1f", humid);
    mqttClient.publish("sensors/humidity", payload, true);
  }
  
  if (motionDetected.getIfChanged(motion)) {
    mqttClient.publish("sensors/motion", motion ? "ON" : "OFF", true);
  }
  
  feed_watchdog();
  delay(100);
}
```

This pattern offers several advantages:
- **Efficiency**: Only publishes data that has actually changed
- **Simplicity**: Single mutex protects all shared data
- **Type Safety**: Template ensures compile-time type checking
- **Clean API**: Clear separation between setting, getting, and change detection

## Common Threading Pitfalls

### ❌ Don't Do This
```cpp
// WRONG: Shared variable without synchronization
volatile float globalTemp = 0.0; // Race condition!

void connectedLoop() {
  globalTemp = readSensor(); // Thread 1 writes
}

void loop1() {
  display.print(globalTemp); // Thread 2 reads - potential corruption!
}
```

### ❌ Avoid This
```cpp
// WRONG: Both threads using same I2C/SPI device
void connectedLoop() {
  i2cSensor.read(); // May interfere with display
}

void loop1() {
  i2cDisplay.update(); // May interfere with sensor
}
```

### ✅ Do This Instead
```cpp
// CORRECT: Use I2C/SPI devices from only ONE thread
void connectedLoop() {
  // Handle ALL I2C/SPI operations in main thread
  i2cSensor.read();
  updateDisplayData(); // Prepare data for UI thread
}

void loop1() {
  // UI thread only reads prepared data, no direct I2C/SPI access
  displayPreparedData();
}
```

> **Important**: I2C and SPI buses should only be accessed from one thread. Attempting to share these buses between threads, even with mutexes, can lead to timing issues and bus corruption. Designate one thread (typically the main thread) to handle all bus operations.

## Performance Tips

- **Keep loop1() lightweight**: Avoid long delays or heavy computations
- **Use appropriate timeouts**: Don't wait indefinitely for mutexes in UI thread
- **Consider task priorities**: UI responsiveness vs. data collection priorities
- **Monitor stack usage**: Both threads have limited stack space
- **Feed the watchdog**: Always call `feed_watchdog()` in your main thread loops

## Advanced FreeRTOS Features

### Task Notifications
For simple signaling between threads, task notifications are more efficient than semaphores:

```cpp
TaskHandle_t uiTaskHandle = NULL;

// In setup1()
uiTaskHandle = xTaskGetCurrentTaskHandle();

// Signal from main thread
void connectedLoop() {
  // When new data is ready
  xTaskNotify(uiTaskHandle, 1, eSetBits);
}

// Wait in UI thread
void loop1() {
  uint32_t notification;
  if (xTaskNotifyWait(0, 0xFFFFFFFF, &notification, pdMS_TO_TICKS(100))) {
    // New data notification received
    updateDisplay();
  }
}
```

### Memory Management
Be aware of heap fragmentation with dynamic allocations across threads:

```cpp
// Prefer stack allocation when possible
void loop1() {
  char buffer[100]; // Stack allocation - thread-safe
  // ... use buffer
}

// If dynamic allocation needed, consider thread-local pools
```

### Debugging Multithreaded Applications

```cpp
// Enable task monitoring
void printTaskInfo() {
  char buffer[1000];
  vTaskList(buffer);
  Serial.println(buffer);
}

// Monitor stack high water marks
void checkStackUsage() {
  UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
  Serial.printf("Stack remaining: %d words\n", stackRemaining);
}
```

## Integration with Arduino Libraries

Many Arduino libraries are not thread-safe. When using them across threads:

1. **Check library documentation** for thread safety guarantees
2. **Use external synchronization** (mutexes) when in doubt
3. **Consider library alternatives** designed for RTOS environments
4. **Test thoroughly** under concurrent load

## Troubleshooting Common Issues

### Watchdog Resets
- Ensure `feed_watchdog()` is called regularly in main thread
- Check for deadlocks between threads
- Verify stack sizes are adequate

### Data Corruption
- Review all shared variables for proper synchronization
- Use static analysis tools to detect race conditions
- Add debugging output to track data flow

### Performance Issues
- Profile both threads independently
- Check for priority inversion
- Monitor CPU usage per core

---

*This document covers advanced topics for experienced developers. For basic library usage and simple examples, refer to the main README.md file.*
