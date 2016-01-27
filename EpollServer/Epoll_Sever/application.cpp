#include "application.h"
#include "ui_application.h"


Application::Application(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Application)
{
    ui->setupUi(this);
}

Application::~Application()
{
    delete ui;
}

void Application::on_actionStart_Server_triggered()
{
    pthread_t listenThread;

    pthread_create(&listenThread, NULL, &StartServer, (void*)1);
}
