#include "mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QTime>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout> // Ensure all layout headers are present
#include <QHBoxLayout>
#include <QGridLayout>
#include <QThread> // For msleep if used (though direct use in GUI thread is not ideal)

// Dummy PCAN channel details
const void* DEFAULT_PCAN_CHANNEL_PTR = (void*)0x51;
const unsigned short DEFAULT_PCAN_BAUDRATE_VAL = 0x001C;
const uint16_t DEFAULT_MOTOR_NODE_ID = 0x01;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_motorController(nullptr), m_isCANConnected(false), m_isMotorEnabled(false), m_plotTimeValue(0.0)
{
    setupUI(); // Call to setup all UI elements

    m_pollingTimer = new QTimer(this);
    connect(m_pollingTimer, &QTimer::timeout, this, &MainWindow::updateMotorData);

    m_sequenceTimer = new QTimer(this);
    connect(m_sequenceTimer, &QTimer::timeout, this, &MainWindow::onSequenceTimerTimeout);
    m_currentSequenceStepIndex = -1;

    // Remove hardcoded sequence - user will define it via GUI
    // m_sequenceSteps.append(MotorSequenceStep(1000, 2000, 500, 500));
    // ...

    // Set default values for input fields
    m_txtCanChannel->setText("PCAN_USBBUS1");
    m_txtBaudRate->setText("500");
    m_txtNodeId->setText(QString::number(DEFAULT_MOTOR_NODE_ID));
    m_txtTargetPosition->setText("0");
    m_txtImmediateVelocity->setText("0");
    m_txtImmediateAccel->setText("1000");
    m_txtImmediateDecel->setText("1000");
    m_txtStepVelocity->setText("0");
    m_txtStepDuration->setText("1000");
    m_txtStepAccel->setText("1000");
    m_txtStepDecel->setText("1000");

    logMessage("Application started. Please configure and connect to CAN.");
    updateSequenceTable(); // Initial empty display
}

MainWindow::~MainWindow()
{
    if (m_motorController) {
        delete m_motorController;
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("CANopen Motor Analyzer");
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    m_mainLayout = new QVBoxLayout(m_centralWidget);

    // --- Configuration Section ---
    QGroupBox *gbConfig = new QGroupBox("CAN Configuration", this);
    m_configLayout = new QGridLayout(); // m_configLayout is already a member
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
    m_configLayout->addWidget(m_btnConnect, 3, 0, 1, 2);
    gbConfig->setLayout(m_configLayout);
    m_mainLayout->addWidget(gbConfig);
    connect(m_btnConnect, &QPushButton::clicked, this, &MainWindow::connectCAN);

    // --- General Motor Controls ---
    QGroupBox *gbGeneralMotorControl = new QGroupBox("General Motor Control", this);
    QHBoxLayout *generalMotorControlLayout = new QHBoxLayout();
    m_btnEnableMotor = new QPushButton("Enable Motor", this);
    m_btnDisableMotor = new QPushButton("Disable Motor", this);
    generalMotorControlLayout->addWidget(m_btnEnableMotor);
    generalMotorControlLayout->addWidget(m_btnDisableMotor);
    generalMotorControlLayout->addStretch();
    gbGeneralMotorControl->setLayout(generalMotorControlLayout);
    m_mainLayout->addWidget(gbGeneralMotorControl);
    connect(m_btnEnableMotor, &QPushButton::clicked, this, &MainWindow::enableMotor);
    connect(m_btnDisableMotor, &QPushButton::clicked, this, &MainWindow::disableMotor);

    // --- Immediate Position Control ---
    QGroupBox *gbPositionControl = new QGroupBox("Immediate Position Control", this);
    QGridLayout *positionControlLayout = new QGridLayout();
    m_lblTargetPosition = new QLabel("Target Pos:", this);
    m_txtTargetPosition = new QLineEdit(this);
    m_btnSetTargetPosition = new QPushButton("Set Position", this);
    positionControlLayout->addWidget(m_lblTargetPosition, 0, 0);
    positionControlLayout->addWidget(m_txtTargetPosition, 0, 1);
    positionControlLayout->addWidget(m_btnSetTargetPosition, 0, 2);
    gbPositionControl->setLayout(positionControlLayout);
    m_mainLayout->addWidget(gbPositionControl);
    connect(m_btnSetTargetPosition, &QPushButton::clicked, this, &MainWindow::setTargetPosition);

    // --- Immediate Velocity Control ---
    QGroupBox *gbVelocityControl = new QGroupBox("Immediate Velocity Control", this);
    QGridLayout *velocityControlLayout = new QGridLayout();
    m_lblImmediateVelocity = new QLabel("Target Velocity:", this);
    m_txtImmediateVelocity = new QLineEdit(this);
    m_lblImmediateAccel = new QLabel("Acceleration:", this);
    m_txtImmediateAccel = new QLineEdit(this);
    m_lblImmediateDecel = new QLabel("Deceleration:", this);
    m_txtImmediateDecel = new QLineEdit(this);
    m_btnSetImmediateVelocity = new QPushButton("Set Velocity", this);
    m_btnStopMotorImmediate = new QPushButton("Stop Motor", this);
    velocityControlLayout->addWidget(m_lblImmediateVelocity, 0, 0);
    velocityControlLayout->addWidget(m_txtImmediateVelocity, 0, 1);
    velocityControlLayout->addWidget(m_btnSetImmediateVelocity, 0, 2);
    velocityControlLayout->addWidget(m_lblImmediateAccel, 1, 0);
    velocityControlLayout->addWidget(m_txtImmediateAccel, 1, 1);
    velocityControlLayout->addWidget(m_btnStopMotorImmediate, 1, 2);
    velocityControlLayout->addWidget(m_lblImmediateDecel, 2, 0);
    velocityControlLayout->addWidget(m_txtImmediateDecel, 2, 1);
    gbVelocityControl->setLayout(velocityControlLayout);
    m_mainLayout->addWidget(gbVelocityControl);
    connect(m_btnSetImmediateVelocity, &QPushButton::clicked, this, &MainWindow::onSetImmediateVelocityClicked);
    connect(m_btnStopMotorImmediate, &QPushButton::clicked, this, &MainWindow::onStopMotorClicked);

    // --- Sequence Definition ---
    QGroupBox *gbSequenceDef = new QGroupBox("Sequence Definition", this);
    QVBoxLayout *sequenceDefMainLayout = new QVBoxLayout();
    m_tblSequenceSteps = new QTableWidget(this);
    m_tblSequenceSteps->setColumnCount(4);
    m_tblSequenceSteps->setHorizontalHeaderLabels({"Target Velocity", "Duration (ms)", "Acceleration", "Deceleration"});
    m_tblSequenceSteps->horizontalHeader()->setStretchLastSection(true);
    m_tblSequenceSteps->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tblSequenceSteps->setSelectionMode(QAbstractItemView::SingleSelection);
    sequenceDefMainLayout->addWidget(m_tblSequenceSteps);
    QGridLayout *stepInputLayout = new QGridLayout();
    m_lblStepVelocity = new QLabel("Velocity:", this); m_txtStepVelocity = new QLineEdit(this);
    m_lblStepDuration = new QLabel("Duration (ms):", this); m_txtStepDuration = new QLineEdit(this);
    m_lblStepAccel = new QLabel("Accel:", this); m_txtStepAccel = new QLineEdit(this);
    m_lblStepDecel = new QLabel("Decel:", this); m_txtStepDecel = new QLineEdit(this);
    stepInputLayout->addWidget(m_lblStepVelocity, 0, 0); stepInputLayout->addWidget(m_txtStepVelocity, 0, 1);
    stepInputLayout->addWidget(m_lblStepDuration, 0, 2); stepInputLayout->addWidget(m_txtStepDuration, 0, 3);
    stepInputLayout->addWidget(m_lblStepAccel, 1, 0); stepInputLayout->addWidget(m_txtStepAccel, 1, 1);
    stepInputLayout->addWidget(m_lblStepDecel, 1, 2); stepInputLayout->addWidget(m_txtStepDecel, 1, 3);
    sequenceDefMainLayout->addLayout(stepInputLayout);
    QHBoxLayout *stepButtonsLayout = new QHBoxLayout();
    m_btnAddStep = new QPushButton("Add Step", this);
    m_btnRemoveStep = new QPushButton("Remove Selected Step", this);
    m_btnClearSequence = new QPushButton("Clear Sequence", this);
    stepButtonsLayout->addWidget(m_btnAddStep);
    stepButtonsLayout->addWidget(m_btnRemoveStep);
    stepButtonsLayout->addWidget(m_btnClearSequence);
    stepButtonsLayout->addStretch();
    sequenceDefMainLayout->addLayout(stepButtonsLayout);
    gbSequenceDef->setLayout(sequenceDefMainLayout);
    m_mainLayout->addWidget(gbSequenceDef);
    connect(m_btnAddStep, &QPushButton::clicked, this, &MainWindow::onAddStepClicked);
    connect(m_btnRemoveStep, &QPushButton::clicked, this, &MainWindow::onRemoveSelectedStepClicked);
    connect(m_btnClearSequence, &QPushButton::clicked, this, &MainWindow::onClearSequenceClicked);

    // --- Sequence Execution ---
    QGroupBox *gbSequenceExec = new QGroupBox("Sequence Execution", this);
    QHBoxLayout *sequenceExecLayout = new QHBoxLayout();
    m_btnStartSequence = new QPushButton("Start Sequence", this);
    m_btnStopSequence = new QPushButton("Stop Sequence", this);
    sequenceExecLayout->addWidget(m_btnStartSequence);
    sequenceExecLayout->addWidget(m_btnStopSequence);
    sequenceExecLayout->addStretch();
    gbSequenceExec->setLayout(sequenceExecLayout);
    m_mainLayout->addWidget(gbSequenceExec);
    connect(m_btnStartSequence, &QPushButton::clicked, this, &MainWindow::startSequence);
    connect(m_btnStopSequence, &QPushButton::clicked, this, &MainWindow::stopSequence);

    // --- Display Section ---
    QGridLayout *displayLayout = new QGridLayout();
    m_lblActualPosition = new QLabel("Actual Position:", this);
    m_txtActualPosition = new QLineEdit(this); m_txtActualPosition->setReadOnly(true);
    m_lblStatusword = new QLabel("Statusword:", this);
    m_txtStatusword = new QLineEdit(this); m_txtStatusword->setReadOnly(true);
    displayLayout->addWidget(m_lblActualPosition, 0, 0); displayLayout->addWidget(m_txtActualPosition, 0, 1);
    displayLayout->addWidget(m_lblStatusword, 1, 0); displayLayout->addWidget(m_txtStatusword, 1, 1);
    m_mainLayout->addLayout(displayLayout);

    // --- Plotting Section ---
    m_plotLayout = new QHBoxLayout();
    m_plot = new QCustomPlot(this);
    m_plotLayout->addWidget(m_plot);
    m_mainLayout->addLayout(m_plotLayout, 1);

    // --- Logging Section ---
    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    m_mainLayout->addWidget(m_logTextEdit, 1);

    // Initial state of controls
    m_btnEnableMotor->setEnabled(false); m_btnDisableMotor->setEnabled(false);
    m_txtTargetPosition->setEnabled(false); m_btnSetTargetPosition->setEnabled(false);
    m_txtImmediateVelocity->setEnabled(false); m_txtImmediateAccel->setEnabled(false);
    m_txtImmediateDecel->setEnabled(false); m_btnSetImmediateVelocity->setEnabled(false);
    m_btnStopMotorImmediate->setEnabled(false);
    m_tblSequenceSteps->setEnabled(false); m_txtStepVelocity->setEnabled(false);
    m_txtStepDuration->setEnabled(false); m_txtStepAccel->setEnabled(false);
    m_txtStepDecel->setEnabled(false); m_btnAddStep->setEnabled(false);
    m_btnRemoveStep->setEnabled(false); m_btnClearSequence->setEnabled(false);
    m_btnStartSequence->setEnabled(false); m_btnStopSequence->setEnabled(false);

    resize(800, 700); // Adjusted height for new sections
}

void MainWindow::logMessage(const QString& message) {
    m_logTextEdit->append(QTime::currentTime().toString("hh:mm:ss.zzz") + ": " + message);
    qDebug() << message;
}

void MainWindow::connectCAN() {
    if (m_isCANConnected) {
        disconnectCAN();
        return;
    }
    void* pcanChannel = const_cast<void*>(DEFAULT_PCAN_CHANNEL_PTR);
    unsigned short baudRateVal = DEFAULT_PCAN_BAUDRATE_VAL;
    uint16_t nodeId = m_txtNodeId->text().toUShort();
    if (nodeId == 0) { /* ... error handling ... */ return; }

    logMessage(QString("Attempting to connect..."));
    if (m_canHandler.connectDevice(pcanChannel, baudRateVal)) {
        m_isCANConnected = true;
        logMessage("CAN connection successful.");
        if(m_motorController) delete m_motorController;
        m_motorController = new MotorController(m_canHandler, nodeId);
        logMessage(QString("MotorController initialized for Node ID %1.").arg(nodeId));

        m_btnConnect->setText("Disconnect from CAN");
        m_txtCanChannel->setEnabled(false); m_txtBaudRate->setEnabled(false); m_txtNodeId->setEnabled(false);
        m_btnEnableMotor->setEnabled(true); m_btnDisableMotor->setEnabled(true);
        m_txtTargetPosition->setEnabled(true); m_btnSetTargetPosition->setEnabled(true);
        m_txtImmediateVelocity->setEnabled(true); m_txtImmediateAccel->setEnabled(true);
        m_txtImmediateDecel->setEnabled(true); m_btnSetImmediateVelocity->setEnabled(true);
        m_btnStopMotorImmediate->setEnabled(true);
        m_tblSequenceSteps->setEnabled(true); m_txtStepVelocity->setEnabled(true);
        m_txtStepDuration->setEnabled(true); m_txtStepAccel->setEnabled(true);
        m_txtStepDecel->setEnabled(true); m_btnAddStep->setEnabled(true);
        m_btnRemoveStep->setEnabled(true); m_btnClearSequence->setEnabled(true);
        m_btnStartSequence->setEnabled(true); m_btnStopSequence->setEnabled(true);

        m_pollingTimer->start(100);
        m_plotTimeValue = 0; m_timeData.clear(); m_positionData.clear();
        // Plotting calls remain commented
    } else {
        m_isCANConnected = false;
        QMessageBox::critical(this, "Connection Error", QString::fromStdString(m_canHandler.getLastError()));
        logMessage("CAN connection failed: " + QString::fromStdString(m_canHandler.getLastError()));
    }
}

void MainWindow::disconnectCAN() {
    if (!m_isCANConnected) return;
    m_pollingTimer->stop();
    if (m_sequenceTimer->isActive()) stopSequence(); // Stop sequence if running
    m_canHandler.disconnectDevice();
    m_isCANConnected = false; m_isMotorEnabled = false;
    logMessage("Disconnected from CAN.");
    if(m_motorController) { delete m_motorController; m_motorController = nullptr; }

    m_btnConnect->setText("Connect to CAN");
    m_txtCanChannel->setEnabled(true); m_txtBaudRate->setEnabled(true); m_txtNodeId->setEnabled(true);
    m_btnEnableMotor->setEnabled(false); m_btnDisableMotor->setEnabled(false);
    m_txtTargetPosition->setEnabled(false); m_btnSetTargetPosition->setEnabled(false);
    m_txtImmediateVelocity->setEnabled(false); m_txtImmediateAccel->setEnabled(false);
    m_txtImmediateDecel->setEnabled(false); m_btnSetImmediateVelocity->setEnabled(false);
    m_btnStopMotorImmediate->setEnabled(false);
    m_tblSequenceSteps->setEnabled(false); m_txtStepVelocity->setEnabled(false);
    m_txtStepDuration->setEnabled(false); m_txtStepAccel->setEnabled(false);
    m_txtStepDecel->setEnabled(false); m_btnAddStep->setEnabled(false);
    m_btnRemoveStep->setEnabled(false); m_btnClearSequence->setEnabled(false);
    m_btnStartSequence->setEnabled(false); m_btnStopSequence->setEnabled(false);
    m_txtActualPosition->clear(); m_txtStatusword->clear();
}

void MainWindow::enableMotor() {
    if (!m_isCANConnected || !m_motorController) { /* ... */ return; }
    logMessage("Attempting to enable motor...");
    if (m_motorController->enableMotor()) {
        logMessage("Motor enable sequence sent. " + QString::fromStdString(m_motorController->getLastError()));
        m_isMotorEnabled = true;
    } else { /* ... error handling ... */ m_isMotorEnabled = false; }
}

void MainWindow::disableMotor() {
    if (!m_isCANConnected || !m_motorController) { /* ... */ return; }
    logMessage("Attempting to disable motor...");
    if (m_motorController->disableMotor()) {
        logMessage("Motor disable sequence sent. " + QString::fromStdString(m_motorController->getLastError()));
        m_isMotorEnabled = false;
    } else { /* ... error handling ... */ }
}

void MainWindow::setTargetPosition() {
    if (!m_isCANConnected || !m_motorController) { /* ... */ return; }
    bool ok; int32_t targetPos = m_txtTargetPosition->text().toInt(&ok);
    if (!ok) { /* ... error handling ... */ return; }
    logMessage(QString("Setting target position to %1...").arg(targetPos));
    if (m_motorController->setTargetPosition(targetPos)) { /* ... */ } else { /* ... */ }
}

void MainWindow::updateMotorData() {
    if (!m_isCANConnected || !m_motorController) return;
    int32_t actualPos = 0; uint16_t statusword = 0;
    bool posOk = m_motorController->readActualPosition(actualPos);
    bool statusOk = m_motorController->readStatusword(statusword);
    if (posOk) {
        m_txtActualPosition->setText(QString::number(actualPos));
        m_timeData.append(m_plotTimeValue); m_positionData.append(actualPos);
    }
    if (statusOk) {
        m_txtStatusword->setText("0x" + QString::number(statusword, 16).toUpper().rightJustified(4, '0'));
    }
    if (posOk) { m_plotTimeValue += 0.1; /* Plotting calls commented */ }
}

// --- Sequence Control Methods ---
void MainWindow::startSequence() {
    if (!m_isCANConnected || !m_motorController) { /* ... error ... */ return; }
    if (m_sequenceSteps.isEmpty()) { /* ... info ... */ return; }
    if (!m_isMotorEnabled) {
        logMessage("Warning: Motor not enabled. Attempting to enable for sequence...");
        enableMotor();
        uint16_t tempStatusword = 0;
        if(!m_isMotorEnabled && !m_motorController->readStatusword(tempStatusword)) {
            logMessage("Motor still not confirmed enabled. Status: " + QString::number(tempStatusword, 16));
            QMessageBox::warning(this, "Sequence Error", "Motor could not be enabled.");
            return;
        }
    }
    m_currentSequenceStepIndex = 0;
    logMessage("Starting motor sequence...");
    executeCurrentSequenceStep();
}

void MainWindow::stopSequence() {
    m_sequenceTimer->stop();
    m_currentSequenceStepIndex = -1;
    logMessage("Sequence stopped by user.");
    if (m_motorController && m_isCANConnected) {
        logMessage("Setting target velocity to 0 as part of stop sequence.");
        if (!m_motorController->setTargetVelocity(0)) { /* ... warning ... */ }
    }
}

void MainWindow::executeCurrentSequenceStep() {
    if (m_currentSequenceStepIndex < 0 || m_currentSequenceStepIndex >= m_sequenceSteps.size()) {
        logMessage("Sequence finished or invalid step index.");
        if (m_motorController && m_isCANConnected) {
            if (!m_motorController->setTargetVelocity(0)) { /* ... warning ... */ }
        }
        m_sequenceTimer->stop(); return;
    }
    const MotorSequenceStep& currentStep = m_sequenceSteps.at(m_currentSequenceStepIndex);
    logMessage(QString("Executing step %1: Vel=%2, Dur=%3ms, Accel=%4, Decel=%5")
                   .arg(m_currentSequenceStepIndex + 1).arg(currentStep.targetVelocity)
                   .arg(currentStep.durationMs).arg(currentStep.profileAcceleration)
                   .arg(currentStep.profileDeceleration));
    bool stepSuccess = true;
    if (m_motorController) {
        if (!m_motorController->setProfileAcceleration(currentStep.profileAcceleration)) { /* log, stepSuccess = false; */ }
        if (stepSuccess && !m_motorController->setProfileDeceleration(currentStep.profileDeceleration)) { /* log, stepSuccess = false; */ }
        if (stepSuccess && !m_motorController->setTargetVelocity(currentStep.targetVelocity)) { /* log, stepSuccess = false; */ }
    } else { /* log, stepSuccess = false; */ }
    if (stepSuccess) { m_sequenceTimer->start(currentStep.durationMs); }
    else { /* log, QMessageBox, stopSequence(); */ }
}

void MainWindow::onSequenceTimerTimeout() {
    logMessage(QString("Step %1 duration finished.").arg(m_currentSequenceStepIndex + 1));
    m_currentSequenceStepIndex++;
    executeCurrentSequenceStep();
}

// --- Slots for new UI elements ---
void MainWindow::onSetImmediateVelocityClicked() {
    if (!m_isCANConnected || !m_motorController) { /* ... error ... */ return; }
    bool velOk, accelOk, decelOk;
    int32_t velocity = m_txtImmediateVelocity->text().toInt(&velOk);
    uint32_t accel = m_txtImmediateAccel->text().toUInt(&accelOk);
    uint32_t decel = m_txtImmediateDecel->text().toUInt(&decelOk);

    if (!velOk || !accelOk || !decelOk) {
        QMessageBox::warning(this, "Input Error", "Invalid velocity/acceleration/deceleration value.");
        logMessage("Error: Invalid input for immediate velocity control."); return;
    }
    logMessage(QString("Setting immediate velocity: Vel=%1, Accel=%2, Decel=%3").arg(velocity).arg(accel).arg(decel));
    if (!m_motorController->setOperationMode(MotorSdoObjects::MODE_PROFILE_VELOCITY)) { /* log error */ return; }
    if (!m_motorController->setProfileAcceleration(accel)) { /* log error */ return; }
    if (!m_motorController->setProfileDeceleration(decel)) { /* log error */ return; }
    if (!m_motorController->setTargetVelocity(velocity)) { /* log error */ }
}

void MainWindow::onStopMotorClicked() {
    if (!m_isCANConnected || !m_motorController) { /* ... error ... */ return; }
    logMessage("Stop Motor button clicked. Setting target velocity to 0.");
    if (!m_motorController->setTargetVelocity(0)) { // Or a more specific halt SDO if available
        logMessage("Error setting target velocity to 0: " + QString::fromStdString(m_motorController->getLastError()));
        QMessageBox::warning(this, "Motor Error", "Failed to stop motor (set vel to 0).");
    }
}

void MainWindow::onAddStepClicked() {
    bool velOk, durOk, accelOk, decelOk;
    int32_t velocity = m_txtStepVelocity->text().toInt(&velOk);
    uint32_t duration = m_txtStepDuration->text().toUInt(&durOk);
    uint32_t accel = m_txtStepAccel->text().toUInt(&accelOk);
    uint32_t decel = m_txtStepDecel->text().toUInt(&decelOk);

    if (!velOk || !durOk || !accelOk || !decelOk || duration == 0) {
        QMessageBox::warning(this, "Input Error", "Invalid sequence step value(s). Duration must be > 0.");
        logMessage("Error: Invalid input for sequence step."); return;
    }
    m_sequenceSteps.append(MotorSequenceStep(velocity, duration, accel, decel));
    updateSequenceTable();
    logMessage("Added step to sequence.");
}

void MainWindow::onRemoveSelectedStepClicked() {
    int currentRow = m_tblSequenceSteps->currentRow();
    if (currentRow >= 0 && currentRow < m_sequenceSteps.size()) {
        m_sequenceSteps.removeAt(currentRow);
        updateSequenceTable();
        logMessage(QString("Removed step %1 from sequence.").arg(currentRow + 1));
    } else {
        QMessageBox::information(this, "Remove Step", "No step selected or invalid selection.");
        logMessage("No step selected for removal or selection out of bounds.");
    }
}

void MainWindow::onClearSequenceClicked() {
    if (QMessageBox::question(this, "Clear Sequence", "Are you sure you want to clear all steps?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_sequenceSteps.clear();
        updateSequenceTable();
        logMessage("Sequence cleared.");
    }
}

void MainWindow::updateSequenceTable() {
    m_tblSequenceSteps->setRowCount(0); // Clear existing rows
    for (int i = 0; i < m_sequenceSteps.size(); ++i) {
        const MotorSequenceStep& step = m_sequenceSteps.at(i);
        m_tblSequenceSteps->insertRow(i);
        m_tblSequenceSteps->setItem(i, 0, new QTableWidgetItem(QString::number(step.targetVelocity)));
        m_tblSequenceSteps->setItem(i, 1, new QTableWidgetItem(QString::number(step.durationMs)));
        m_tblSequenceSteps->setItem(i, 2, new QTableWidgetItem(QString::number(step.profileAcceleration)));
        m_tblSequenceSteps->setItem(i, 3, new QTableWidgetItem(QString::number(step.profileDeceleration)));
    }
}
