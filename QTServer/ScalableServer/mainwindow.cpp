#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _tcpServ = new QTcpServer(this);

    _numClients = 0;

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
    QTcpSocket *clientConnection = _tcpServ->nextPendingConnection();

    QString strInfo = QString("Accepted connection from %1 Socket Desc: %2").arg(clientConnection->peerAddress().toString()).arg(clientConnection->socketDescriptor());

    clientConnected();

    //qDebug() << strInfo;

    ServerThread *thrd = new ServerThread(clientConnection->socketDescriptor(), this);

    connect(thrd, SIGNAL(finished()), thrd, SLOT(deleteLater()));


    thrd->start();
   // clientConnection->close();
   // clientConnection->deleteLater();

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

void MainWindow::socketReady()
{

}
