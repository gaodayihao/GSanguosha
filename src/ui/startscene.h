#ifndef STARTSCENE_H
#define STARTSCENE_H

#include "button.h"
#include "pixmap.h"
#include "server.h"

#include <QGraphicsScene>
#include <QAction>
#include <QTextEdit>

class StartScene: public QGraphicsScene{
    Q_OBJECT

public:
    StartScene();
    void addButton(QAction *action);
    void setServerLogBackground();
    void switchToServer(Server *server);

private:
    void printServerInfo();

    Pixmap *logo;
    QTextEdit *server_log;
    QList<Button*> buttons;

    QLineEdit *server_cmdline;
private slots:
    void transBase64(QString msg);
signals:
    void log_append(QString);
};

#endif // STARTSCENE_H
