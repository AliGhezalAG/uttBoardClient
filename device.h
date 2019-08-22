#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothaddress.h>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothlocaldevice.h>
#include <QBluetoothServiceInfo>
#include <QBluetoothSocket>
#include <QBluetoothServiceDiscoveryAgent>
#include <QDebug>
#include <QtCore>

#include <iostream>
#include <fstream>

QT_FORWARD_DECLARE_CLASS(QBluetoothDeviceDiscoveryAgent)
QT_FORWARD_DECLARE_CLASS(QBluetoothDeviceInfo)

QT_USE_NAMESPACE

using namespace std;

#define UTT_BOARD_ADDRESS               "94:21:97:60:14:D6"
#define PACKET_SIZE                     12

struct Packet {
    int recordNumber;
    double timeStamp;
    QJsonArray pressureMeasures;

    QJsonObject toJson() const {
        return {
            {"recordNumber", recordNumber},
            {"timeStamp", timeStamp},
            {"pressureMeasures", pressureMeasures}
        };
    }
};

struct Acquisition {
    int          serialNumber;
    int          samplingFrequency;
    QString      frequencyUnit;
    QString      scaleMode;
    double        versionNumber;
    int          batteryLevel;
    int          distanceBetweenX;
    int          distanceBetweenY;
    QString      distanceUnit;
    QString      pressureOrder;
    QString      pressureUnit;
    int          recordNumber;
    int          errorCode;
    int          weightValue;
    int          impedance;
    QJsonArray   measurePackets;

    QJsonObject toJson() const {
        return {
            {"serialNumber", serialNumber},
            {"samplingFrequency", samplingFrequency},
            {"frequencyUnit", frequencyUnit} ,
            {"scaleMode", scaleMode},
            {"versionNumber", versionNumber},
            {"batteryLevel", batteryLevel},
            {"distanceBetweenX", distanceBetweenX},
            {"distanceBetweenY", distanceBetweenY} ,
            {"distanceUnit", distanceUnit},
            {"pressureOrder", pressureOrder},
            {"pressureUnit", pressureUnit},
            {"recordNumber", recordNumber},
            {"errorCode", errorCode} ,
            {"weightValue", weightValue},
            {"impedance", impedance},
            {"measurePackets", measurePackets}
        };
    }
};

class Device : public QObject
{
    Q_OBJECT

public:
    Device();
    ~Device();
    void startScan();

private slots:
    void startClient();
    void deviceScanFinished();
    void addDevice(const QBluetoothDeviceInfo&);
    void addService(const QBluetoothServiceInfo &info);
    void readSocket();
    void requestData();
    void processReceivedData();
    void deviceDisconnected();

private:
    QList<QString>                  discoveredDevicesList;
    QList<QBluetoothServiceInfo>    discoveredServicesList;
    QBluetoothSocket                *socket;
    QBluetoothServiceDiscoveryAgent *serviceDiscoveryAgent;
    QBluetoothDeviceDiscoveryAgent  *deviceDiscoveryAgent;
    QBluetoothLocalDevice           *localDevice;
    QByteArray                      receivedData;
    QByteArray                      openingPacket;
    QByteArray                      closingPacket;
    QList<QByteArray>               recordPacketList;
    Acquisition                     *acquisition;
    ofstream                        logFile;
    void extractPackets();
    int byteArrayToInt(const QByteArray &bytes);
    double byteArrayToDouble(const QByteArray &bytes);

signals:
    void dataReceived();

};

#endif // DEVICE_H
