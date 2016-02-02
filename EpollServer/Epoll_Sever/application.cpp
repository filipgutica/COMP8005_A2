#include "application.h"
#include "ui_application.h"


Application::Application(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Application)
{
    ui->setupUi(this);

    _numClients = 0;

    connect(this, SIGNAL(valueChangedLog(QString)), ui->lblNumClnts, SLOT(setText(QString)), Qt::DirectConnection);
}

Application::~Application()
{
    delete ui;
}

void Application::on_actionStart_Server_triggered()
{
    pthread_t listenThread;


    pthread_create(&listenThread, NULL, &StartServer, (void*)this);
}

void Application::ClientConnect()
{
    _numClients++;

    emit valueChangedLog(QString::number(_numClients));
}

void Application::ClientDisconnect()
{
    _numClients--;

    emit valueChangedLog(QString::number(_numClients));
}

void Application::UpdateGui(int n)
{
    emit valueChangedLog(QString::number(n));
}
