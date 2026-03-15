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

#ifndef APICLIENT_HPP
#define APICLIENT_HPP

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class ApiClient : public QObject
{
    Q_OBJECT
private:
    QTcpSocket *socket ;
    QByteArray buffer ;

    explicit ApiClient(QObject* parent = nullptr);
    ~ApiClient() = default ;
    
public:
    static ApiClient* instance() ;
    ApiClient(const ApiClient&) = delete ;
    ApiClient& operator=(const ApiClient&) = delete ;
    ApiClient(ApiClient&&) = delete ;
    ApiClient& operator=(ApiClient&&) = delete ;

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

#endif // APICLIENT_HPP
