#include "Video_Player.h"

Video_Player_Widget::Video_Player_Widget(QWidget *parent)
	: QWidget(parent)
{
	player = new QMediaPlayer();
	connect(player, SIGNAL(durationChanged(qint64)), this, SLOT(durationChanged(qint64)));
	connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(UpdateTime(qint64)));
	videoWidget = new QVideoWidget();

	ctrlwidget = new QWidget();
	btstart_pause = new QPushButton();
	connect(btstart_pause,SIGNAL(clicked()),this, SLOT(play_pause()));
	btstart_pause->setText("play");
	btstop = new QPushButton();
	connect(btstop, SIGNAL(clicked()), this, SLOT(stop()));
	btstop->setText("stop");
	total_time_value = 0;
	total_time = QTime(0, 0, 0);
	time_label = new QLabel();
	isplaying = 0;
	loadsuccess = 0;
	
	QHBoxLayout *ctrl_layout = new QHBoxLayout();
	ctrl_layout->addWidget(btstart_pause);
	ctrl_layout->addWidget(btstop);
	ctrl_layout->addStretch();
	ctrl_layout->addWidget(time_label);
	ctrl_layout->addStretch();
	ctrlwidget->setLayout(ctrl_layout);

	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout->addWidget(videoWidget);
	main_layout->addWidget(ctrlwidget);
	main_layout->setStretchFactor(videoWidget, 25);
	main_layout->setStretchFactor(ctrlwidget, 1);
	this->setLayout(main_layout);
    //this->setFixedHeight(640);
    //this->setFixedWidth(450);
	UpdateTime(0);

}
Video_Player_Widget::~Video_Player_Widget(){

}
void Video_Player_Widget::UpdateTime(qint64 time)
{
	total_time = QTime((total_time_value / 3600000), (total_time_value / 60000) % 60,
		(total_time_value / 1000) % 60);
	QTime current_time(0, (time / 60000) % 60, (time / 1000) % 60);
	QString str = current_time.toString("hh:mm:ss") + "/" + total_time.toString("hh:mm:ss");
	time_label->setText(str);
}

void Video_Player_Widget::play_pause(){
	if (loadsuccess){//load video success
		if (isplaying){
			btstart_pause->setText("start");
			isplaying = false;
			player->pause();
		}
		else{
			btstart_pause->setText("pause");
			isplaying = true;
			player->play();
		}
	}
}

void Video_Player_Widget::stop(){
	if (loadsuccess){
		isplaying = false;
		player->stop();
		btstart_pause->setText("start");
	}
}
void Video_Player_Widget::loadvideo(QString filename){
	name = filename;
	loadsuccess = Isloadsuccess();
	if (loadsuccess){
		total_time_value = player->duration();
		UpdateTime(0);
	}
}

bool Video_Player_Widget::Isloadsuccess(){//need to check
	player->setMedia(QUrl::fromLocalFile(name));
	if (player->mediaStatus() > 1){
		player->setVideoOutput(videoWidget);
		videoWidget->show();
		return true;
	}
	return false;
}

void Video_Player_Widget::durationChanged(qint64 duration){
	total_time_value = duration;
}
