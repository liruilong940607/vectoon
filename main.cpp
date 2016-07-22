#include "Mainwindow.h"
#include <QtWidgets/QApplication>
#include "CmConsoleWindow.h"
#include <iostream>
using namespace std;
int main(int argc, char *argv[])
{
	CmConsoleWindow cw;
	cw.SetTitle("Console window demo - Just a test");
	cw.SetConsoleAtrribute(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	cw.ResetConsoleAtrribute();

	//Video_Vector video_vector;
	//video_vector.load_video("./data/out.mp4");
	//video_vector.CalculateHMatrixAndSave();
	//return 0;

	QApplication a(argc, argv);
	Mainwindow w;
	w.show();

	//Video_Player_Widget player;
	//player.loadvideo("./data/out.mp4");
	//player.show();
	
	return a.exec();
}
