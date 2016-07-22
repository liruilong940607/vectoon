#include <QApplication>
#include "siftmatch.h"
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("GB18030"));

    QApplication a(argc, argv);
    SiftMatch w;
    w.show();
    
    return a.exec();
}
