# CANopen Motor Analyzer

## 1. Overview

CANopen Motor Analyzer is a Qt-based C++ application designed to communicate with and control CANopen-enabled motor drives. While initially conceptualized for a Maxon IDX56 motor and PEAK PCAN-USB adapter, the current version operates in a simulated environment. It provides a graphical user interface to:

- Connect to a CAN bus (simulated).
- Enable and disable the motor drive.
- Control the motor using:
    - Immediate Position Control (Profile Position Mode).
    - Immediate Velocity Control (Profile Velocity Mode).
    - Pre-defined Velocity Sequences.
- Read actual motor position and statusword (simulated values).
- (Future) Plot real-time motor data.

This project was developed in an environment where direct hardware testing was not possible. Therefore, the PEAK PCANBasic SDK and QCustomPlot library functionalities are simulated with dummy implementations to allow for compilation and logical testing of the application flow. The CAN communication and motor responses are simulated by the `CANHandler` class.

## 2. Features

- **CAN Connection Management**: Connect to and disconnect from a (simulated) CAN interface.
- **General Motor Control**:
    - Enable motor (transitions drive to "Operation Enabled" state, defaults to Profile Velocity mode).
    - Disable motor.
- **Immediate Position Control**:
    - Set a target position for the motor (assumes Profile Position mode is set by user if desired, as `Enable Motor` defaults to Profile Velocity).
- **Immediate Velocity Control**:
    - Set target velocity, profile acceleration, and profile deceleration for direct velocity control.
    - The application sets the Mode of Operation to Profile Velocity (3) when these commands are issued.
    - A dedicated "Stop Motor" button halts motion using the configured deceleration by setting target velocity to zero.
- **Sequence Definition and Execution**:
    - Define a sequence of steps, where each step includes:
        - Target Velocity
        - Duration (in milliseconds)
        - Profile Acceleration for the step
        - Profile Deceleration for the step
    - Add steps to a sequence table.
    - Remove selected steps or clear the entire sequence.
    - Execute the defined sequence.
    - Stop a running sequence.
- **Data Display**:
    - Show (simulated) actual motor position and statusword.
    - Log operations, SDO communication (simulated), and errors to a message window.
- **SDO Communication**:
    - All motor control features utilize CANopen SDO (Service Data Object) messages to write parameters (e.g., Controlword, Target Position, Target Velocity, Mode of Operation, Acceleration/Deceleration) and read status. This is simulated internally.
- **(Future) Plotting**: A QCustomPlot widget is integrated, but actual plotting is currently disabled due to the dummy library.

## 3. Dependencies

### 3.1. Runtime Dependencies (for a real deployment)
- **PEAK PCAN-Basic API**: Driver and library from PEAK-System.
- **Qt5 Runtime Libraries**: Core, GUI, Widgets.
- **QCustomPlot Library**: Runtime component if used as a shared library.

### 3.2. Build-time Dependencies (General)
- **C++ Compiler**: Supporting C++11 (e.g., g++ on Linux, MinGW or MSVC on Windows).
- **Qt Development Tools**: Qt installation including qmake, Qt Core, GUI, Widgets headers and libraries.
- **PEAK PCAN-Basic SDK**: Header (`PCANBasic.h`) and library (`.lib`/`.so`).
    - *Simulated in this project with `libs/pcanbasic/PCANBasic.h` and `libs/pcanbasic/PCANBasic.cpp`.*
- **QCustomPlot Library**: Header (`qcustomplot.h`) and source (`qcustomplot.cpp`).
    - *Simulated in this project with `libs/qcustomplot/qcustomplot.h` and `libs/qcustomplot/qcustomplot.cpp`.*

## 4. Project Structure

```
CANAnalyzer/
├── CANAnalyzer.pro         # qmake project file
├── main.cpp                # Main application entry point
├── mainwindow.h            # GUI MainWindow header
├── mainwindow.cpp          # GUI MainWindow implementation
├── can_handler.h           # CAN communication handling header
├── can_handler.cpp         # CAN communication handling implementation
├── motor_controller.h      # Motor control logic header
├── motor_controller.cpp    # Motor control logic implementation
├── libs/
│   ├── pcanbasic/
│   │   ├── PCANBasic.h     # (Dummy) PCANBasic SDK header
│   │   └── PCANBasic.cpp   # (Dummy) PCANBasic SDK implementation
│   └── qcustomplot/
│       ├── qcustomplot.h   # (Dummy) QCustomPlot header
│       └── qcustomplot.cpp # (Dummy) QCustomPlot implementation
└── README.md               # This file
```

## 5. Setup and Build Instructions

This section provides guidance for building the project on Linux and Windows.

### 5.1. General Notes on External Libraries (for Real Deployment)

For a real deployment with actual hardware, you would need to:
- **Replace Dummy PCAN-Basic SDK**: Download the official SDK from [PEAK-System website](https://www.peak-system.com). Replace `libs/pcanbasic/PCANBasic.h` with the official header. The corresponding library file (`.so` for Linux, `.lib`/`.dll` for Windows) will need to be correctly linked in the `CANAnalyzer.pro` file and/or placed where the system or application can find it.
- **Replace Dummy QCustomPlot**: Download the full source files (`qcustomplot.h`, `qcustomplot.cpp`) from the [QCustomPlot website](https://www.qcustomplot.com) and replace the dummy versions in `libs/qcustomplot/`.

*Note: For the current simulated project as provided, the dummy library files are already included in the `libs/` directory and are compiled directly.*

### 5.2. Building on Linux (Example: Debian/Ubuntu)

#### 5.2.1. Prerequisites

- **Core Build Tools**:
    - `build-essential`: Provides g++, make, etc.
- **Qt5 Development Libraries**:
    - `qt5-qmake`: Qt 5 qmake tool.
    - `qtbase5-dev`: Qt 5 development headers and libraries.

Install these using:
```bash
sudo apt-get update && sudo apt-get install -y build-essential qt5-qmake qtbase5-dev
```
*(Note: For other Linux distributions, package names might vary. Please use your distribution's package manager.)*

#### 5.2.2. Build Steps

1.  **Navigate to Project Directory**:
    ```bash
    cd /path/to/your/CANAnalyzer_source_code/CANAnalyzer
    ```
    *(Replace path as needed.)*
2.  **Generate Makefile**:
    ```bash
    qmake CANAnalyzer.pro
    ```
3.  **Compile**:
    ```bash
    make
    ```
4.  **Clean Build (Optional)**: `make clean`, then repeat steps 2 and 3.

### 5.3. Building on Windows

Building on Windows can be done using Qt Creator (with MinGW or MSVC compilers) or Visual Studio (with the Qt VS Tools extension).

#### 5.3.1. General Prerequisites for Windows

1.  **Qt Installation**:
    *   Use the Qt Online Installer from the [official Qt website](https://www.qt.io/download-qt-installer).
    *   Select a Qt version (e.g., Qt 5.15.x).
    *   Choose compiler toolchains (MinGW for Qt Creator standalone, or an MSVC version matching your Visual Studio).
    *   Ensure Qt Creator is selected if you plan to use it.
2.  **PEAK PCAN-Basic SDK (for Windows)**:
    *   Download from the [PEAK-System website](https://www.peak-system.com).
    *   This typically includes `PCANBasic.h`, `PCANBasic.lib` (32/64-bit), and `PCANBasic.dll` (32/64-bit).
3.  **QCustomPlot (Full Library)**:
    *   Download `qcustomplot.h` and `qcustomplot.cpp` from its [official website](https://www.qcustomplot.com).
4.  **Git for Windows (Optional)**:
    *   For cloning the repository, available at [git-scm.com](https://git-scm.com).

#### 5.3.2. Method 1: Using Qt Creator on Windows

1.  **Obtain Source Code & Prepare Libraries (for Real Hardware)**:
    *   Clone or download the `CANAnalyzer` source code.
    *   Replace dummy `libs/pcanbasic/PCANBasic.h` with the real header.
    *   Place the appropriate `PCANBasic.lib` (32-bit or 64-bit matching your Qt Kit) in a location like `libs/pcanbasic/lib/win_x64/`.
    *   Replace dummy `libs/qcustomplot/` files with the real `qcustomplot.h` and `qcustomplot.cpp`.
2.  **Open and Configure Project**:
    *   Open `CANAnalyzer.pro` in Qt Creator.
    *   Select an appropriate Qt Kit (e.g., MinGW 64-bit or an MSVC kit).
3.  **Update `.pro` File for PCAN-Basic (for Real Hardware)**:
    *   Edit `CANAnalyzer.pro` to link against `PCANBasic.lib`:
      ```pro
      win32 { # This block executes only on Windows
          # Uncomment and adjust path and x64/x86 as needed for real SDK
          # LIBS += -L$$PWD/libs/pcanbasic/lib/win_x64/ -lPCANBasic
          # INCLUDEPATH += $$PWD/libs/pcanbasic # Already included for dummy
          # DEPENDPATH += $$PWD/libs/pcanbasic  # Already included for dummy
      }
      ```
    *   The existing `INCLUDEPATH` and `DEPENDPATH` for `libs/pcanbasic` (used for the dummy `PCANBasic.h`) should be sufficient for the header. The main change is uncommenting and configuring the `LIBS` line for the actual library.
    *   Re-run qmake if prompted or manually.
4.  **Handle `PCANBasic.dll` (for Real Hardware)**:
    *   Copy the `PCANBasic.dll` (matching your build architecture) to the build output directory (e.g., `build-CANAnalyzer-Desktop_Qt_MinGW_w64_bit-Debug/debug/`) alongside `CANAnalyzer.exe`, or add its location to the system PATH.
5.  **Build and Run**:
    *   Use Qt Creator's build (Ctrl+B) and run (Ctrl+R) actions.

#### 5.3.3. Method 2: Using Visual Studio with Qt VS Tools

1.  **Install Qt VS Tools & Configure Qt**:
    *   In Visual Studio: `Extensions > Manage Extensions`, install "Qt Visual Studio Tools".
    *   Configure Qt versions: `Qt VS Tools > Qt Versions > Add New Qt Version`.
2.  **Obtain Source Code & Prepare Libraries**: (Similar to Qt Creator Step 1)
3.  **Import Project**:
    *   `Qt VS Tools > Open Qt Project File (.pro)...` to create `.sln` and `.vcxproj` from `CANAnalyzer.pro`.
4.  **Configure Project Properties for PCAN-Basic (for Real Hardware)**:
    *   Right-click project > Properties.
    *   Adjust for your Configuration (Debug/Release) and Platform (x64/Win32).
    *   `VC++ Directories > Include Directories`: Add path to `PCANBasic.h` (e.g., `$(ProjectDir)libs\pcanbasic\`).
    *   `VC++ Directories > Library Directories`: Add path to `PCANBasic.lib` (e.g., `$(ProjectDir)libs\pcanbasic\lib\win_x64\`).
    *   `Linker > Input > Additional Dependencies`: Add `PCANBasic.lib`.
5.  **Handle `PCANBasic.dll`**: (Similar to Qt Creator Step 4)
6.  **Build and Run**: Use Visual Studio's build/run commands.

#### 5.3.4. Note on Uncommenting Plotting Code
If you integrate the full QCustomPlot library (replacing the dummy files), remember to uncomment the plotting-related code in `mainwindow.cpp` to enable graphing features.

### 5.4. Running the Application

After a successful build:
- The executable `CANAnalyzer.exe` (on Windows) or `CANAnalyzer` (on Linux) will typically be found in a build-specific subdirectory next to your source code folder (e.g., `build-CANAnalyzer-Desktop_...-Debug/`) or directly in the project root if shadow building is disabled.
- Run it from its location (e.g., by double-clicking on Windows or `./CANAnalyzer` from terminal in Linux).

*(Remember, this version of the application, as built with the provided dummy libraries, runs in a simulated environment.)*

## 6. Usage Guidelines
(This section remains largely the same as before, but users now have more detailed build OS-specific build info)
... (rest of the README content from previous version) ...

## 7. Deliverables (Previously Section 6)
(This section remains the same)
... (rest of the README content from previous version) ...
