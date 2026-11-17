# lwcan Library Documentation

> 📘 This is the detailed API reference and technical guide. For a quick overview and getting started, see [README.md](../README.md).

## Overview

lwcan is a lightweight, portable C library for Controller Area Network (CAN) communication. It provides a flexible framework for implementing CAN protocols including classical CAN, CAN FD, and transport protocols like ISOTP (ISO 15765-2).

The library is designed for embedded systems with limited resources and features:

- Small memory footprint
- Platform-independent architecture
- Support for multiple CAN interfaces
- Multiple protocol layers (RAW, ISOTP)
- UDS (Unified Diagnostic Services) client implementation
- Memory pool-based allocation
- Timeout management system

## Table of Contents

1. [Architecture](#architecture)
2. [Core Concepts](#core-concepts)
3. [Getting Started](#getting-started)
4. [API Reference](#api-reference)
5. [Configuration](#configuration)
6. [Protocol Implementations](#protocol-implementations)
7. [Examples](#examples)

---

## Architecture

### Layered Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  (UDS Client, Custom Protocols, User Code)                  │
├─────────────────────────────────────────────────────────────┤
│              Transport Protocol Layer (ISOTP)               │
│  (ISO 15765-2 - CAN based transport protocol)               │
├─────────────────────────────────────────────────────────────┤
│                   RAW Layer                                 │
│  (Direct CAN frame access)                                  │
├─────────────────────────────────────────────────────────────┤
│                   CAN Interface Layer (CANIF)               │
│  (Abstraction for CAN hardware/driver)                      │
├─────────────────────────────────────────────────────────────┤
│                   Hardware Driver                           │
│  (Platform-specific CAN controller)                         │
└─────────────────────────────────────────────────────────────┘
```

### Key Components

| Component | Description |
|-----------|-------------|
| **CANIF** | CAN interface abstraction layer that manages CAN controllers |
| **RAW** | Raw CAN frame access layer for direct communication |
| **ISOTP** | ISOTP (ISO 15765-2) transport protocol implementation |
| **TIMEOUTS** | System for managing timed events and callbacks |
| **MEMORY** | Memory management with pool-based allocation |
| **BUFFERS** | Dynamic buffer management for message payload |

---

## Core Concepts

### CAN Addresses

CAN communication uses a structured address format defined in `lwcan/can.h`:

```c
struct addr_can {
    uint8_t can_ifindex;     // CAN interface index
    union {
        struct {
            canid_t tx_id;   // Transmission CAN ID
            canid_t rx_id;   // Reception CAN ID
        } tp;                 // Transport protocol address
    } can_addr;
};
```

### CAN Identifiers

CAN identifiers follow standard definitions:

- `CAN_SFF_MASK` - Standard frame format (11-bit ID)
- `CAN_EFF_MASK` - Extended frame format (29-bit ID)
- `CAN_EFF_FLAG` - Extended frame flag
- `CAN_RTR_FLAG` - Remote transmission request
- `CAN_ERR_FLAG` - Error message frame

### Buffer Management

Messages are handled through buffer structures:

```c
struct lwcan_buffer {
    struct lwcan_buffer *next;  // Next buffer in chain
    uint8_t *payload;           // Pointer to data
    uint32_t length;            // Data length
};
```

Buffers are allocated dynamically and support chaining for segmented messages.

#### Buffer API

```c
struct lwcan_buffer *lwcan_buffer_new(uint32_t length);
```
Creates a new buffer with the specified length.

```c
void lwcan_buffer_delete(struct lwcan_buffer *buffer);
```
Deletes a buffer and frees its memory.

```c
lwcanerr_t lwcan_buffer_copy_to(struct lwcan_buffer *buffer, const uint8_t *source, uint32_t length);
```
Copies data into the buffer from the beginning.

```c
lwcanerr_t lwcan_buffer_copy_to_offset(struct lwcan_buffer *buffer, const uint8_t *source, uint32_t length, uint32_t offset);
```
Copies data into the buffer at the specified offset.

```c
lwcanerr_t lwcan_buffer_copy_from(struct lwcan_buffer *buffer, uint8_t *destination, uint32_t length);
```
Copies data from the buffer to the destination.

```c
lwcanerr_t lwcan_buffer_copy_from_offset(struct lwcan_buffer *buffer, uint8_t *destination, uint32_t length, uint32_t offset);
```
Copies data from the buffer at the specified offset to the destination.

---

## Getting Started

### Integration in Your Project

Add lwcan to your CMakeLists.txt:

```cmake
add_subdirectory(lwcan)

# Set required variables for lwcan
set(LWCAN_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lwcan/include)
set(LWCAN_COMPILER_FLAGS "-Wall -Wextra")
set(LWCAN_DEFINITIONS "")

target_link_libraries(your_target PRIVATE lwcan)
# For UDS client support:
target_link_libraries(your_target PRIVATE lwcanapps)
```

**Required Variables:**

| Variable | Description |
|----------|-------------|
| `LWCAN_INCLUDE_DIRS` | Include paths for lwcan headers |
| `LWCAN_COMPILER_FLAGS` | Compiler flags for building lwcan |
| `LWCAN_DEFINITIONS` | Preprocessor definitions |

These variables must be set before calling `target_link_libraries()` for lwcan.

### Basic Initialization

```c
#include "lwcan/init.h"
#include "lwcan/canif.h"
#include "lwcan/can.h"

// Initialize lwcan
lwcan_init();

// Add a CAN interface (platform-specific)
struct canif my_canif;
lwcanerr_t ret = canif_add(&my_canif, "can0", my_init_function);
```

---

## API Reference

### Error Codes

All functions return `lwcanerr_t` (int8_t) error codes:

| Error Code | Value | Description |
|------------|-------|-------------|
| `ERROR_OK` | 0 | Success |
| `ERROR_ARG` | -1 | Invalid argument |
| `ERROR_CANIF` | -2 | CAN interface error |
| `ERROR_CANIF_MAX` | -3 | Maximum CAN interfaces reached |
| `ERROR_IF` | -4 | Low-level interface error |
| `ERROR_MEMORY` | -5 | Out of memory |
| `ERROR_TRANSMIT_TIMEOUT` | -6 | Transmission timeout |
| `ERROR_RECEIVE_TIMEOUT` | -7 | Receive timeout |
| `ERROR_INPROGRESS` | -8 | Operation in progress |
| `ERROR_ABORTED` | -9 | Operation aborted |
| `ERROR_CONNECT` | -10 | Not connected |
| `ERROR_SEQUENCE` | -11 | Sequence missmatch |

### CAN Interface Layer (CANIF)

#### `canif_add()`

Adds a CAN interface to the system.

```c
lwcanerr_t canif_add(struct canif *canif, const char *name, canif_init_function init);
```

**Parameters:**
- `canif` - Pointer to CAN interface structure
- `name` - Interface name (e.g., "can0")
- `init` - Initialization function pointer

**Returns:** `ERROR_OK` on success, error code otherwise

#### `canif_remove()`

Removes a CAN interface from the system.

```c
lwcanerr_t canif_remove(struct canif *canif);
```

#### `canif_set_bitrate()`

Sets the bitrate for a CAN interface.

```c
lwcanerr_t canif_set_bitrate(struct canif *canif, uint32_t bitrate);
```

#### `canif_set_filter()`

Sets the receive filter for a CAN interface.

```c
lwcanerr_t canif_set_filter(struct canif *canif, struct can_filter *filter);
```

#### `canif_get_by_name()`

Retrieves a CAN interface by name.

```c
struct canif *canif_get_by_name(const char *name);
```

#### `canif_get_by_index()`

Retrieves a CAN interface by index.

```c
struct canif *canif_get_by_index(uint8_t index);
```

#### `canif_name_to_index()`

Converts interface name to index.

```c
uint8_t canif_name_to_index(const char *name);
```

#### `canif_index_to_name()`

Converts interface index to name.

```c
const char *canif_index_to_name(uint8_t index);
```

#### `canif_get_name()`

Returns the name of a CAN interface.

```c
const char *canif_get_name(struct canif *canif);
```

#### `canif_get_index()`

Returns the index of a CAN interface.

```c
uint8_t canif_get_index(struct canif *canif);
```

### RAW Layer

The RAW layer provides direct access to CAN frames without protocol overhead.

#### `canraw_new()`

Creates a new RAW protocol control block.

```c
struct canraw_pcb *canraw_new(void);
```

**Returns:** Pointer to PCB or NULL if memory allocation failed

#### `canraw_bind()`

Binds a RAW PCB to a CAN interface.

```c
lwcanerr_t canraw_bind(struct canraw_pcb *pcb, struct addr_can *addr);
```

**Parameters:**
- `pcb` - Protocol control block
- `addr` - CAN address with interface index

#### `canraw_remove()`

Removes and frees a RAW PCB.

```c
void canraw_remove(struct canraw_pcb *pcb);
```

#### `canraw_send()`

Sends a CAN frame.

```c
lwcanerr_t canraw_send(
    struct canraw_pcb *pcb,
    void *frame,
    uint8_t frame_size,
    uint32_t timeout,
    canif_sent_function sent,
    void *arg
);
```

**Parameters:**
- `pcb` - Protocol control block
- `frame` - Pointer to CAN frame (can_frame or canfd_frame)
- `frame_size` - Size of the frame structure
- `timeout` - Transmission timeout in milliseconds
- `sent` - Callback function (optional)
- `arg` - Argument passed to callback

#### `canraw_set_receive_callback()`

Sets the receive callback function.

```c
lwcanerr_t canraw_set_receive_callback(
    struct canraw_pcb *pcb,
    canraw_receive_function receive
);
```

#### `canraw_set_callback_arg()`

Sets the callback argument for a RAW PCB.

```c
lwcanerr_t canraw_set_callback_arg(
    struct canraw_pcb *pcb,
    void *arg
);
```

**Callback signature:**
```c
typedef uint8_t (*canraw_receive_function)(
    void *arg,
    struct canraw_pcb *pcb,
    void *frame
);
```

Return 1 to consume the frame, 0 to pass it to other listeners.

### ISOTP Layer

ISOTP (ISO 15765-2) implements transport-layer segmentation and reassembly.

#### ISOTP Frame Types

| Type | Value | Description |
|------|-------|-------------|
| Single Frame (SF) | 0x00 | Single frame message (≤7 bytes) |
| First Frame (FF) | 0x10 | First frame of segmented message |
| Consecutive Frame (CF) | 0x20 | Subsequent frames |
| Flow Control (FC) | 0x30 | Flow control response |

#### `isotp_new()`

Creates a new ISOTP protocol control block.

```c
struct isotp_pcb *isotp_new(void);
```

**Returns:** Pointer to PCB or NULL if memory allocation failed

#### `isotp_bind()`

Binds an ISOTP PCB to CAN addresses.

```c
lwcanerr_t isotp_bind(struct isotp_pcb *pcb, const struct addr_can *addr);
```

**Parameters:**
- `pcb` - Protocol control block
- `addr` - CAN address with interface index and CAN IDs

**Example:**
```c
struct addr_can addr = {
    .can_ifindex = 1,
    .can_addr.tp = {
        .tx_id = 0x7E0,
        .rx_id = 0x7E8
    }
};
lwcanerr_t ret = isotp_bind(pcb, &addr);
```

#### `isotp_remove()`

Removes and frees an ISOTP PCB.

```c
void isotp_remove(struct isotp_pcb *pcb);
```

#### `isotp_send()`

Sends data using ISOTP.

```c
lwcanerr_t isotp_send(struct isotp_pcb *pcb, const uint8_t *data, uint32_t length);
```

**Parameters:**
- `pcb` - Protocol control block
- `data` - Pointer to data buffer
- `length` - Data length (supports >8 bytes via segmentation)

#### `isotp_received()`

Processes received CAN frames for ISOTP.

```c
lwcanerr_t isotp_received(struct isotp_pcb *pcb, struct lwcan_buffer *buffer);
```

#### ISOTP Callbacks

##### Setting Callbacks

```c
lwcanerr_t isotp_set_receive_callback(struct isotp_pcb *pcb, isotp_receive_function receive);
lwcanerr_t isotp_set_receive_ff_callback(struct isotp_pcb *pcb, isotp_receive_ff_function receive_ff);
lwcanerr_t isotp_set_sent_callback(struct isotp_pcb *pcb, isotp_sent_function sent);
lwcanerr_t isotp_set_error_callback(struct isotp_pcb *pcb, isotp_error_function error);
lwcanerr_t isotp_set_callback_arg(struct isotp_pcb *pcb, void *arg);
```

##### Receive Callback

```c
typedef void (*isotp_receive_function)(
    void *arg,
    struct isotp_pcb *pcb,
    struct lwcan_buffer *buffer
);
```

##### First Frame Callback

```c
typedef void (*isotp_receive_ff_function)(
    void *arg,
    struct isotp_pcb *pcb
);
```

Called when a First Frame is received.

##### Sent Callback

```c
typedef void (*isotp_sent_function)(
    void *arg,
    struct isotp_pcb *pcb,
    uint32_t length
);
```

Called after data has been transmitted.

##### Error Callback

```c
typedef void (*isotp_error_function)(
    void *arg,
    lwcanerr_t error
);
```

#### ISOTP Configuration Options

Defined in `lwcan/options.h`:

| Option | Default | Description |
|--------|---------|-------------|
| `LWCAN_NO_SYSTEM` | 0 | Use lwCAN without OS-awareness (no thread) |
| `LWCAN_MEM_SIZE` | 512 | Total memory pool size in bytes |
| `LWCAN_ISOTP` | 1 | Enable ISOTP support |
| `ISOTP_CANFD` | 0 | Enable CAN FD support |
| `ISOTP_CANFD_BRS` | 1 | Enable bitrate switch |
| `ISOTP_PADDING_BYTE` | 0xAA | Padding byte value |
| `ISOTP_RECEIVE_BS` | 0 | Block size (0 = no flow control) |
| `ISOTP_RECEIVE_ST` | 0 | Separation time (ms) |
| `ISOTP_N_WFT` | 0 | Max waiting frames |
| `ISOTP_N_AS` | 1000 | Transmission timeout (ms) |
| `ISOTP_N_BS` | 1000 | Block size timeout (ms) |
| `ISOTP_N_CS` | 5 | Consecutive frame timeout (ms) |
| `ISOTP_N_BR` | 5 | Block receive timeout (ms) |
| `ISOTP_N_CR` | 1000 | Consecutive frame receive timeout (ms) |
| `ISOTP_MAX_PCB_NUM` | 2 | Maximum concurrent connections |
| `LWCAN_RAW` | 0 | Enable RAW layer |
| `CANRAW_MAX_PCB_NUM` | 2 | Maximum RAW connections |
| `LWCAN_THREAD_NAME` | "lwcan_thread" | Thread name (when LWCAN_NO_SYSTEM=0) |
| `LWCAN_THREAD_STACKSIZE` | 0 | Thread stack size in bytes (platform-dependent) |
| `LWCAN_THREAD_PRIO` | 1 | Thread priority (platform-dependent) |

### LWCAN_NO_SYSTEM: Threaded vs Polling Mode

The `LWCAN_NO_SYSTEM` option controls how the library handles internal processing:

#### Default Mode (`LWCAN_NO_SYSTEM = 0`) — Background Thread

When `LWCAN_NO_SYSTEM` is disabled (default), lwcan creates a dedicated background thread on `lwcan_init()`. This thread continuously calls `lwcan_timeouts_handler()` and sleeps for the remaining time until the next timeout.

**Behavior:**
- A thread is created with name `LWCAN_THREAD_NAME`, priority `LWCAN_THREAD_PRIO`, and stack size `LWCAN_THREAD_STACKSIZE`
- The thread calls `system_msleep()` for the remaining time returned by `lwcan_timeouts_handler()`
- When a new timeout is registered or removed, the thread is woken up via `system_wakeup()`
- **User code does not need to call `lwcan_timeouts_handler()` manually**

```c
// No manual polling needed — the library handles it internally
lwcan_init();
// ... use the library ...
// lwcan_timeouts_handler() is called automatically by the background thread
```

#### Polling Mode (`LWCAN_NO_SYSTEM = 1`) — Manual Polling

When `LWCAN_NO_SYSTEM` is set to 1, lwcan runs without OS awareness. No thread is created, and the user is responsible for calling `lwcan_timeouts_handler()` periodically.

**Behavior:**
- No thread is created on `lwcan_init()`
- The user must call `lwcan_timeouts_handler()` in their own loop (e.g., main loop, ISR, or timer callback)
- `system_msleep()` and `system_wakeup()` are no-ops
- `system_now()` must still be implemented for time tracking

```c
// User must manually poll the timeout handler
lwcan_init();

while (1) {
    // ... application logic ...
    lwcan_timeouts_handler();  // Required: processes expired timeouts
    // system_msleep() is a no-op, use your platform's sleep function
}
```

**When to use Polling Mode:**
- Bare-metal applications without an RTOS
- When the application needs full control over the processing loop
- When integrating lwcan into an existing event loop or timer callback
- When thread resources are constrained

#### Platform Functions

When `LWCAN_NO_SYSTEM = 0`, the following platform functions must be implemented in `lwcanarch/system_arch.h`:

| Function | Purpose |
|----------|---------|
| `system_thread_new()` | Create a new thread |
| `system_msleep()` | Sleep for specified milliseconds |
| `system_wakeup()` | Wake up a sleeping thread |
| `system_now()` | Return current time in milliseconds |

When `LWCAN_NO_SYSTEM = 1`, only `system_now()` is required.

### Timeout Management

> **See also:** [LWCAN_NO_SYSTEM: Threaded vs Polling Mode](#LWCAN_NO_SYSTEM-Threaded-vs-Polling-Mode) — explains how the library processes timeouts in background thread vs manual polling mode.

#### `lwcan_timeout()`

Registers a timeout callback.

```c
void lwcan_timeout(
    struct lwcan_timeout *timeout,
    uint32_t time_ms,
    lwcan_timeout_handler handler,
    void *arg
);
```

**Parameters:**
- `timeout` - Pointer to `lwcan_timeout` structure to manage
- `time_ms` - Timeout in milliseconds
- `handler` - Callback function
- `arg` - Argument passed to callback

#### `lwcan_untimeout()`

Cancels a registered timeout.

```c
void lwcan_untimeout(struct lwcan_timeout *timeout);
```

**Parameters:**
- `timeout` - Pointer to `lwcan_timeout` structure to cancel

#### `lwcan_timeouts_handler()`

Processes expired timeouts (called periodically). Returns remaining time until next timeout.

```c
uint32_t lwcan_timeouts_handler(void);
```

**Returns:** Remaining time in milliseconds until the next scheduled timeout, or 0 if no timeouts are pending.

### UDS Client

UDS (Unified Diagnostic Services) client implementation for vehicle diagnostics.

#### `uds_client_init()`

Initializes the UDS client.

```c
lwcanerr_t uds_client_init(void);
```

#### `uds_client_deinit()`

Deinitializes the UDS client.

```c
lwcanerr_t uds_client_deinit(void);
```

#### `uds_client_connect()`

Connects to a UDS server.

```c
lwcanerr_t uds_client_connect(
    const struct addr_can *addr,
    connect_cb_function cb,
    void *arg
);
```

#### `uds_client_disconnect()`

Disconnects from the UDS server.

```c
void uds_client_disconnect(void);
```

#### `uds_set_context()`

Sets the UDS context callbacks.

```c
lwcanerr_t uds_set_context(const struct uds_context *context, void *handle);
```

#### `uds_set_p2_timer()`

Sets the P2 timer value.

```c
lwcanerr_t uds_set_p2_timer(uint32_t time);
```

#### `uds_set_p2_star_timer()`

Sets the P2* timer value.

```c
lwcanerr_t uds_set_p2_star_timer(uint32_t time);
```

**Context structure:**
```c
struct uds_context {
    void (*request)(void *handle, uint8_t sid, uint8_t *data, uint32_t size);           /* For server (not implemented yet) */
    void (*positive_response)(void *handle, uint8_t *data, uint32_t size);               /* For client */
    void (*negative_response)(void *handle, uint8_t rejected_sid, uint8_t nrc);          /* For client */
    void (*error)(void *handle, lwcanerr_t error);                                       /* Common */
};
```

#### `uds_send_request()`

Sends a UDS request.

```c
lwcanerr_t uds_send_request(
    const void *request,
    uint32_t size,
    uint8_t with_response
);
```

**Parameters:**
- `request` - UDS request data (SID + parameters)
- `size` - Request size
- `with_response` - 0 for no response, >0 for expected response

#### UDS Configuration

| Option | Default | Description |
|--------|---------|-------------|
| `UDS_S3_SERVER` | 10000 | Server session timeout (ms) |
| `UDS_S3_CLIENT` | 9000 | Client session timeout (ms) |
| `UDS_P2_DEFAULT` | 50 | P2 timer default (ms) |
| `UDS_P2_STAR_DEFAULT` | 500 | P2* timer default (ms) |
| `UDS_CONNECT_RETRY_NUM` | 20 | Connection retries |
| `UDS_CONNECT_RETRY_TIMEOUT` | 50 | Retry timeout (ms) |
| `UDS_KEEP_SESSION` | 1 | Keep session with tester present (uses 0x31 SetDiagnosticSessionControl) |
| `UDS_KEEP_SESSION_SUPPRESS` | 0x80 | Tester present subfunction (suppress response) |

---

## Configuration

### lwcan_options.h

Create `lwcan_options.h` in your project and modify configuration:

```c
/* Memory configuration */
#define LWCAN_MEM_SIZE              512

/* Protocol enable/disable */
#define LWCAN_ISOTP                 1
#define LWCAN_RAW                   1

/* Protocol-specific options */
#define ISOTP_MAX_PCB_NUM           4
#define CANRAW_MAX_PCB_NUM          4
```

### Platform Integration

Create `lwcanarch/cc.h` for platform-specific defines:

```c
#ifndef LWCC_ARCH_H
#define LWCC_ARCH_H

/* Platform assert */
#define LWCAN_PLATFORM_ASSERT(x) \
    do { \
        printf("Assertion failed: %s\n", x); \
        abort(); \
    } while(0)

#endif
```

Implement `system_now()` to return milliseconds since startup.

---

## Protocol Implementations

### ISOTP Protocol Flow

#### Sending Data

```
Sender                    Receiver
------                    --------

[Data > 8 bytes]
       -->
       |  First Frame (FF)
       |  Payload: first 6 bytes
       |
       |  <-- Flow Control (FC)
       |     Accept
       |
       |  Consecutive Frame (CF) #1
       |     Payload: next 7 bytes
       |
       |  Consecutive Frame (CF) #2
       |     Payload: remaining bytes
       |
       |  [More CFs...]
       |
       V  Final Frame
```

#### Receiving Data

```
Sender                    Receiver
------                    --------

       -->
       |  First Frame (FF)
       |
       |  <-- Flow Control (FC)
       |     Block size: 0 (no flow control)
       |     Separation time: 0
       |
       |  Consecutive Frame (CF) #1
       |
       |  Consecutive Frame (CF) #2
       |
       |  [More CFs...]
       |
       V  Final Frame
```

### UDS Protocol

UDS uses ISOTP for transport and defines diagnostic services, e.g.:

| Service | Description |
|---------|-------------|
| 0x10 | Diagnostic Session Control |
| 0x11 | ECU Reset |
| 0x22 | Read Data By Identifier |
| 0x27 | Security Access |
| 0x28 | Communication Control |
| 0x2E | Write Data By Identifier |
| 0x3E | Tester Present |
| 0x85 | Control DTC Setting |
| 0x14 | Clear Diagnostic Information |
| 0x19 | Read DTC Information |

**Example UDS Request:**
```c
// Read Data By Identifier (0x22) for VIN (0xF190)
uint8_t request[] = {0x22, 0xF1, 0x90};
uds_send_request(request, 3, 1);
```

---

## Examples

### Example 1: RAW CAN Communication

```c
#include "lwcan/init.h"
#include "lwcan/raw.h"
#include "lwcan/can.h"

void raw_receive_example(void *arg, struct canraw_pcb *pcb, void *frame) {
    struct can_frame *can_frame = (struct can_frame *)frame;
    printf("Received CAN frame: ID=0x%X, DLC=%d\n",
           can_frame->can_id & CAN_EFF_MASK, can_frame->len);
}

void raw_send_example(void) {
    // Create RAW PCB
    struct canraw_pcb *pcb = canraw_new();

    // Bind to interface
    struct addr_can addr = {
        .can_ifindex = canif_name_to_index("can0")
    };
    canraw_bind(pcb, &addr);

    // Set receive callback
    canraw_set_receive_callback(pcb, raw_receive_example);

    // Send a frame
    struct can_frame frame = {
        .can_id = 0x123,
        .len = 4,
        .data = {0x01, 0x02, 0x03, 0x04}
    };
    canraw_send(pcb, &frame, sizeof(frame), 1000, NULL, NULL);
}
```

### Example 2: ISOTP Communication

```c
#include "lwcan/init.h"
#include "lwcan/isotp.h"

void isotp_receive(void *arg, struct isotp_pcb *pcb, struct lwcan_buffer *buffer) {
    printf("Received ISOTP message: %.*s\n",
           (int)buffer->length, (char *)buffer->payload);
}

void isotp_send_example(void) {
    // Create ISOTP PCB
    struct isotp_pcb *pcb = isotp_new();

    // Bind to addresses
    struct addr_can addr = {
        .can_ifindex = canif_name_to_index("can0"),
        .can_addr.tp = {
            .tx_id = 0x7E0,
            .rx_id = 0x7E8
        }
    };
    isotp_bind(pcb, &addr);

    // Set callbacks
    isotp_set_receive_callback(pcb, isotp_receive);

    // Send long message (will be segmented)
    const char *message = "This is a long message that needs segmentation";
    isotp_send(pcb, (const uint8_t *)message, strlen(message));
}
```

### Example 3: UDS Client

```c
#include "lwcan/init.h"
#include "lwcan/apps/uds_client.h"

void uds_positive_response(void *handle, uint8_t *data, uint32_t size) {
    printf("UDS Positive Response: ");
    for (uint32_t i = 0; i < size; i++) {
        printf("0x%02X ", data[i]);
    }
    printf("\n");
}

void uds_negative_response(void *handle, uint8_t rejected_sid, uint8_t nrc) {
    printf("UDS Negative Response: SID=0x%02X, NRC=0x%02X\n", rejected_sid, nrc);
}

void uds_error(void *handle, lwcanerr_t error) {
    printf("UDS Error: %d\n", error);
}

void uds_example(void) {
    // Initialize UDS client
    uds_client_init();

    // Set context
    struct uds_context context = {
        .positive_response = uds_positive_response,
        .negative_response = uds_negative_response,
        .error = uds_error
    };
    uds_set_context(&context, NULL);

    // Connect to server
    struct addr_can addr = {
        .can_ifindex = canif_name_to_index("can0"),
        .can_addr.tp = {
            .tx_id = 0x7E0,
            .rx_id = 0x7E8
        }
    };

    uds_client_connect(&addr, NULL, NULL);

    // Send diagnostic request
    uint8_t request[] = {0x22, 0xF1, 0x90};
    uds_send_request(request, 3, 1);
}
```

### Example 4: CAN FD with ISOTP

```c
#include "lwcan/init.h"
#include "lwcan/isotp.h"

// Enable CAN FD in lwcan_options.h:
// #define ISOTP_CANFD 1
// #define ISOTP_CANFD_BRS 1

void canfd_isotp_example(void) {
    struct isotp_pcb *pcb = isotp_new();

    struct addr_can addr = {
        .can_ifindex = 1,
        .can_addr.tp = {
            .tx_id = 0x7DA,
            .rx_id = 0x7DB
        }
    };
    isotp_bind(pcb, &addr);

    // Send CAN FD frame (up to 64 bytes)
    uint8_t data[64];
    memset(data, 0x55, sizeof(data));
    isotp_send(pcb, data, sizeof(data));
}
```

---

## Memory Management

lwcan uses pool-based memory allocation for predictable behavior:

- ISOTP PCBs: `ISOTP_MAX_PCB_NUM` (default: 2)
- RAW PCBs: `CANRAW_MAX_PCB_NUM` (default: 2)
- Buffer memory pool: `LWCAN_MEM_SIZE` (default: 512 bytes)

Memory pool sizes can be configured in `lwcan_options.h`.

---

## Platform-Specific Implementation

### CAN Driver Interface

Implement the CAN interface functions for your hardware:

```c
lwcanerr_t my_can_init(struct canif *canif) {
    // Initialize CAN controller
    CAN_Init(...);
    return ERROR_OK;
}

lwcanerr_t my_can_output(struct canif *canif, void *frame,
                        uint8_t frame_size, uint32_t timeout,
                        canif_sent_function sent, void *arg) {
    // Send frame via hardware
    CAN_Transmit(...);
    // Call sent callback when complete
    sent(arg, ERROR_OK);
    return ERROR_OK;
}

lwcanerr_t my_can_set_filter(struct canif *canif, struct can_filter *filter) {
    // Configure hardware filter
    CAN_SetFilter(...);
    return ERROR_OK;
}
```

### System Time

Implement `system_now()` for timeout management:

```c
#include <sys/time.h>

uint32_t system_now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
```
