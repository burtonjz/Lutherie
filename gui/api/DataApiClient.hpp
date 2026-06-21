/*
 * Copyright (C) 2025 Jared Burton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DATA_API_CLIENT_HPP_
#define DATA_API_CLIENT_HPP_

#include "requests/DataDescriptor.hpp"

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <vector>

class DataApiClient : public QObject {
    Q_OBJECT
    
private:
    QTcpSocket* socket_ ;
    QByteArray readBuffer_ ;

    explicit DataApiClient(QObject* parent = nullptr);
    ~DataApiClient() = default ;
    
public:
    static DataApiClient* instance() ;
    DataApiClient(const DataApiClient&) = delete ;
    DataApiClient& operator=(const DataApiClient&) = delete ;
    DataApiClient(DataApiClient&&) = delete ;
    DataApiClient& operator=(DataApiClient&&) = delete ;

    void connectToBackend();
    
signals:
    void connected();
    void disconnected();
    void dataReceived(DataDescriptor header, std::vector<double> buffer);
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

};

#endif // DATA_API_CLIENT_HPP_
