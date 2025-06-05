#include "mainwindow.h" // Include the MainWindow header
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w; // Create an instance of our main window
    w.show();    // Show the main window
    return a.exec();
}
