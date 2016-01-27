#include "serverthread.h"

ServerThread::ServerThread(qintptr sd, QObject *parent) : QThread(parent),_socketDescriptor(sd)
{
    _mainWindow = (MainWindow*) parent;
}

void ServerThread::run()
{
    _tcpSocket = new QTcpSocket();

    qDebug () << "Setting socket descriptor " << _socketDescriptor;

    if (!_tcpSocket->setSocketDescriptor(_socketDescriptor))
    {
        qDebug() << "Cant set socket descriptor " << _socketDescriptor <<  " " << _tcpSocket->errorString();
        return;
    }

    connect(_tcpSocket, SIGNAL(readyRead()), this, SLOT(readSocket()), Qt::DirectConnection);
    connect(_tcpSocket, SIGNAL(disconnected()), this, SLOT(handleSocketDisconnect()), Qt::DirectConnection);

    exec();
}

void ServerThread::readSocket()
{


        QByteArray data = _tcpSocket->readAll();

        qDebug() << "Received: " << data;

        _tcpSocket->write(data);

        _tcpSocket->waitForBytesWritten();


}

void ServerThread::handleSocketDisconnect()
{
    if (_tcpSocket->isOpen())
    {

        qDebug() << "CLient Disconnect " <<  _tcpSocket->socketDescriptor();

        _tcpSocket->close();
        _tcpSocket->deleteLater();

        _mainWindow->clientDisconnected();

        this->exit();
        //this->deleteLater();
    }
}
