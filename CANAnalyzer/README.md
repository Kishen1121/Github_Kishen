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

### 3.2. Build-time Dependencies
- **C++ Compiler**: Supporting C++11 (e.g., g++).
- **Qt5 Development Tools**: `qt5-qmake`, `qtbase5-dev`.
- **PEAK PCAN-Basic SDK**: Header (`PCANBasic.h`) and library.
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

These instructions assume a Linux environment with g++ and Qt5 development tools installed.

### 5.1. Install Dependencies (Ubuntu Example)
```bash
sudo apt-get update
sudo apt-get install -y build-essential qt5-qmake qtbase5-dev
```

### 5.2. Obtain Libraries (for a real deployment)
- **PEAK PCAN-Basic SDK**: Download from [PEAK-System website](https://www.peak-system.com). Place `PCANBasic.h` into `libs/pcanbasic/` and the library file into a system path or project-local path linked in `CANAnalyzer.pro`.
- **QCustomPlot**: Download from [QCustomPlot website](https://www.qcustomplot.com). Replace the dummy files in `libs/qcustomplot/` with the full source files.

*Note: For this simulated project, the dummy library files are already included.*

### 5.3. Build the Application
1.  Navigate to the `CANAnalyzer` project directory.
2.  Run `qmake CANAnalyzer.pro` to generate the Makefile.
3.  Run `make` to compile. (Use `make clean` to remove old build artifacts if needed).
4.  The executable `CANAnalyzer` will be created in the project directory (or a build-specific directory depending on Qt configuration).

## 6. Usage Guidelines

1.  **Run the Application**: Execute `./CANAnalyzer`.
2.  **Configuration**:
    - **CAN Channel**: GUI field is present (default `PCAN_USBBUS1`). For this simulated version, the actual connection logic uses a hardcoded channel value.
    - **Baud Rate**: GUI field is present (default `500` kbit/s). For this simulated version, the actual connection logic uses a hardcoded baud rate value.
    - **Motor Node ID**: Enter the CANopen Node ID for the motor (e.g., `1`). This value is used.
3.  **Connect**: Click "Connect to CAN". Logs will confirm connection to the (simulated) CAN bus and initialization of the motor controller.
4.  **General Motor Operations**:
    - **Enable Motor**: Click to send commands to enable the motor drive. This prepares the motor for operation and by default, sets the Mode of Operation to Profile Velocity.
    - **Disable Motor**: Click to disable the motor drive.
5.  **Immediate Position Control**:
    - (Ensure motor is enabled. Note: `Enable Motor` defaults to Profile Velocity. Manual mode setting to Profile Position for this control section is not yet a distinct GUI feature).
    - Enter desired position in "Target Pos:" field.
    - Click "Set Position".
6.  **Immediate Velocity Control**:
    - (Ensure motor is enabled. This section implicitly uses Profile Velocity Mode as `Enable Motor` sets it, and "Set Velocity" button also ensures it).
    - Enter "Target Velocity", "Acceleration", and "Deceleration" values.
    - Click "Set Velocity" to command the motor.
    - Click "Stop Motor" to ramp down the motor to zero velocity.
7.  **Sequence Definition and Execution**:
    - **Define Steps**:
        - In the "Sequence Definition" section, enter values for "Velocity", "Duration (ms)", "Accel", and "Decel".
        - Click "Add Step to Sequence".
    - **Manage Sequence**:
        - Use the table and "Remove Selected Step" / "Clear Sequence" buttons.
    - **Execute Sequence**:
        - (Ensure motor is enabled).
        - Click "Run Sequence" and "Stop Sequence" as needed.
8.  **Data Display**:
    - "Actual Position" and "Statusword" fields show periodically polled (simulated) data.
    - The plot widget is present but currently does not display data.
9.  **Logging**: All operations and (simulated) SDO command details are logged in the text area.

### Important Notes for Simulated Version:
- **CAN Communication is Simulated**: The application does not actually communicate over a CAN bus. `CANHandler` simulates SDO responses, including read-backs of some written values and basic SDO aborts.
- **Motor Behavior is Not Real**: Commands will appear to succeed based on the simulation, but no physical motor will move.
- **Plotting is Disabled**: QCustomPlot features for graph drawing are commented out.

To use this application with real hardware, the dummy PCANBasic and QCustomPlot implementations must be replaced with the actual libraries. The CAN channel and baud rate handling in `MainWindow::connectCAN()` would also need to be modified to parse and use the values from the GUI input fields instead of the current hardcoded dummy values.

## 7. Deliverables (Previously Section 6)

This directory (`CANAnalyzer/`) contains all source code, the qmake project file (`CANAnalyzer.pro`), and the dummy library files under `libs/` used for this development exercise.
The final executable `CANAnalyzer` (if built) is also part of the deliverable for demonstration of compilation.
This `README.md` file serves as documentation.
