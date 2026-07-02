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

#include <spdlog/spdlog.h>
#include <string>

ControlApiClient* ControlApiClient::instance(){
    static ControlApiClient* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new ControlApiClient();
    }
    return s_instance ;
}

ControlApiClient::ControlApiClient(QObject *parent): 
    QObject{parent}, 
    socket_(new QTcpSocket(this))
{
    connect(socket_, &QTcpSocket::readyRead, this, &ControlApiClient::onReadyRead);
    connect(socket_, &QTcpSocket::connected, this, &ControlApiClient::connected);
    connect(socket_, &QTcpSocket::disconnected, this, &ControlApiClient::disconnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, &ControlApiClient::onErrorOccurred);
}

void ControlApiClient::connectToBackend(){
    Config::load();
    QString serverAddress = QString::fromStdString(Config::get<std::string>("server.address").value()) ;
    int serverPort = Config::get<int>("server.control_port").value() ;
    SPDLOG_INFO("connecting to {} port {}", serverAddress.toStdString(), serverPort);
    socket_->connectToHost(serverAddress, serverPort );
}

void ControlApiClient::sendMessage(const json& j){
    QByteArray msg = QByteArray::fromStdString(j.dump()) + "\n" ;
    SPDLOG_INFO("sending Control API Client request: {}", j.dump());
    if ( socket_->state() == QAbstractSocket::ConnectedState ){
        socket_->write(msg);
    }
}

// slot functions

void ControlApiClient::onReadyRead() {
    buffer_.append(socket_->readAll());

    while (true) {
        int idxEnd = buffer_.indexOf('\n');
        if ( idxEnd == -1 ) break ;

        QByteArray line = buffer_.left(idxEnd);
        buffer_.remove(0, idxEnd + 1);

        try {
            json j = json::parse(line.constData(), line.constData() + line.size());
            SPDLOG_DEBUG("Api Response Received: {}", line.toStdString());
            emit dataReceived(j);
        } catch (const json::parse_error& e) {
            SPDLOG_WARN("Invalid JSON received: {}. Error: {}", line.toStdString(), e.what());
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
    emit errorOccurred(socket_->errorString());
}
