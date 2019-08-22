#include "device.h"

Device::Device()
{
    deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(5000);
    discoveredDevicesList = {};
    discoveredServicesList = {};

    connect(deviceDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this, SLOT(addDevice(QBluetoothDeviceInfo)));
    connect(deviceDiscoveryAgent, SIGNAL(finished()), this, SLOT(deviceScanFinished()));
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

void Device::deviceScanFinished()
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

    connect(socket, &QBluetoothSocket::connected, this, &Device::requestData);
    connect(socket, &QBluetoothSocket::readyRead, this, &Device::readSocket);
    connect(this, &Device::dataReceived, this, &Device::processReceivedData);
    connect(socket, &QBluetoothSocket::disconnected, this, &Device::deviceDisconnected);
}

void Device::processReceivedData()
{
    qDebug() << "Processing received data...";
    extractPackets();
    acquisition = new Acquisition();
    acquisition->serialNumber = byteArrayToInt(openingPacket.mid(0, 4));
    acquisition->samplingFrequency = byteArrayToInt(openingPacket.mid(4, 1));
    acquisition->scaleMode = "Normal mode";
    acquisition->versionNumber = byteArrayToInt(openingPacket.mid(6, 1));
    acquisition->batteryLevel = byteArrayToInt(openingPacket.mid(7, 1));
    acquisition->distanceBetweenX = byteArrayToInt(openingPacket.mid(8, 2));
    acquisition->distanceBetweenY = byteArrayToInt(openingPacket.mid(10, 2));

    acquisition->recordNumber = byteArrayToInt(closingPacket.mid(0, 4));
    acquisition->errorCode = byteArrayToInt(closingPacket.mid(4, 2));
    acquisition->weightValue = byteArrayToInt(closingPacket.mid(6, 2));
    acquisition->impedance = byteArrayToInt(closingPacket.mid(8, 2));

    QListIterator<QByteArray> i(recordPacketList);
    QByteArray currentPacket;

    while (i.hasNext()){
        currentPacket = i.next();
        Packet *packet = new Packet();
        packet->recordNumber = byteArrayToInt(currentPacket.mid(0, 4));
        packet->timeStamp = packet->recordNumber / acquisition->samplingFrequency;
        QList<double> records = {};
        packet->pressureMeasures.append(QJsonValue(byteArrayToDouble(currentPacket.mid(4, 2))));
        packet->pressureMeasures.append(QJsonValue(byteArrayToDouble(currentPacket.mid(6, 2))));
        packet->pressureMeasures.append(QJsonValue(byteArrayToDouble(currentPacket.mid(8, 2))));
        packet->pressureMeasures.append(QJsonValue(byteArrayToDouble(currentPacket.mid(10, 2))));
        acquisition->measurePackets.append(packet->toJson());
    }

    auto doc = QJsonDocument(acquisition->toJson());
    logFile.open ("results.json", ios::out | ios::app);
    logFile << doc.toJson().constData();
    logFile.close();

    qInfo() << acquisition->toJson();
}

int Device::byteArrayToInt(const QByteArray &bytes){
    QByteArray bytesToHex = bytes.toHex(0);
    bool ok;
    int hex = bytesToHex.toInt(&ok, 16);
    return hex;
}

double Device::byteArrayToDouble(const QByteArray &bytes){
    QByteArray bytesToHex = bytes.toHex(0);
    double hex = bytesToHex.toDouble();
    return hex;
}

void Device::extractPackets()
{
    qInfo() << "Extracting packets...";

    QByteArray openingPacketStartPrefix = QByteArray::fromHex("5049");
    QByteArray openingPacketEndPrefix = QByteArray::fromHex("4950");
    int openingPacketStartIndex = receivedData.indexOf(openingPacketStartPrefix)+2;
    int openingPacketEndIndex = receivedData.indexOf(openingPacketEndPrefix);
    openingPacket = receivedData.mid(openingPacketStartIndex, PACKET_SIZE);

    QByteArray closingPacketStartPrefix = QByteArray::fromHex("5052");
    int closingPacketStartIndex = receivedData.indexOf(closingPacketStartPrefix)+2;
    closingPacket = receivedData.mid(closingPacketStartIndex, PACKET_SIZE);

    QByteArray recordPacket = receivedData.mid(openingPacketEndIndex+2, closingPacketStartIndex-openingPacketEndIndex-4);
    recordPacketList = {};
    int recordPacketStartIndex = 2;

    while(recordPacketStartIndex < recordPacket.size()){
        recordPacketList.append(recordPacket.mid(recordPacketStartIndex, PACKET_SIZE));
        recordPacketStartIndex += 16;
    }
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
    receivedData.append(data);
    qDebug() << data;
    if (data.endsWith(QByteArray::fromHex("5250"))){
        qInfo() << "this is the end!!";
        emit dataReceived();
    }

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

void Device::deviceDisconnected(){
    qInfo() << "Disconnected!";
}
