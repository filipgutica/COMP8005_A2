#ifndef SERVERTHREAD_H
#define SERVERTHREAD_H

#include <QThread>
#include <QtNetwork>
#include <QDebug>
#include "mainwindow.h"
#include <QRunnable>

class MainWindow;

class ServerThread : public QThread
{
    Q_OBJECT
public:
    explicit ServerThread(qintptr sd, QObject *parent);
    void run();
    void setSocketDescriptor(qintptr);

private:
    int _socketDescriptor;
    QTcpSocket *_tcpSocket;
    MainWindow *_mainWindow;


signals:


public slots:
    void readSocket();
    void handleSocketDisconnect();

};

#endif // SERVERTHREAD_H
