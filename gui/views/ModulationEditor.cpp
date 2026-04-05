// /*
//  * Copyright (C) 2026 Jared Burton
//  *
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU Lesser General Public License as published by
//  * the Free Software Foundation, either version 3 of the License, or
//  * (at your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  * GNU Lesser General Public License for more details.
//  *
//  * You should have received a copy of the GNU Lesser General Public License
//  * along with this program. If not, see <https://www.gnu.org/licenses/>.
//  */

// // std::map<std::pair<int, ParameterType>, ModulationControl*> modulationControls_ ;
// // std::vector<std::pair<int, ParameterType>> controlOrder_ ;

// // QLabel* editorLabel_ ;
// // QGridLayout* ctrlLayout_ ;

// #include "views/ModulationEditor.hpp"

// #include <QVBoxLayout>
// #include <QEvent>
// #include <QApplication>
// #include <QScrollBar>

// ModulationEditor::ModulationEditor(QString name, QWidget* parent):
//     QWidget(parent),
//     container_(new QWidget(this)),
//     gridContainer_(new QWidget()),
//     modulationControls_(),
//     controlOrder_(),
//     ctrlLayout_(new QGridLayout()),
//     closeButton_(new QPushButton("Close", container_))
// {    
//     setWindowTitle(name + "Modulation");
//     setupLayout();
//     close();

//     connect(
//         closeButton_, &QPushButton::clicked, 
//         this, &ModulationEditor::onCloseButtonClicked 
//     );
// }

// ModulationEditor::~ModulationEditor(){
//     for ( auto [k, v] : modulationControls_ ){
//         v->deleteLater();
//     }
// }

// void ModulationEditor::add(ModulationModel* model){
//     if ( !model ) return ;
    
//     auto key = std::make_pair(model->getId(),model->getType());
//     if ( modulationControls_.contains(key) ){
//         qWarning() << "modulation control for component " << key.first 
//             << " parameter " << GET_PARAMETER_TRAIT_MEMBER(key.second, name)
//             << " is already present in editor." ;
//         return ;
//     }

//     auto ctrl = new ModulationControl(key.first, key.second, this);

//     connect(
//         ctrl, &ModulationControl::modulationDepthEdited,
//         this, &ModulationEditor::modulationDepthEdited
//     );

//     connect(
//         ctrl, &ModulationControl::modulationStrategyEdited,
//         this, &ModulationEditor::modulationStrategyEdited
//     );

//     modulationControls_[key] = ctrl ;
//     controlOrder_.push_back(key);    
//     updateLayout();
// }

// void ModulationEditor::remove(int componentId, ParameterType p){
//     auto key = std::make_pair(componentId,p);

//     if ( !modulationControls_.contains(key) ){
//         qWarning() << "modulation control for component " << key.first 
//             << " parameter " << GET_PARAMETER_TRAIT_MEMBER(key.second, name)
//             << " is not present in editor and cannot be removed" ;
//         return ;
//     }
//     auto ctrl = modulationControls_.at(key);

//     modulationControls_.erase(key);
//     controlOrder_.erase(std::remove(
//         controlOrder_.begin(), controlOrder_.end(), key), 
//         controlOrder_.end()
//     );
//     ctrlLayout_->removeWidget(ctrl);
//     ctrl->deleteLater();
//     updateLayout();
// }

// void ModulationEditor::setName(const QString& name){
//     setWindowTitle(name + "Modulation");
// }

// void ModulationEditor::setModulationStatus(int componentId, ParameterType p, bool active){
//     auto key = std::make_pair(componentId, p);

//     if ( !modulationControls_.contains(key) ){
//         qWarning() << "modulation control for component " << key.first 
//             << " parameter " << GET_PARAMETER_TRAIT_MEMBER(key.second, name)
//             << " is not present in editor and cannot be removed" ;
//         return ;
//     }
//     auto ctrl = modulationControls_.at(key);
//     ctrl->setConnectionStatus(active);
// }

// void ModulationEditor::setupLayout(){
//     QVBoxLayout* mainLayout = new QVBoxLayout(container_);

//     gridContainer_->setLayout(ctrlLayout_);

//     scroll_ = new QScrollArea(container_);
//     scroll_->setWidget(gridContainer_);
//     scroll_->setWidgetResizable(true);
//     scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//     scroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

//     mainLayout->addWidget(scroll_);
//     mainLayout->addWidget(closeButton_);
// }

// void ModulationEditor::updateLayout(){
//     for ( const auto& [key, v] : modulationControls_ ){
//         ctrlLayout_->removeWidget(v);
//     }

//     int count = 0 ;
//     for ( const auto& key : controlOrder_ ){
//         int row = count / 2 ;
//         int col = count % 2 ;
//         ctrlLayout_->addWidget(modulationControls_.at(key), row, col);
//         count++ ;
//     }

//     if ( modulationControls_.empty() ) return ;

//     // handle sizing
//     auto controlSize = modulationControls_.begin()->second->sizeHint();
//     int nRows = (count + 1) / 2 ;
//     int vSpacing = ctrlLayout_->verticalSpacing();
//     int vMargins = ctrlLayout_->contentsMargins().top() +
//                    ctrlLayout_->contentsMargins().bottom() ;
//     int hSpacing = ctrlLayout_->horizontalSpacing();
//     int hMargins = ctrlLayout_->contentsMargins().left() +
//                    ctrlLayout_->contentsMargins().right() ;
    
//     int contentWidth = controlSize.width() * 2 + hSpacing + hMargins ;
//     int contentHeight = controlSize.height() * nRows + vSpacing * (nRows - 1) + vMargins ;

//     int scrollWidth = scroll_->verticalScrollBar()->sizeHint().width() + scroll_->frameWidth() * 2 ;
//     int totalWidth = scrollWidth + contentWidth ;

//     scroll_->setFixedWidth(totalWidth);
//     scroll_->setMaximumHeight(contentHeight);

//     adjustSize();
//     container_->setFixedWidth(container_->sizeHint().width());
//     container_->setMaximumHeight(
//         contentHeight + closeButton_->sizeHint().height()
//     );
// }

// void ModulationEditor::onCloseButtonClicked(){
//     close();
// }
