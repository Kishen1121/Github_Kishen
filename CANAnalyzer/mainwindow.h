#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit> // For log messages
#include <QTimer>
#include <QTableWidget> // For QTableWidget

#include "libs/qcustomplot/qcustomplot.h" // QCustomPlot header
#include "can_handler.h"
#include "motor_controller.h"

// Define the structure for a single step in a motor control sequence
struct MotorSequenceStep {
    int32_t targetVelocity;
    uint32_t durationMs;
    uint32_t profileAcceleration; // Optional, can be set per step or globally
    uint32_t profileDeceleration; // Optional, can be set per step or globally

    // Default constructor for easy initialization if needed
    MotorSequenceStep(int32_t vel = 0, uint32_t dur = 1000, uint32_t acc = 1000, uint32_t dec = 1000)
        : targetVelocity(vel), durationMs(dur), profileAcceleration(acc), profileDeceleration(dec) {}
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // CAN Connection Slots
    void connectCAN();
    void disconnectCAN();

    // Motor Control Slots
    void enableMotor();
    void disableMotor();
    void setTargetPosition(); // This is for position mode, may need adjustment for velocity sequences

    // Polling/Update Slot
    void updateMotorData();

    // Sequence Control Slots
    void startSequence();
    void stopSequence();
    void onSequenceTimerTimeout();

    // Slots for new UI elements
    void onSetImmediateVelocityClicked();
    void onStopMotorClicked(); // For the new immediate stop button
    void onAddStepClicked();
    void onRemoveSelectedStepClicked();
    void onClearSequenceClicked();

private:
    void setupUI();
    void logMessage(const QString& message);

    // UI Elements
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QGridLayout *m_configLayout;
    QHBoxLayout *m_plotLayout; // For plot and potentially controls next to it

    // Configuration UI
    QLabel *m_lblCanChannel;
    QLineEdit *m_txtCanChannel; // e.g., "PCAN_USBBUS1" (string representation)
    QLabel *m_lblBaudRate;
    QLineEdit *m_txtBaudRate;   // e.g., "500" (for 500 kbit/s)
    QLabel *m_lblNodeId;
    QLineEdit *m_txtNodeId;
    QPushButton *m_btnConnect;
    // QPushButton *m_btnDisconnect; // Optional

    // Motor Control UI
    QPushButton *m_btnEnableMotor;
    QPushButton *m_btnDisableMotor;

    // Immediate Position Control (existing elements, re-categorized for clarity)
    QLabel *m_lblTargetPosition;      // For position mode
    QLineEdit *m_txtTargetPosition;   // For position mode
    QPushButton *m_btnSetTargetPosition; // For position mode

    // Immediate Velocity Control UI
    QLabel *m_lblImmediateVelocity;
    QLineEdit *m_txtImmediateVelocity;
    QLabel *m_lblImmediateAccel;
    QLineEdit *m_txtImmediateAccel;
    QLabel *m_lblImmediateDecel;
    QLineEdit *m_txtImmediateDecel;
    QPushButton *m_btnSetImmediateVelocity;
    QPushButton *m_btnStopMotorImmediate;

    // Display UI
    QLabel *m_lblActualPosition;
    QLineEdit *m_txtActualPosition; // Read-only display
    QLabel *m_lblStatusword;
    QLineEdit *m_txtStatusword;     // Read-only display

    // Plotting
    QCustomPlot *m_plot;

    // Sequence Definition UI
    QTableWidget *m_tblSequenceSteps;
    QLabel *m_lblStepVelocity;
    QLineEdit *m_txtStepVelocity;
    QLabel *m_lblStepDuration;
    QLineEdit *m_txtStepDuration;
    QLabel *m_lblStepAccel;
    QLineEdit *m_txtStepAccel;
    QLabel *m_lblStepDecel;
    QLineEdit *m_txtStepDecel;
    QPushButton *m_btnAddStep;
    QPushButton *m_btnRemoveStep;
    QPushButton *m_btnClearSequence;

    // Sequence Execution UI
    QPushButton *m_btnStartSequence;
    QPushButton *m_btnStopSequence;

    // Logging
    QTextEdit *m_logTextEdit;

    // Timer for polling motor status
    QTimer *m_pollingTimer;

    // Sequence related members
    QList<MotorSequenceStep> m_sequenceSteps;
    QTimer *m_sequenceTimer;
    int m_currentSequenceStepIndex;

    // Backend Logic
    CANHandler m_canHandler; // Direct instance for simplicity
    MotorController *m_motorController; // Pointer, initialized after CAN connection and Node ID is known

    // State variables
    bool m_isCANConnected;
    bool m_isMotorEnabled; // Basic state tracking

    // Plotting data storage
    QVector<double> m_timeData;
    QVector<double> m_positionData;
    double m_plotTimeValue;

    // Helper methods
    void executeCurrentSequenceStep();
    void updateSequenceTable(); // To refresh QTableWidget
};

#endif // MAINWINDOW_H
