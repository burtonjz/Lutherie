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
#include "app/Theme.hpp"

#include <spdlog/spdlog.h>

ControlApiClient* ControlApiClient::instance(){
    static ControlApiClient* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new ControlApiClient();
    }
    return s_instance ;
}

ControlApiClient::ControlApiClient(QObject *parent): 
    QObject{parent}, 
    socket_(new QTcpSocket(this)),
    queuedRequests_(),
    reconnectTimer_(new QTimer(this))
{
    connect(
        socket_, &QTcpSocket::readyRead, 
        this, &ControlApiClient::onReadyRead
    );
    connect(
        socket_, &QTcpSocket::connected, 
        this, &ControlApiClient::onConnected
    );
    connect(
        socket_, &QTcpSocket::disconnected, 
        this, &ControlApiClient::onDisconnected
    );
    connect(
        socket_, &QTcpSocket::errorOccurred, 
        this, &ControlApiClient::onErrorOccurred
    );

    reconnectTimer_->setSingleShot(true);
    connect(
        reconnectTimer_, &QTimer::timeout,
        this, &ControlApiClient::connectToBackend
    );
}

void ControlApiClient::connectToBackend(){
    Config::load();
    QString serverAddress = QString::fromStdString(Config::get<std::string>("server.address").value()) ;
    int serverPort = Config::get<int>("server.control_port").value() ;
    SPDLOG_INFO("connecting to {} port {}", serverAddress.toStdString(), serverPort);
    socket_->connectToHost(serverAddress, serverPort );
}

bool ControlApiClient::sendMessage(const json& j){
    QByteArray msg = QByteArray::fromStdString(j.dump()) + "\n" ;
    SPDLOG_INFO("sending Control API Client request: {}", j.dump());
    if ( socket_->state() == QAbstractSocket::ConnectedState ){
        socket_->write(msg);
        return true ;
    } else {
        SPDLOG_WARN("Control API Client is currently not connected to backend server. Queueing message...");
        queuedRequests_.push_back(j);
        return false ;
    }
}

bool ControlApiClient::isConnected() const {
    return socket_->state() == QAbstractSocket::ConnectedState ;
}

void ControlApiClient::scheduleReconnect() {
    if (reconnectAttempts_ >= Theme::API_NUM_RECONNECT_ATTEMPTS ){
        SPDLOG_ERROR(
            "Control API Client giving up after {} reconnect attempts", 
            reconnectAttempts_
        );
        emit reconnectFailed();
        return ;
    }

    int delay = Theme::API_RECONNECT_DELAY_MS * (1 << reconnectAttempts_++); 

    SPDLOG_INFO(
        "Scheduling reconnect attempt {}/{} in {} ms",
        reconnectAttempts_, Theme::API_NUM_RECONNECT_ATTEMPTS, delay
    );

    reconnectTimer_->start(delay);
}

void ControlApiClient::flushQueue(){
    if ( queuedRequests_.size() == 0 ) return ;
    
    SPDLOG_DEBUG(
        "flushing {} queued message(s)", 
        queuedRequests_.size()
    );
    while ( !queuedRequests_.empty() ){
        if ( socket_->state() != QAbstractSocket::ConnectedState ){
            SPDLOG_WARN(
                "lost connection mid-flush, {} message(s) still queued",
                queuedRequests_.size()
            );
            break ;
        }

        json j = std::move(queuedRequests_.front());
        queuedRequests_.erase(queuedRequests_.begin());
        sendMessage(j);
    }
}

// slot functions
void ControlApiClient::onReadyRead(){
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

void ControlApiClient::onConnected(){
    SPDLOG_INFO("Control API Client connected");
    reconnectAttempts_ = 0 ;
    flushQueue();
    emit connected();
}

void ControlApiClient::onDisconnected(){
    SPDLOG_WARN("Control API Client disconnected");
    emit disconnected();
    scheduleReconnect();
}

void ControlApiClient::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    SPDLOG_WARN("Control API Client socket error: {}", socket_->errorString().toStdString());
    emit errorOccurred(socket_->errorString());
    if ( !reconnectTimer_->isActive() ) scheduleReconnect();
}

