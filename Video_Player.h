#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <qwidget.h>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>
#include <qpushbutton.h>
#include <qdatetime.h>
#include <qstring.h>
#include <qlabel.h>
#include <QLayout>
#include <qdebug.h>

class Video_Player_Widget : public QWidget
{
	Q_OBJECT
public:
	Video_Player_Widget(QWidget *parent = 0);
	~Video_Player_Widget();

	void loadvideo(QString filename);
    bool Isloadsuccess();

private:
	
	QString name;
	QMediaPlayer *player;
	QVideoWidget *videoWidget;
	QWidget *ctrlwidget;
	QPushButton *btstart_pause;
	QPushButton *btstop;
	qint64 total_time_value;
	QTime total_time;
	QLabel *time_label;
	bool isplaying;
	bool loadsuccess;

private slots:

	void UpdateTime(qint64 time);
	void play_pause();
	void stop();

    void durationChanged(qint64 duration);

};



#endif
