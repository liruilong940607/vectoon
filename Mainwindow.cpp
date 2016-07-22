#include "Mainwindow.h"

Mainwindow::Mainwindow(QWidget *parent)
	: QMainWindow(parent)
{
	createActions();
	createMenus();
    initializelayout();
	this->setGeometry(200, 200, 500, 500);
}

Mainwindow::~Mainwindow()
{

}

void Mainwindow::createActions(){
	openAct = new QAction(QIcon("./Icon/open.png"), tr("&Open..."), this);
	openAct->setShortcuts(QKeySequence::Open);
	openAct->setStatusTip(tr("open a existing file"));
	connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

	saveAct = new QAction(QIcon("./Icon/save.png"), tr("&Save"), this);
	saveAct->setShortcut(QKeySequence::Save);
	saveAct->setStatusTip(tr("Save the document to disk"));
	connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    playAct = new QAction(QIcon("./Icon/play.jpg"), tr("&Play"), this);//
	playAct->setStatusTip(tr("play the video"));
	connect(playAct, SIGNAL(triggered()), this, SLOT(play()));
}

void Mainwindow::createMenus(){
	menuBar();
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(openAct);
	fileMenu->addAction(saveAct);
	fileMenu->addAction(playAct);
}

void Mainwindow::initializelayout(){
    int height = 450;

    mainwidget = new QWidget();

    initleftToolWidget();
    player = new Video_Player_Widget();
    rightShowWidget = new QWidget();
    initmsglabel();

    leftToolWidget->setGeometry(0,0,100,height);
    player->setGeometry(0,0,500,height);
    rightShowWidget->setGeometry(0,0,500,height);
    msglabel->setFixedHeight(150);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet("QSplitter::handle{background-color:rgb(255,255,255)}");
    splitter->addWidget(leftToolWidget);
    splitter->addWidget(player);
    splitter->addWidget(rightShowWidget);

    QSplitter *mainsplitter = new QSplitter(Qt::Vertical);
    mainsplitter->addWidget(splitter);
    mainsplitter->addWidget(msglabel);

    this->setFixedHeight(height+150);
    this->setFixedWidth(leftToolWidget->width()+player->width()+rightShowWidget->width()+30);
    this->setCentralWidget(mainsplitter);

    statusBar();
}

void Mainwindow::initleftToolWidget(){
    leftToolWidget = new QWidget();

    btcutVideo = new QPushButton("cut video");
    is_ready_to_cutvideo = false;
    connect(btcutVideo,SIGNAL(clicked()),this,SLOT(cutVideo()));

    QVBoxLayout *btlayout = new QVBoxLayout();
    btlayout->addWidget(btcutVideo);
    btlayout->addStretch();

    leftToolWidget->setLayout(btlayout);
}

void Mainwindow::initmsglabel(){
    msglabel = new QTextBrowser();
    updateMSG("Print Info:\n");
}

void Mainwindow::open()
{
    srcname = QFileDialog::getOpenFileName(this, tr("Open File"), "./data/");
    play();
}
void Mainwindow::save()
{

}
void Mainwindow::play(){
	player->loadvideo(srcname);
	player->show();
    if(player->Isloadsuccess()){
        is_ready_to_cutvideo = true;
        updateMSG(">>open file successfully! : "+srcname+"\n");
    }else{
        updateMSG(">>open file failed! \n");
    }
}
void Mainwindow::cutVideo(){
    if(is_ready_to_cutvideo){
        is_ready_to_cutvideo = false;
        updateMSG(">>start to cut video......... \n");
		video_vector.CalculateHMatrixAndSave();

    }
    //show_cutvideo_result();
    is_ready_to_cutvideo = true;
}
void Mainwindow::updateMSG(QString newmsg){
    message += newmsg;
    msglabel->setText(message);
}
