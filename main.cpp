#include <QCoreApplication>
#include "device.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Device *d = new Device();
    d->startScan();

    return a.exec();
}
