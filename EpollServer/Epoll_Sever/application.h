#ifndef APPLICATION_H
#define APPLICATION_H

#include <QMainWindow>
#include <QDebug>
#include <pthread.h>
#include "server.h"

namespace Ui {
class Application;
}

class Application : public QMainWindow
{
    Q_OBJECT

public:
    explicit Application(QWidget *parent = 0);
    ~Application();
    void ClientConnect();
    void ClientDisconnect();
    void UpdateGui(int);

private slots:
    void on_actionStart_Server_triggered();

signals:
    void valueChangedLog(QString);

private:
    Ui::Application *ui;
    int _numClients;
};

#endif // APPLICATION_H
