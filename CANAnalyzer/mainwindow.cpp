#include "mainwindow.h"
#include <QMessageBox> // For error popups
#include <QDebug>      // For console debugging output
#include <QTime>       // For QTime::currentTime()

// Dummy PCAN channel details (matching main.cpp for now, but should be user input)
const void* DEFAULT_PCAN_CHANNEL_PTR = (void*)0x51; // Placeholder for PCAN_USBBUS1
const unsigned short DEFAULT_PCAN_BAUDRATE_VAL = 0x001C; // Placeholder for PCAN_BAUD_500K
const uint16_t DEFAULT_MOTOR_NODE_ID = 0x01;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_motorController(nullptr), m_isCANConnected(false), m_isMotorEnabled(false), m_plotTimeValue(0.0)
{
    setupUI();

    // Initialize m_motorController to nullptr, it will be created on successful CAN connection
    m_pollingTimer = new QTimer(this);
    connect(m_pollingTimer, &QTimer::timeout, this, &MainWindow::updateMotorData);

    // Set default values for input fields
    m_txtCanChannel->setText("PCAN_USBBUS1"); // String representation
    m_txtBaudRate->setText("500"); // kbit/s
    m_txtNodeId->setText(QString::number(DEFAULT_MOTOR_NODE_ID));
    m_txtTargetPosition->setText("0");

    logMessage("Application started. Please configure and connect to CAN.");
}

MainWindow::~MainWindow()
{
    if (m_motorController) {
        delete m_motorController;
    }
    // Qt handles deletion of child widgets
}

void MainWindow::setupUI()
{
    setWindowTitle("CANopen Motor Analyzer");
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QVBoxLayout(m_centralWidget);

    // --- Configuration Section ---
    m_configLayout = new QGridLayout();
    m_lblCanChannel = new QLabel("CAN Channel:", this);
    m_txtCanChannel = new QLineEdit(this);
    m_lblBaudRate = new QLabel("Baud Rate (kbit/s):", this);
    m_txtBaudRate = new QLineEdit(this);
    m_lblNodeId = new QLabel("Motor Node ID:", this);
    m_txtNodeId = new QLineEdit(this);
    m_btnConnect = new QPushButton("Connect to CAN", this);

    m_configLayout->addWidget(m_lblCanChannel, 0, 0);
    m_configLayout->addWidget(m_txtCanChannel, 0, 1);
    m_configLayout->addWidget(m_lblBaudRate, 1, 0);
    m_configLayout->addWidget(m_txtBaudRate, 1, 1);
    m_configLayout->addWidget(m_lblNodeId, 2, 0);
    m_configLayout->addWidget(m_txtNodeId, 2, 1);
    m_configLayout->addWidget(m_btnConnect, 3, 0, 1, 2); // Span 2 columns

    m_mainLayout->addLayout(m_configLayout);
    connect(m_btnConnect, &QPushButton::clicked, this, &MainWindow::connectCAN);

    // --- Motor Control Section ---
    QHBoxLayout *motorControlLayout = new QHBoxLayout();
    m_btnEnableMotor = new QPushButton("Enable Motor", this);
    m_btnDisableMotor = new QPushButton("Disable Motor", this);
    m_lblTargetPosition = new QLabel("Target Pos:", this);
    m_txtTargetPosition = new QLineEdit(this);
    m_btnSetTargetPosition = new QPushButton("Set Position", this);

    motorControlLayout->addWidget(m_btnEnableMotor);
    motorControlLayout->addWidget(m_btnDisableMotor);
    motorControlLayout->addWidget(m_lblTargetPosition);
    motorControlLayout->addWidget(m_txtTargetPosition);
    motorControlLayout->addWidget(m_btnSetTargetPosition);
    m_mainLayout->addLayout(motorControlLayout);

    connect(m_btnEnableMotor, &QPushButton::clicked, this, &MainWindow::enableMotor);
    connect(m_btnDisableMotor, &QPushButton::clicked, this, &MainWindow::disableMotor);
    connect(m_btnSetTargetPosition, &QPushButton::clicked, this, &MainWindow::setTargetPosition);

    // Initially disable motor controls until CAN is connected
    m_btnEnableMotor->setEnabled(false);
    m_btnDisableMotor->setEnabled(false);
    m_txtTargetPosition->setEnabled(false);
    m_btnSetTargetPosition->setEnabled(false);

    // --- Display Section ---
    QGridLayout *displayLayout = new QGridLayout();
    m_lblActualPosition = new QLabel("Actual Position:", this);
    m_txtActualPosition = new QLineEdit(this);
    m_txtActualPosition->setReadOnly(true);
    m_lblStatusword = new QLabel("Statusword:", this);
    m_txtStatusword = new QLineEdit(this);
    m_txtStatusword->setReadOnly(true);

    displayLayout->addWidget(m_lblActualPosition, 0, 0);
    displayLayout->addWidget(m_txtActualPosition, 0, 1);
    displayLayout->addWidget(m_lblStatusword, 1, 0);
    displayLayout->addWidget(m_txtStatusword, 1, 1);
    m_mainLayout->addLayout(displayLayout);

    // --- Plotting Section ---
    m_plotLayout = new QHBoxLayout();
    m_plot = new QCustomPlot(this); // This will use the dummy QCustomPlot
    // The following lines require the full QCustomPlot library, so we comment them out for now.
    // m_plot->addGraph(); // Graph for actual position
    // m_plot->graph(0)->setPen(QPen(Qt::blue));
    // m_plot->xAxis->setLabel("Time (s)");
    // m_plot->yAxis->setLabel("Position");
    // m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    m_plotLayout->addWidget(m_plot);
    m_mainLayout->addLayout(m_plotLayout, 1); // Give plot more stretch factor

    // --- Logging Section ---
    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    m_mainLayout->addWidget(m_logTextEdit, 1); // Give log some stretch factor too

    resize(800, 600); // Set initial window size
}

void MainWindow::logMessage(const QString& message) {
    m_logTextEdit->append(QTime::currentTime().toString("hh:mm:ss.zzz") + ": " + message);
    qDebug() << message; // Also output to console for debugging during development
}

void MainWindow::connectCAN() {
    if (m_isCANConnected) {
        // Handle disconnect logic if button is dual-purpose or add a separate disconnect button
        disconnectCAN();
        return;
    }

    // These would normally be parsed from m_txtCanChannel and m_txtBaudRate
    // For dummy, using predefined values.
    // TODO: Implement parsing for channel string and baud rate value.
    void* pcanChannel = const_cast<void*>(DEFAULT_PCAN_CHANNEL_PTR);
    unsigned short baudRateVal = DEFAULT_PCAN_BAUDRATE_VAL;
    uint16_t nodeId = m_txtNodeId->text().toUShort();

    if (nodeId == 0) {
        QMessageBox::warning(this, "Connection Error", "Invalid Node ID.");
        logMessage("Error: Invalid Node ID specified.");
        return;
    }

    logMessage(QString("Attempting to connect: Channel (simulated %1), Baudrate (simulated %2 kbit/s), Node ID %3")
        .arg(m_txtCanChannel->text()).arg(m_txtBaudRate->text()).arg(nodeId));

    if (m_canHandler.connectDevice(pcanChannel, baudRateVal)) {
        m_isCANConnected = true;
        logMessage("CAN connection successful. " + QString::fromStdString(m_canHandler.getLastError()));

        if(m_motorController) delete m_motorController;
        m_motorController = new MotorController(m_canHandler, nodeId);
        logMessage(QString("MotorController initialized for Node ID %1.").arg(nodeId));

        m_btnConnect->setText("Disconnect from CAN");
        m_txtCanChannel->setEnabled(false);
        m_txtBaudRate->setEnabled(false);
        m_txtNodeId->setEnabled(false);

        m_btnEnableMotor->setEnabled(true);
        m_btnDisableMotor->setEnabled(true); // Enable disable button
        m_txtTargetPosition->setEnabled(true);
        m_btnSetTargetPosition->setEnabled(true);

        m_pollingTimer->start(100); // Poll every 100ms
        m_plotTimeValue = 0; // Reset plot time
        m_timeData.clear();
        m_positionData.clear();
        // m_plot->graph(0)->setData(m_timeData, m_positionData);
        // m_plot->replot();


    } else {
        m_isCANConnected = false;
        QMessageBox::critical(this, "Connection Error", QString::fromStdString(m_canHandler.getLastError()));
        logMessage("CAN connection failed: " + QString::fromStdString(m_canHandler.getLastError()));
    }
}

void MainWindow::disconnectCAN() {
    if (!m_isCANConnected) return;

    m_pollingTimer->stop();
    m_canHandler.disconnectDevice();
    m_isCANConnected = false;
    m_isMotorEnabled = false; // Reset motor state on disconnect
    logMessage("Disconnected from CAN. " + QString::fromStdString(m_canHandler.getLastError()));

    if(m_motorController) {
        delete m_motorController;
        m_motorController = nullptr;
    }

    m_btnConnect->setText("Connect to CAN");
    m_txtCanChannel->setEnabled(true);
    m_txtBaudRate->setEnabled(true);
    m_txtNodeId->setEnabled(true);

    m_btnEnableMotor->setEnabled(false);
    m_btnDisableMotor->setEnabled(false);
    m_txtTargetPosition->setEnabled(false);
    m_btnSetTargetPosition->setEnabled(false);
    m_txtActualPosition->clear();
    m_txtStatusword->clear();
}


void MainWindow::enableMotor() {
    if (!m_isCANConnected || !m_motorController) {
        logMessage("Error: Not connected to CAN or motor controller not initialized.");
        return;
    }
    logMessage("Attempting to enable motor...");
    if (m_motorController->enableMotor()) {
        logMessage("Motor enable sequence sent. " + QString::fromStdString(m_motorController->getLastError()));
        m_isMotorEnabled = true;
        // Further state checks via statusword would be good here
    } else {
        logMessage("Failed to enable motor. " + QString::fromStdString(m_motorController->getLastError()));
        QMessageBox::warning(this, "Motor Error", "Failed to enable motor: " + QString::fromStdString(m_motorController->getLastError()));
        m_isMotorEnabled = false;
    }
}

void MainWindow::disableMotor() {
    if (!m_isCANConnected || !m_motorController) {
        logMessage("Error: Not connected to CAN or motor controller not initialized.");
        return;
    }
    logMessage("Attempting to disable motor...");
    if (m_motorController->disableMotor()) {
        logMessage("Motor disable sequence sent. " + QString::fromStdString(m_motorController->getLastError()));
        m_isMotorEnabled = false;
    } else {
        logMessage("Failed to disable motor. " + QString::fromStdString(m_motorController->getLastError()));
        QMessageBox::warning(this, "Motor Error", "Failed to disable motor: " + QString::fromStdString(m_motorController->getLastError()));
    }
}

void MainWindow::setTargetPosition() {
    if (!m_isCANConnected || !m_motorController) {
        logMessage("Error: Not connected to CAN or motor controller not initialized.");
        return;
    }
    bool ok;
    int32_t targetPos = m_txtTargetPosition->text().toInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Input Error", "Invalid target position.");
        logMessage("Error: Invalid target position input.");
        return;
    }

    logMessage(QString("Setting target position to %1...").arg(targetPos));
    if (m_motorController->setTargetPosition(targetPos)) {
        logMessage("Target position set successfully. " + QString::fromStdString(m_motorController->getLastError()));
    } else {
        logMessage("Failed to set target position. " + QString::fromStdString(m_motorController->getLastError()));
        QMessageBox::warning(this, "Motor Error", "Failed to set target position: " + QString::fromStdString(m_motorController->getLastError()));
    }
}

void MainWindow::updateMotorData() {
    if (!m_isCANConnected || !m_motorController) {
        return; // Should not happen if timer is managed correctly
    }

    int32_t actualPos = 0;
    uint16_t statusword = 0;
    bool posOk = m_motorController->readActualPosition(actualPos);
    bool statusOk = m_motorController->readStatusword(statusword);

    if (posOk) {
        m_txtActualPosition->setText(QString::number(actualPos));
        // Add data to plot
        m_timeData.append(m_plotTimeValue);
        m_positionData.append(actualPos);
    } else {
        // Could show an error or "N/A" in the text field
        // logMessage("Poll: Failed to read actual position. " + QString::fromStdString(m_motorController->getLastError()));
    }

    if (statusOk) {
        m_txtStatusword->setText("0x" + QString::number(statusword, 16).toUpper().rightJustified(4, '0'));
    } else {
        // logMessage("Poll: Failed to read statusword. " + QString::fromStdString(m_motorController->getLastError()));
    }

    // Plotting logic
    if (posOk) { // Only advance time if position was read
        // The following lines require the full QCustomPlot library
        // m_plot->graph(0)->setData(m_timeData, m_positionData);
        // m_plot->rescaleAxes(); // Rescale axes to fit data
        // if (m_timeData.size() > 200) { // Keep a fixed window of N points
        //      m_plot->xAxis->setRange(m_timeData.last() - 20.0, m_timeData.last()); // Show last 20 seconds (assuming 100ms poll)
        // } else {
        //      m_plot->xAxis->setRange(0, m_plotTimeValue);
        // }
        // m_plot->replot();
        m_plotTimeValue += 0.1; // Increment time by polling interval (100ms = 0.1s)
    }
}
