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

#include "api/ApiClient.hpp"
#include "config/Config.hpp"

#include <string>

ApiClient* ApiClient::instance(){
    static ApiClient* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new ApiClient();
    }
    return s_instance ;
}

ApiClient::ApiClient(QObject *parent)
    : QObject{parent}, socket(new QTcpSocket(this)){
    connect(socket, &QTcpSocket::readyRead, this, &ApiClient::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &ApiClient::connected);
    connect(socket, &QTcpSocket::disconnected, this, &ApiClient::disconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &ApiClient::onErrorOccurred);
}

void ApiClient::connectToBackend(){
    Config::load();
    QString serverAddress = QString::fromStdString(Config::get<std::string>("server.address").value()) ;
    int serverPort = Config::get<int>("server.port").value() ;
    qDebug() << "connecting to " << serverAddress << "port" << serverPort ;
    socket->connectToHost(serverAddress, serverPort );
}

void ApiClient::sendMessage(const json& j){
    QByteArray msg = QByteArray::fromStdString(j.dump()) + "\n" ;
    qInfo() << "Sending Message:" << msg ;
    if ( socket->state() == QAbstractSocket::ConnectedState ){
        socket->write(msg);
    }
}

// slot functions

void ApiClient::onReadyRead() {
    buffer.append(socket->readAll());

    while (true) {
        int idxEnd = buffer.indexOf('\n');
        if ( idxEnd == -1 ) break ;

        QByteArray line = buffer.left(idxEnd);
        buffer.remove(0, idxEnd + 1);

        try {
            json j = json::parse(line.constData(), line.constData() + line.size());
            qDebug() << "Api Response Received:" << line;
            emit dataReceived(j);
        } catch (const json::parse_error& e) {
            qWarning() << "Invalid JSON received:" << line << "-" << e.what();
        }
    }
}

void ApiClient::onConnected() {
    emit connected();
}

void ApiClient::onDisconnected() {
    emit disconnected();
}

void ApiClient::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    emit errorOccurred(socket->errorString());
}
