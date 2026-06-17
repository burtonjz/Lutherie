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

#ifndef CONTROL_API_CLIENT_HPP_
#define CONTROL_API_CLIENT_HPP_

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class ControlApiClient : public QObject {
    Q_OBJECT
    
private:
    QTcpSocket *socket ;
    QByteArray buffer ;

    explicit ControlApiClient(QObject* parent = nullptr);
    ~ControlApiClient() = default ;
    
public:
    static ControlApiClient* instance() ;
    ControlApiClient(const ControlApiClient&) = delete ;
    ControlApiClient& operator=(const ControlApiClient&) = delete ;
    ControlApiClient(ControlApiClient&&) = delete ;
    ControlApiClient& operator=(ControlApiClient&&) = delete ;

    void connectToBackend();
    void sendMessage(const json& j);

signals:
    void connected();
    void disconnected();
    void dataReceived(const json& j);
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

};

#endif // CONTROL_API_CLIENT_HPP_
