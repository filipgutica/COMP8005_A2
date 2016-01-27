#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QDebug>
#include "serverthread.h"
#include <QThreadPool>
#include <QVector>

#define MAX_THREADS 100

class ServerThread;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void handleClient(int);
    void clientConnected();
    void clientDisconnected();

signals:
    void valueChangedLog(QString);

private slots:
    void acceptConnection();
    void socketReady();
    void on_actionStartServer_triggered();

private:
    Ui::MainWindow *ui;
    QTcpServer *_tcpServ;
    QTcpSocket *_socket;
    int _numClients;
    QThreadPool *pool;
    int _currentThreadCount;


};

#endif // MAINWINDOW_H
