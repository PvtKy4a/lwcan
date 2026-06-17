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

## Documentation

📚 **Full documentation is available in [docs/DOCUMENTATION.md](docs/DOCUMENTATION.md)**

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

// Periodic call to the timeout handler.
while (1) {
    lwcan_timeouts_handler();
    sleep_ms(1);
}
```

## API Overview

The library provides the following protocol layers:

- **RAW** - Direct CAN frame access
- **ISOTP** - ISO 15765-2 transport protocol
- **CANIF** - CAN interface abstraction
- **UDS Client** - Unified Diagnostic Services implementation

See the [API Reference](docs/DOCUMENTATION.md#api-reference) in the full documentation for detailed information.

## Configuration

Customize the library behavior by creating `lwcan_options.h` in your project. See [Configuration](docs/DOCUMENTATION.md#configuration) in the documentation for available options.

## Examples

See the [Examples](docs/DOCUMENTATION.md#examples) section in the documentation for implementation examples including:
- RAW CAN Communication
- ISOTP Communication
- UDS Client
- CAN FD with ISOTP

## Platform Integration

Implement platform-specific functions for your hardware. See [Platform Integration](docs/DOCUMENTATION.md#platform-specific-implementation) for details.
