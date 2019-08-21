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

QT_FORWARD_DECLARE_CLASS(QBluetoothDeviceDiscoveryAgent)
QT_FORWARD_DECLARE_CLASS(QBluetoothDeviceInfo)

QT_USE_NAMESPACE

#define UTT_BOARD_ADDRESS              "94:21:97:60:14:D6"

class Device : public QObject
{
    Q_OBJECT

public:
    Device();
    ~Device();
    void startScan();

private slots:
    void addDevice(const QBluetoothDeviceInfo&);
    void startClient();
    void scanFinished();
    void addService(const QBluetoothServiceInfo &info);
    void readSocket();
    void requestData();
    void processReceivedData();

private:
    QList<QString> discoveredDevicesList;
    QList<QBluetoothServiceInfo> discoveredServicesList;
    QBluetoothSocket *socket;
    QBluetoothServiceDiscoveryAgent *serviceDiscoveryAgent;
    QBluetoothDeviceDiscoveryAgent *deviceDiscoveryAgent;
    QBluetoothLocalDevice *localDevice;

};

#endif // DEVICE_H
