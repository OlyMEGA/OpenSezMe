/****************************************************************************
** Meta object code from reading C++ file 'dbs.h'
**
** Created: Sun May 6 11:52:51 2012
**      by: The Qt Meta Object Compiler version 61 (Qt 4.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "dbs.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dbs.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 61
#error "This file was generated using the moc from 4.5.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DBS[] = {

 // content:
       2,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   12, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors

 // signals: signature, parameters, type, tag, flags
       5,    4,    4,    4, 0x05,

 // slots: signature, parameters, type, tag, flags
      17,    4,    4,    4, 0x08,
      30,    4,    4,    4, 0x08,
      44,    4,    4,    4, 0x08,
      58,    4,    4,    4, 0x08,
      70,    4,    4,    4, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_DBS[] = {
    "DBS\0\0handleCmd()\0initSystem()\0"
    "receiveData()\0pingTimeout()\0pingThing()\0"
    "processCommand()\0"
};

const QMetaObject DBS::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_DBS,
      qt_meta_data_DBS, 0 }
};

const QMetaObject *DBS::metaObject() const
{
    return &staticMetaObject;
}

void *DBS::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DBS))
        return static_cast<void*>(const_cast< DBS*>(this));
    return QObject::qt_metacast(_clname);
}

int DBS::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: handleCmd(); break;
        case 1: initSystem(); break;
        case 2: receiveData(); break;
        case 3: pingTimeout(); break;
        case 4: pingThing(); break;
        case 5: processCommand(); break;
        default: ;
        }
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void DBS::handleCmd()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
