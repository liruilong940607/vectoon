#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QFileDialog>
#include <qmenu.h>
#include <qmenubar.h>
#include <QSplitter>
#include <QLayout>
#include <QTextBrowser>
#include "Video_Player.h"
#include "Video_Vector.h"

class  Mainwindow: public QMainWindow
{
	Q_OBJECT

public:
    Mainwindow(QWidget *parent = 0);
    ~Mainwindow();

private:
	void createActions();
	void createMenus();
    void initializelayout();
    void initleftToolWidget();
    void initmsglabel();

	QMenu *fileMenu;
	QAction *saveAct;
	QAction *openAct;
	QAction *playAct;
	QString srcname;

    QWidget *mainwidget;
    QWidget *leftToolWidget;
    Video_Player_Widget *player;
    QWidget *rightShowWidget;
    QTextBrowser *msglabel;
    QString message;

    //lefttoolwidget
    QPushButton *btcutVideo;
    bool is_ready_to_cutvideo;

    Video_Vector video_vector;
private slots:
	void open();
	void save();
    void play();
    void cutVideo();
    void updateMSG(QString newmsg);
};

#endif // MAINWINDOW_H
