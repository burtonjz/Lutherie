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

#include "api/ControlApiClient.hpp"
#include "config/Config.hpp"

#include <string>

ControlApiClient* ControlApiClient::instance(){
    static ControlApiClient* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new ControlApiClient();
    }
    return s_instance ;
}

ControlApiClient::ControlApiClient(QObject *parent)
    : QObject{parent}, socket(new QTcpSocket(this)){
    connect(socket, &QTcpSocket::readyRead, this, &ControlApiClient::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &ControlApiClient::connected);
    connect(socket, &QTcpSocket::disconnected, this, &ControlApiClient::disconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &ControlApiClient::onErrorOccurred);
}

void ControlApiClient::connectToBackend(){
    Config::load();
    QString serverAddress = QString::fromStdString(Config::get<std::string>("server.address").value()) ;
    int serverPort = Config::get<int>("server.control_port").value() ;
    qDebug() << "connecting to " << serverAddress << "port" << serverPort ;
    socket->connectToHost(serverAddress, serverPort );
}

void ControlApiClient::sendMessage(const json& j){
    QByteArray msg = QByteArray::fromStdString(j.dump()) + "\n" ;
    qInfo() << "Sending Message:" << msg ;
    if ( socket->state() == QAbstractSocket::ConnectedState ){
        socket->write(msg);
    }
}

// slot functions

void ControlApiClient::onReadyRead() {
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

void ControlApiClient::onConnected() {
    emit connected();
}

void ControlApiClient::onDisconnected() {
    emit disconnected();
}

void ControlApiClient::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    emit errorOccurred(socket->errorString());
}
