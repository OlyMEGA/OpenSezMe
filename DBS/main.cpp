#include <QtCore/QCoreApplication>
#include "dbs.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    DBS omega_dbs;

    return a.exec();
}
