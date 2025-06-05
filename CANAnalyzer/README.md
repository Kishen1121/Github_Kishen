# CANopen Motor Analyzer

## 1. Overview

CANopen Motor Analyzer is a Qt-based C++ application designed to communicate with and control a CANopen-enabled motor drive, specifically targeting a Maxon IDX56 motor via a PEAK PCAN-USB adapter. It provides a graphical user interface to:

- Connect to a CAN bus.
- Enable and disable the motor.
- Set target motor positions (Profile Position Mode).
- Read actual motor position and statusword.
- (Future) Plot real-time motor data.

This project was developed in an environment where direct hardware testing was not possible. Therefore, the PCANBasic SDK and QCustomPlot library functionalities are simulated with dummy implementations to allow for compilation and logical testing of the application flow.

## 2. Dependencies

### 2.1. Runtime Dependencies (for a real deployment)
- **PEAK PCAN-Basic API**: Driver and library from PEAK-System for the PCAN-USB adapter.
- **Qt5 Runtime Libraries**: Core, GUI, Widgets.
- **QCustomPlot Library**: (If used as a shared library, its runtime component).

### 2.2. Build-time Dependencies
- **C++ Compiler**: Supporting C++11 (e.g., g++).
- **Qt5 Development Tools**: `qt5-qmake`, `qtbase5-dev` (includes headers, libraries, moc, rcc, uic).
- **PEAK PCAN-Basic SDK**: Header (`PCANBasic.h`) and library (`.lib`/`.so`) file.
    - *Simulated in this project with `libs/pcanbasic/PCANBasic.h` and `libs/pcanbasic/PCANBasic.cpp`.*
- **QCustomPlot Library**: Header (`qcustomplot.h`) and source (`qcustomplot.cpp`).
    - *Simulated in this project with `libs/qcustomplot/qcustomplot.h` and `libs/qcustomplot/qcustomplot.cpp`.*

## 3. Project Structure

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

## 4. Setup and Build Instructions

These instructions assume a Linux environment with g++ and Qt5 development tools installed.

### 4.1. Install Dependencies (Ubuntu Example)
```bash
sudo apt-get update
sudo apt-get install -y build-essential qt5-qmake qtbase5-dev
```

### 4.2. Obtain Libraries (for a real deployment)
- **PEAK PCAN-Basic SDK**: Download from [PEAK-System website](https://www.peak-system.com). Place `PCANBasic.h` into `libs/pcanbasic/` and the appropriate library file (e.g., `libPCBUSB.so`) into a system library path or a project-local path linked in the `.pro` file (e.g., by uncommenting and adjusting `LIBS += -L$$PWD/libs/pcanbasic/ -lYourPcanLib`).
- **QCustomPlot**: Download from [QCustomPlot website](https://www.qcustomplot.com). Replace the dummy `qcustomplot.h` and `qcustomplot.cpp` in `libs/qcustomplot/` with the full source files from the library.

*Note: For this simulated project, the dummy library files are already included.*

### 4.3. Build the Application
1.  Navigate to the `CANAnalyzer` project directory:
    ```bash
    cd path/to/CANAnalyzer
    ```
2.  Run `qmake` to generate the Makefile:
    ```bash
    qmake CANAnalyzer.pro
    ```
3.  Run `make` to compile the project:
    ```bash
    make
    ```
    A `make clean` might be useful if you encounter issues with stale build artifacts.
4.  The executable `CANAnalyzer` will be created in the project directory (or a build-specific directory depending on Qt configuration).

## 5. Usage Guidelines

1.  **Run the Application**: Execute `./CANAnalyzer`.
2.  **Configuration**:
    - **CAN Channel**: Enter the appropriate PCAN channel name for your adapter (e.g., `PCAN_USBBUS1`). (Currently uses a hardcoded dummy value).
    - **Baud Rate**: Enter the CAN bus baud rate in kbit/s (e.g., `500`). (Currently uses a hardcoded dummy value).
    - **Motor Node ID**: Enter the CANopen Node ID of your motor drive (e.g., `1`).
3.  **Connect**: Click the "Connect to CAN" button. Log messages will indicate success or failure.
4.  **Motor Control**:
    - Once connected, use the "Enable Motor" button to attempt to enable the motor drive.
    - Enter a desired position in the "Target Pos" field and click "Set Position".
    - Click "Disable Motor" to disable the drive.
5.  **Data Display**:
    - "Actual Position" and "Statusword" fields will be updated periodically if polling is active (after successful connection).
    - The plot widget is included in the UI, but real-time plotting is currently commented out due to the dummy QCustomPlot library.
6.  **Logging**: Status and error messages are displayed in the text area at the bottom of the window.

### Important Notes for Simulated Version:
- **CAN Communication is Simulated**: The application does not actually communicate over a CAN bus. All PCAN API calls are to dummy functions that simulate success or predefined error conditions. SDO message responses are also simulated.
- **Motor Behavior is Not Real**: The motor control commands will appear to succeed based on the simulation, but no actual motor will move or change state.
- **Plotting is Disabled**: The QCustomPlot features for drawing graphs are commented out. The plot widget will appear empty.

To use this application with real hardware, the dummy PCANBasic and QCustomPlot implementations must be replaced with the actual libraries, and the code using these libraries (especially plotting and potentially CAN channel/baud rate parsing) would need to be fully enabled and tested.

## 6. Deliverables

This directory (`CANAnalyzer/`) contains all source code, the qmake project file (`CANAnalyzer.pro`), and the dummy library files under `libs/` used for this development exercise.
The final executable `CANAnalyzer` (if built) is also part of the deliverable for demonstration of compilation.
This `README.md` file serves as documentation.
