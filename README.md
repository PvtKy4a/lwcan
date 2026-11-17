# lwcan - Lightweight CAN Library

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

lwcan is a lightweight, portable C library for Controller Area Network (CAN) communication. It provides a flexible framework for implementing CAN protocols including classical CAN, CAN FD, and transport protocols like ISOTP (ISO 15765-2).

## Features

- Small memory footprint - designed for embedded systems
- Platform-independent architecture
- Support for multiple CAN interfaces
- Multiple protocol layers (RAW, ISOTP)
- UDS (Unified Diagnostic Services) client implementation
- Memory pool-based allocation
- Timeout management system
- CAN FD support (optional)

## Quick Start

### Integration

Add lwcan to your CMakeLists.txt:

```cmake
add_subdirectory(lwcan)

target_link_libraries(your_target PRIVATE lwcan)
# For UDS client support:
target_link_libraries(your_target PRIVATE lwcanapps)
```

### Basic Usage

```c
#include "lwcan/init.h"
#include "lwcan/canif.h"

// Initialize the library
lwcan_init();

// Add a CAN interface
struct canif my_canif;
canif_add(&my_canif, "can0", my_init_function);

// By default, lwcan runs a background thread that handles timeouts automatically.
// See docs/DOCUMENTATION.md#LWCAN_NO_SYSTEM-Threaded-vs-Polling-Mode for details.
```

## API Layers

The library provides the following protocol layers:

| Layer | Description |
|-------|-------------|
| **CANIF** | CAN interface abstraction layer |
| **RAW** | Direct CAN frame access |
| **ISOTP** | ISO 15765-2 transport protocol |
| **UDS Client** | Unified Diagnostic Services implementation |

## Documentation

- 📘 **[Full Documentation](docs/DOCUMENTATION.md)** — Complete API reference, examples, configuration options, and platform integration guide.

## Configuration

Customize the library behavior by creating `lwcan_options.h` in your project. See [Configuration](docs/DOCUMENTATION.md#configuration) in the full documentation for available options.

## Examples

See the [Examples](docs/DOCUMENTATION.md#examples) section in the full documentation for implementation examples:

- RAW CAN Communication
- ISOTP Communication
- UDS Client
- CAN FD with ISOTP

## Platform Integration

Implement platform-specific functions for your hardware. See [Platform Integration](docs/DOCUMENTATION.md#platform-specific-implementation) for details.
