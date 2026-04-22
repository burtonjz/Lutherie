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

#include "managers/AnalysisManager.hpp"
#include "config/Config.hpp"
#include "meta/ComponentRegistry.hpp"

#include "widgets/SpectrumAnalyzerWidget.hpp"
#include "widgets/OscilloscopeWidget.hpp"

AnalysisManager::AnalysisManager(QObject* parent):
    QObject(parent),
    analyzerWidgets_(),
    registeredComponents_(),
    udpSocket_(new QUdpSocket(this)),
    port_(Config::get<unsigned int>("analysis.port").value_or(54322))
{
    setPort(port_);

    // create child widgets
    analyzerWidgets_[ComponentType::SpectrumAnalyzer] =
        new SpectrumAnalyzerWidget();

    analyzerWidgets_[ComponentType::Oscilloscope] = 
        new OscilloscopeWidget();

    // connections
    connect(
        udpSocket_, &QUdpSocket::readyRead, 
        this, &AnalysisManager::onReadyRead
    );
}

void AnalysisManager::setPort(quint16 port){
    if ( udpSocket_->state() == QAbstractSocket::BoundState ){
        udpSocket_->close();
    }

    port_ = port ;

    if ( !udpSocket_->bind(QHostAddress::LocalHost, port_)){
        qWarning() << "Failed to bind UDP socket to port" << port_ ;
    } else {
        qDebug() << "Spectrum analyzer listening on UDP port" << port_ ;
    }
}

QWidget* AnalysisManager::getAnalyzerWidget(ComponentType typ) const {
    if ( analyzerWidgets_.contains(typ) ){
        return dynamic_cast<QWidget*>(analyzerWidgets_.at(typ));
    }
    return nullptr ;
}   

std::vector<ComponentType> AnalysisManager::getAnalyzerTypes() const {
    std::vector<ComponentType> output ;
    for ( const auto& [key, _] : analyzerWidgets_ ){
        output.push_back(key);
    }
    return output ;
}

void AnalysisManager::onReadyRead(){
    while ( udpSocket_->hasPendingDatagrams() ){
        QByteArray datagram ;

        datagram.resize(udpSocket_->pendingDatagramSize());
        udpSocket_->readDatagram(datagram.data(), datagram.size());

        // parse incoming data.
        // header int32 component id, then array of floats
        int32_t componentId ;
        std::memcpy(&componentId, datagram.data(), sizeof(int32_t));
        const float* data = reinterpret_cast<const float*>(datagram.data() + sizeof(int32_t));
        size_t count = (datagram.size() - sizeof(int32_t)) / sizeof(float);

        if ( !registeredComponents_.contains(componentId) ) return ;

        ComponentType typ = registeredComponents_.at(componentId);

        if ( !analyzerWidgets_.contains(typ) ) return ;

        analyzerWidgets_.at(typ)->onData(componentId, data, count);
    }
}

void AnalysisManager::onComponentAdded(int componentId, ComponentType typ){
    if ( registeredComponents_.contains(componentId) ) return ;
    if ( !analyzerWidgets_.contains(typ) ) return ;

    registeredComponents_[componentId] = typ ;
    analyzerWidgets_.at(typ)->addLayer(
        componentId, 
        QString::fromStdString(ComponentRegistry::getComponentDescriptor(typ).name)
    );
}

void AnalysisManager::onComponentRemoved(int componentId){
    if ( !registeredComponents_.contains(componentId) ) return ;
    
    ComponentType typ = registeredComponents_.at(componentId);
    registeredComponents_.erase(componentId);

    if ( !analyzerWidgets_.contains(typ) ) return ;
    analyzerWidgets_.at(typ)->removeLayer(componentId);

}

void AnalysisManager::onComponentRename(int componentId, QString name){
    if ( !registeredComponents_.contains(componentId) ) return ;

    ComponentType typ = registeredComponents_.at(componentId);

    analyzerWidgets_.at(typ)->renameLayer(componentId, name);
}