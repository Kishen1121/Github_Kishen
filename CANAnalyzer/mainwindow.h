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

#include "libs/qcustomplot/qcustomplot.h" // QCustomPlot header
#include "can_handler.h"
#include "motor_controller.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // CAN Connection Slots
    void connectCAN();
    void disconnectCAN(); // Optional, if a disconnect button is added

    // Motor Control Slots
    void enableMotor();
    void disableMotor();
    void setTargetPosition();

    // Polling/Update Slot
    void updateMotorData(); // For QTimer

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
    QLabel *m_lblTargetPosition;
    QLineEdit *m_txtTargetPosition;
    QPushButton *m_btnSetTargetPosition;

    // Display UI
    QLabel *m_lblActualPosition;
    QLineEdit *m_txtActualPosition; // Read-only display
    QLabel *m_lblStatusword;
    QLineEdit *m_txtStatusword;     // Read-only display

    // Plotting
    QCustomPlot *m_plot;

    // Logging
    QTextEdit *m_logTextEdit;

    // Timer for polling
    QTimer *m_pollingTimer;

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
};

#endif // MAINWINDOW_H
