#include "device.h"

Device::Device()
{
    deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(5000);
    discoveredDevicesList = {};
    discoveredServicesList = {};

    connect(deviceDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this, SLOT(addDevice(QBluetoothDeviceInfo)));
    connect(deviceDiscoveryAgent, SIGNAL(finished()), this, SLOT(scanFinished()));
}

Device::~Device()
{
    delete deviceDiscoveryAgent;
    delete serviceDiscoveryAgent;
}

void Device::addDevice(const QBluetoothDeviceInfo &info)
{
    discoveredDevicesList.append(info.address().toString());
    QString label = QString("%1 %2").arg(info.address().toString()).arg(info.name());
    qInfo() << label;
}

void Device::startScan()
{
    deviceDiscoveryAgent->start();
}

void Device::scanFinished()
{
    qInfo() << "scan finished!";

    //Using default Bluetooth adapter
    QBluetoothLocalDevice localDevice;
    QBluetoothAddress adapterAddress = localDevice.address();
    QBluetoothAddress uttBoardAddress(UTT_BOARD_ADDRESS);

    /*
         * In case of multiple Bluetooth adapters it is possible to
         * set which adapter will be used by providing MAC Address.
         * Example code:
         *
         * QBluetoothAddress adapterAddress("XX:XX:XX:XX:XX:XX");
         * discoveryAgent = new QBluetoothServiceDiscoveryAgent(adapterAddress);
         */

    serviceDiscoveryAgent = new QBluetoothServiceDiscoveryAgent(adapterAddress);
    serviceDiscoveryAgent->setRemoteAddress(uttBoardAddress);

    connect(serviceDiscoveryAgent, SIGNAL(serviceDiscovered(QBluetoothServiceInfo)),
            this, SLOT(addService(QBluetoothServiceInfo)));
    connect(serviceDiscoveryAgent, SIGNAL(finished()), this, SLOT(startClient()));

    serviceDiscoveryAgent->start();
}

void Device::startClient()
{
    if (socket)
        return;

    // Connect to service
    socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);
    qDebug() << "Create socket";

    QBluetoothAddress uttBoardAddress(UTT_BOARD_ADDRESS);

    socket->connectToService(uttBoardAddress, 1);
    qDebug() << "ConnectToService done";


    connect(socket, SIGNAL(readyRead()), this, SLOT(readSocket()));
    connect(socket, SIGNAL(connected()), this, SLOT(requestData()));
    connect(socket, SIGNAL(disconnected()), this, SIGNAL(processReceivedData()));
}

void Device::processReceivedData()
{
    qDebug() << "Processing received data...";
}
void Device::requestData()
{
    qDebug() << "Requesting data...";

    QByteArray startPrefix = QByteArray::fromHex("5253");
    socket->write(startPrefix, 1024);
}

void Device::readSocket()
{
    qDebug() << "Data received...";
    QByteArray data = socket->readAll();
    qDebug() << data;
    QByteArray endPrefix = QByteArray::fromHex("5350");
    socket->write(endPrefix, 1024);
}

void Device::addService(const QBluetoothServiceInfo &info)
{
    if (info.serviceName().isEmpty())
        return;

    QString line = info.serviceName();
    if (!info.serviceDescription().isEmpty())
        line.append("\n\t" + info.serviceDescription());
    if (!info.serviceProvider().isEmpty())
        line.append("\n\t" + info.serviceProvider());
    discoveredServicesList.append(info);
    qInfo() << line;
}
