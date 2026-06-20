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

#include "api/DataApiClient.hpp"
#include "config/Config.hpp"
#include "requests/DataDescriptor.hpp"

#include <string>

DataApiClient* DataApiClient::instance(){
    static DataApiClient* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new DataApiClient();
    }
    return s_instance ;
}

DataApiClient::DataApiClient(QObject *parent)
    : QObject{parent}, socket_(new QTcpSocket(this)){
    connect(socket_, &QTcpSocket::readyRead, this, &DataApiClient::onReadyRead);
    connect(socket_, &QTcpSocket::connected, this, &DataApiClient::connected);
    connect(socket_, &QTcpSocket::disconnected, this, &DataApiClient::disconnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, &DataApiClient::onErrorOccurred);
}

void DataApiClient::connectToBackend(){
    Config::load();
    QString serverAddress = QString::fromStdString(Config::get<std::string>("server.address").value()) ;
    int serverPort = Config::get<int>("server.data_port").value() ;
    qDebug() << "connecting to " << serverAddress << "port" << serverPort ;
    socket_->connectToHost(serverAddress, serverPort );
}

// slot functions

void DataApiClient::onReadyRead(){
    readBuffer_.append(socket_->readAll());

    while ( true ){
        // Wait until the full header is available
        const size_t headerSize = sizeof(DataDescriptor);
        if ( readBuffer_.size() < headerSize ){
            return ;
        }
            
        DataDescriptor header;
        std::memcpy(
            &header,
            readBuffer_.constData(),
            sizeof(DataDescriptor)
        );
        
        const size_t dataSize = header.size ;
        const size_t totalSize = headerSize + dataSize ;

        // Wait until the entire payload is available
        if ( readBuffer_.size() < totalSize ){
            return ;
        }
            
        QByteArray payload = readBuffer_.mid(headerSize, dataSize);
        readBuffer_.remove(0, totalSize);

        if ( buffers_.contains(header) ){
            buffers_.at(header).clear();
        }

        auto& data = buffers_[header];
        if ( dataSize > 0 ){
            if ( dataSize % sizeof(double) != 0 ){
                qWarning() << "Invalid data size:" << dataSize << "(not divisible by sizeof(double))";
                continue ;
            }

            const int numValues = dataSize / sizeof(double);
            data.resize(numValues);

            std::memcpy(
                data.data(),
                readBuffer_.constData() + headerSize,
                dataSize
            );
        }

        qDebug() << "received data for componentId:" << header.componentId
            << ". dataSize: " << dataSize
            << "bytes (" << data.size() << " values)";

        emit dataReceived(header, data);
    }
}

void DataApiClient::onConnected() {
    emit connected();
}

void DataApiClient::onDisconnected() {
    emit disconnected();
}

void DataApiClient::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    emit errorOccurred(socket_->errorString());
}

void DataApiClient::onComponentRemoved(int componentId){
    for ( auto it = buffers_.begin() ; it != buffers_.end(); ){
        if ( it->first.componentId == componentId ){
            it = buffers_.erase(it);
        } else {
            ++it ;
        }
    }
}