/****************************************************************************
** Meta object code from reading C++ file 'Video_Player.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Video_Player.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Video_Player.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_Video_Player_Widget_t {
    QByteArrayData data[8];
    char stringdata[78];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Video_Player_Widget_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Video_Player_Widget_t qt_meta_stringdata_Video_Player_Widget = {
    {
QT_MOC_LITERAL(0, 0, 19), // "Video_Player_Widget"
QT_MOC_LITERAL(1, 20, 10), // "UpdateTime"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 4), // "time"
QT_MOC_LITERAL(4, 37, 10), // "play_pause"
QT_MOC_LITERAL(5, 48, 4), // "stop"
QT_MOC_LITERAL(6, 53, 15), // "durationChanged"
QT_MOC_LITERAL(7, 69, 8) // "duration"

    },
    "Video_Player_Widget\0UpdateTime\0\0time\0"
    "play_pause\0stop\0durationChanged\0"
    "duration"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Video_Player_Widget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x08 /* Private */,
       4,    0,   37,    2, 0x08 /* Private */,
       5,    0,   38,    2, 0x08 /* Private */,
       6,    1,   39,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::LongLong,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,    7,

       0        // eod
};

void Video_Player_Widget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Video_Player_Widget *_t = static_cast<Video_Player_Widget *>(_o);
        switch (_id) {
        case 0: _t->UpdateTime((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 1: _t->play_pause(); break;
        case 2: _t->stop(); break;
        case 3: _t->durationChanged((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject Video_Player_Widget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Video_Player_Widget.data,
      qt_meta_data_Video_Player_Widget,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *Video_Player_Widget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Video_Player_Widget::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_Video_Player_Widget.stringdata))
        return static_cast<void*>(const_cast< Video_Player_Widget*>(this));
    return QWidget::qt_metacast(_clname);
}

int Video_Player_Widget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
