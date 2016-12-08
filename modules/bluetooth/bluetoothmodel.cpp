/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "bluetoothmodel.h"

namespace dcc {
namespace bluetooth {

BluetoothModel::BluetoothModel(QObject *parent) : QObject(parent)
{

}

void BluetoothModel::addAdapter(const Adapter *adapter)
{
    if (!adapterById(adapter->id())) {
        m_adapters.append(adapter);
        emit adapterAdded(adapter);
    }
}

void BluetoothModel::removeAdapater(const QString &adapterId)
{
    const Adapter *adapter = adapterById(adapterId);
    if (adapter) m_adapters.removeAll(adapter);

    emit adapterRemoved(adapterId);
}

QList<const Adapter *> BluetoothModel::adapters() const
{
    return m_adapters;
}

const Adapter *BluetoothModel::adapterById(const QString &id)
{
    for (const Adapter *adapter : m_adapters) {
        if (adapter->id() == id) {
            return adapter;
        }
    }

    return nullptr;
}

} // namespace bluetooth
} // namespace dcc