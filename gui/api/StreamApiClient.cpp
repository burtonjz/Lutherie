/*
 * Copyright (C) 2026 Jared Burton
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

#include "api/StreamApiClient.hpp"
#include "managers/ComponentManager.hpp"

#include "config/Config.hpp"
#include "meta/ComponentRegistry.hpp"

#include "widgets/SpectrumAnalyzerWidget.hpp"
#include "widgets/OscilloscopeWidget.hpp"

#include <spdlog/spdlog.h>

StreamApiClient* StreamApiClient::instance(){
    if ( !instance_ ){
        instance_ = new StreamApiClient();
    }
    return instance_ ;
}

StreamApiClient::StreamApiClient(QObject* parent):
    QObject(parent),
    analyzerWidgets_(),
    registeredComponents_(),
    udpSocket_(new QUdpSocket(this)),
    port_(Config::get<unsigned int>("analysis.port").value_or(54322))
{
    setPort(port_);

    // stream client maintains analyzer widgets directly
    analyzerWidgets_[ComponentType::SpectrumAnalyzer] =
        new SpectrumAnalyzerWidget();

    analyzerWidgets_[ComponentType::Oscilloscope] = 
        new OscilloscopeWidget();

    // connections
    connect(
        udpSocket_, &QUdpSocket::readyRead, 
        this, &StreamApiClient::onReadyRead
    );
    connect(
        ComponentManager::instance(), &ComponentManager::componentAdded,
        this, &StreamApiClient::onComponentAdded
    );

    connect(
        ComponentManager::instance(), &ComponentManager::componentRemoved,
        this, &StreamApiClient::onComponentRemoved
    );
}

void StreamApiClient::setPort(quint16 port){
    if ( udpSocket_->state() == QAbstractSocket::BoundState ){
        udpSocket_->close();
    }

    port_ = port ;

    if ( !udpSocket_->bind(QHostAddress::LocalHost, port_)){
        SPDLOG_WARN("Failed to bind UDP socket to port", port_);
    } else {
        SPDLOG_INFO("Spectrum analyzer listening on UDP port {}", port_);
    }
}

QWidget* StreamApiClient::getAnalyzerWidget(ComponentType typ) const {
    if ( analyzerWidgets_.contains(typ) ){
        return dynamic_cast<QWidget*>(analyzerWidgets_.at(typ));
    }
    return nullptr ;
}   

std::vector<ComponentType> StreamApiClient::getAnalyzerTypes() const {
    std::vector<ComponentType> output ;
    for ( const auto& [key, _] : analyzerWidgets_ ){
        output.push_back(key);
    }
    return output ;
}

void StreamApiClient::destroy(){
    while ( instance_->analyzerWidgets_.size() > 0 ){
        auto it = instance_->analyzerWidgets_.begin();
        delete it->second ;
        instance_->analyzerWidgets_.erase(it->first);
    }
    delete instance_ ;
    instance_ = nullptr ;
}

void StreamApiClient::onReadyRead(){
    static const qsizetype headerSize = sizeof(DataApiHeader);
    while ( udpSocket_->hasPendingDatagrams() ){
        QByteArray datagram ;

        datagram.resize(udpSocket_->pendingDatagramSize());
        udpSocket_->readDatagram(datagram.data(), datagram.size());

        DataApiHeader header ;
        std::memcpy(&header, datagram.data(), headerSize);

        const float* data = reinterpret_cast<const float*>(datagram.data() + headerSize);
        size_t count = (datagram.size() - headerSize) / sizeof(float);

        // if it's a registered analyzer component
        if ( registeredComponents_.contains(header.componentId) ){
            ComponentType typ = registeredComponents_.at(header.componentId);
            if ( !analyzerWidgets_.contains(typ) ) return ;
            analyzerWidgets_.at(typ)->onData(header.componentId, data, count);
            return ;
        }

        // otherwise, just append the data to the relevant component model
        auto* model = ComponentManager::instance()->getModel(header.componentId);
        if ( !model ) return ;
        model->appendBuffer(header.channel, data, count);
    }
}

void StreamApiClient::onComponentAdded(int componentId, ComponentType typ){
    if ( registeredComponents_.contains(componentId) ) return ;
    if ( !analyzerWidgets_.contains(typ) ) return ;

    registeredComponents_[componentId] = typ ;
    analyzerWidgets_.at(typ)->addLayer(
        componentId, 
        QString::fromStdString(ComponentRegistry::getComponentDescriptor(typ).name)
    );
}

void StreamApiClient::onComponentRemoved(int componentId){
    if ( !registeredComponents_.contains(componentId) ) return ;
    
    ComponentType typ = registeredComponents_.at(componentId);
    registeredComponents_.erase(componentId);

    if ( !analyzerWidgets_.contains(typ) ) return ;
    analyzerWidgets_.at(typ)->removeLayer(componentId);

}

void StreamApiClient::onComponentRename(int componentId){
    if ( !registeredComponents_.contains(componentId) ) return ;

    ComponentType typ = registeredComponents_.at(componentId);

    analyzerWidgets_.at(typ)->renameLayer(
        componentId, 
        ComponentManager::instance()->getModel(componentId)->getName()
    );
}