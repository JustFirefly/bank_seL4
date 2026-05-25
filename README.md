# High-Assurance Banking System on seL4

A showcase-level, component-based mock banking application built natively on the **seL4 microkernel** using the **CAmkES** (Component Architecture for microkernel-based Embedded Systems) framework. 

This project demonstrates strict user-space isolation, the principle of least privilege, fault isolation, and capability-based access control at the lowest levels of systems programming.

## Architecture & Security Model

In a traditional monolithic kernel (like Linux), a vulnerability in the UI or web server could compromise the entire system's memory. On seL4, every component runs in its own entirely isolated hardware address space. Communication is strictly restricted to unforgeable kernel capabilities routed via explicit Inter-Process Communication (IPC) channels, Shared Memory, and Hardware Notifications.

```text
+-------------------------------------------------------------------------+
|                               USER SPACE                                |
|                                                                         |
|  [ VirtioHardware ] <--(MMIO/IRQ)--> [ NetworkStack ]                   |
|                                          ^  ^                           |
|                  (IPC: Net, Shared Data) |  | (IPC: Net, Shared Data)   |
|                                          v  v                           |
|   [ Client ] <---(Notify, Shared Data)-> [ APIGateway ]                 |
|      |   |                                  |   |                       |
|      |   +-----------(IPC: Auth)----------->+   |                       |
|      |                                      |   |                       |
|      |                               [ AuthServer ]                     |
|      +---------------(IPC: Bank)----------->+                           |
|                                             |                           |
|                                      [ Ledger ] <---(IPC: Bank)---------+
|                                          |                              |
|                                     (IPC: Disk)                         |
|                                          v                              |
|                                     [ Storage ]                         |
+-------------------------------------------------------------------------+
|                              KERNEL SPACE                               |
|                            seL4 Microkernel                             |
+-------------------------------------------------------------------------+

```

### Components

* **VirtioHardware (Hardware Space):** Exposes memory-mapped I/O (MMIO) and hardware IRQs from the virtualization layer for external network connectivity.
* **NetworkStack (Untrusted/Network Space):** Processes the raw network packets and exposes a clean IPC interface for network requests.
* **APIGateway (Router Space):** Acts as the ingress controller. It routes networking payloads to internal authentication and banking services via secure IPC.
* **Client (Untrusted User Space):** Simulates an incoming queue of requests and processes batch instructions.
* **AuthServer (Trusted Space):** Validates user credentials and issues cryptographic capability tokens upon successful authentication.
* **Ledger (High-Assurance Space):** Houses the core state via a sandboxed SQLite instance in memory, enforces authorization checks on tokens, and processes financial transactions.
* **Storage (Trusted Space):** A mock isolated block layer tasked with persisting ledger adjustments to a permanent backend. The ledger cannot touch disk directly; it must request this over IPC.

---

## Directory Structure

```text
projects/camkes/apps/banking_system/
├── CMakeLists.txt                 # Application build instructions
├── banking_system.camkes          # System assembly blueprint & topology
├── interfaces/                    # RPC Contract Definitions (IDL4)
│   ├── Auth.idl4
│   ├── Bank.idl4
│   ├── Network.idl4
│   └── Disk.idl4
└── components/                    # Individual isolated domains
    ├── APIGateway/
    │   ├── APIGateway.camkes
    │   └── src/gateway.c          # Ingress routing logic
    ├── AuthServer/
    │   ├── AuthServer.camkes
    │   └── src/auth.c             # Mock security token provider
    ├── Client/
    │   ├── Client.camkes
    │   └── src/client.c           # Automated Batch Processing logic
    ├── Ledger/
    │   ├── Ledger.camkes
    │   └── src/
    │       ├── ledger.c           # Core authorization & ledger logic
    │       ├── sqlite3.c          # Sandboxed SQLite implementation
    │       └── sqlite3.h
    ├── NetworkStack/
    │   ├── NetworkStack.camkes
    │   ├── VirtioHardware.camkes  # Hardware interface definitions
    │   ├── include/               # Network stack headers (lwipopts, etc.)
    │   └── src/network_stack.c    # Network protocol handling
    └── Storage/
        ├── Storage.camkes
        └── src/storage.c          # Storage persistence boundary

```

---

## Setup & Compilation

### 1. Install System Dependencies

```bash
sudo pacman -S base-devel git cmake ninja dtc ccache repo python python-virtualenv qemu-system-aarch64

```

### 2. Configure Python Environment & Workspace

seL4 and CAmkES require specific Python modules to generate boilerplate IPC code. Solved cleanly via a Python virtual environment:

```bash
# Set up isolated Python environment
python -m venv ~/sel4-venv
source ~/sel4-venv/bin/activate
pip install sel4-deps camkes-deps

# Clone workspace using Google repo tool
mkdir ~/sel4-workspace && cd ~/sel4-workspace
repo init -u [https://github.com/seL4/camkes-manifest.git](https://github.com/seL4/camkes-manifest.git)
repo sync

```

### 3. Build and Simulate

Compilation requires an out-of-tree build layout. Note that we are targeting `qemu-arm-virt` to support the VirtIO networking interfaces:

```bash
cd ~/sel4-workspace
mkdir build && cd build

# Configure CMake targeting ARM virtual simulation
../init-build.sh -DPLATFORM=qemu-arm-virt -DSIMULATION=TRUE -DCAMKES_APP=banking_system

# Compile system 
ninja

# Run QEMU emulator with specific networking arguments
./simulate --extra-qemu-args="-netdev user,id=net0,hostfwd=tcp::8080-:8080 -device virtio-net-device,netdev=net0"

```

*To exit the QEMU simulation loop, press `Ctrl+A` then release and press `X`.*

---

## Embedded Verification Scenarios

The `Client` component runs an automated batch processor simulating runtime adversarial boundaries to prove seL4's sandboxing mechanics:

1. **Scenario 1 (Unauthenticated Bypass Attempt):** The `Client` attempts to pass an invalid transaction request straight to the `Ledger`. The `Ledger` catches the invalid token and blocks the transfer at the boundary.
2. **Scenario 2 (Malicious Credentials):** An invalid user attempts to authenticate against the `AuthServer`. The request is scrutinized, rejected, and an invalid token handle is returned.
3. **Scenario 3 (The Happy Path):** A valid user provides correct authentication via the `NetworkStack` and `APIGateway`. The `AuthServer` securely maps a token. The caller passes that token to the `Ledger`, which authorizes the transfer, updates state in SQLite, and invokes the `Storage` layer over IPC to securely commit the data.

```

```
