#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include "networkdevice.h"

#include <QObject>

namespace dcc {

namespace network {

class NetworkDevice;
class NetworkWorker;
class NetworkModel : public QObject
{
    Q_OBJECT

    friend class NetworkWorker;

public:
    explicit NetworkModel(QObject *parent = 0);
    ~NetworkModel();

    const QList<NetworkDevice *> devices() const { return m_devices; }

signals:
    void deviceListChanged(const QList<NetworkDevice *> devices) const;

private slots:
    void onDevicesPropertyChanged(const QString &devices);

private:
    QList<NetworkDevice *> m_devices;
};

}   // namespace network

}   // namespace dcc

#endif // NETWORKMODEL_H