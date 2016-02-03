#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFuture>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _tcpServ = new QTcpServer(this);

    _numClients = 0;
    QThreadPool::globalInstance()->setMaxThreadCount(10000);
    connect(_tcpServ, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
    connect(this, SIGNAL(valueChangedLog(QString)), ui->lblNumClnts, SLOT(setText(QString)), Qt::DirectConnection);
}

MainWindow::~MainWindow()
{
    _tcpServ->close();

    delete ui;
}

void MainWindow::acceptConnection()
{
    _socket = _tcpServ->nextPendingConnection();

    QString strInfo = QString("Accepted connection from %1 Socket Desc: %2").arg(_socket->peerAddress().toString()).arg(_socket->socketDescriptor());

    clientConnected();



   connect(_socket, SIGNAL(readyRead()), this, SLOT(readSocket()), Qt::DirectConnection);
    connect(_socket, SIGNAL(disconnected()), this, SLOT(handleSocketDisconnect()), Qt::DirectConnection);

    //qDebug() << strInfo;

   // ServerThread *thrd = new ServerThread(_socket->socketDescriptor(), this);

   // connect(thrd, SIGNAL(finished()), thrd, SLOT(deleteLater()));

    //thrd->start();

}

void MainWindow::on_actionStartServer_triggered()
{
    if (!_tcpServ->listen(QHostAddress::Any, 7000))
    {
        QMessageBox::critical(this, tr("Server"),
                                    tr("Unable to start the server: %1")
                                    .arg(_tcpServ->errorString()));
    }

    QString strInfo = QString("Server listening on port: %1").arg(7000);
    //appendToLog(strInfo);
}


void MainWindow::clientConnected()
{
    _numClients++;


    emit valueChangedLog(QString::number(_numClients));
}
void MainWindow::clientDisconnected()
{
    _numClients--;

    emit valueChangedLog(QString::number(_numClients));

}

void MainWindow::readSocket()
{
    //run(readThrd, _socket->socketDescriptor());

  //  _socket->waitForBytesWritten();

    QByteArray data = _socket->readAll();

  //  qDebug() << "Received: " << data;

   _socket->write(data);

   // socket.waitForBytesWritten();

}

void MainWindow::handleSocketDisconnect()
{
    if (_socket->isOpen())
    {

        qDebug() << "CLient Disconnect " <<  _socket->socketDescriptor();

        _socket->close();

        clientDisconnected();

    }
}

void readThrd(qintptr sd)
{
    QTcpSocket socket;
    socket.setSocketDescriptor(sd);

    QByteArray data = socket.readAll();

    qDebug() << "Received: " << data;

    socket.write(data);

   // socket.waitForBytesWritten();

}
