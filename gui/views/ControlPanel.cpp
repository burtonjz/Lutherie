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

#include "views/ControlPanel.hpp"
#include "widgets/FileSelectorWidget.hpp"

ControlPanel::ControlPanel(QWidget* parent):
    QScrollArea(parent),
    container_(new QWidget(this)),
    layout_(new QVBoxLayout(container_))
{
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameShape(QFrame::NoFrame);

    container_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    
    layout_->setContentsMargins(0,0,0,0);
    layout_->setSpacing(1);
    layout_->addStretch();

    setWidget(container_);
}

void ControlPanel::addContent(const QString& title, QWidget* content){
    if ( hasContent(content) ) return ;

    auto section = new CollapsibleEditor(title, content, container_);
    
    const int stretchIndex = layout_->count() - 1 ;
    layout_->insertWidget(stretchIndex, section);
    sections_.push_back(section);

    setMinimumSize(layout_->minimumSize());
}

void ControlPanel::removeContent(QWidget* content){
    sections_.erase(
        std::remove_if(sections_.begin(), sections_.end(),
        [content, this](CollapsibleEditor* editor){
            bool match = editor->getContent() == content ;
            if ( match ){
                layout_->removeWidget(editor);
                editor->deleteLater();
            }
           return match ;
        }),
        sections_.end()
    );
    
}

bool ControlPanel::hasContent(QWidget* content) const {
    for ( auto s : sections_ ){
        if ( s->getContent() == content ){
            return true ;
        }
    }
    return false ;
}

void ControlPanel::clear(){
    const auto copy = sections_ ;
    for ( auto s : copy ){
        removeContent(s->getContent());
    }
}

const std::vector<CollapsibleEditor*>& ControlPanel::getSections() const {
    return sections_ ;
}

CollapsibleEditor* ControlPanel::getSection(QWidget* content) const {
    for ( auto s : sections_ ){
        if ( s->getContent() == content ){
            return s ;
        }
    }
    return nullptr ;
}

void ControlPanel::maximizeSection(QWidget* content){
    auto section = getSection(content);
    if ( ! section ) return ;
    section->setCollapsed(false);
}

void ControlPanel::minimizeSection(QWidget* content){
    auto section = getSection(content);
    if ( ! section ) return ;
    section->setCollapsed(true);
}

void ControlPanel::moveContent(QWidget* content, QWidget* destination){
    auto* section = getSection(content);
    if ( !section ) return ;

    if ( destination && !container_->isAncestorOf(destination) ){
        qWarning() << "Cannot move content: destination is not owned by this control panel" ;
        return ;
    }

    if ( destination && !destination->layout() ){
        qWarning() << "Cannot move content: destination has no layout" ;
        return ;
    }

    if ( section->parentWidget() == destination ) return ; // already nested, no action needed

    layout_->removeWidget(section);
    destination->layout()->addWidget(section);
    section->setParent(destination);
    section->setIndent(1);
}

void ControlPanel::promoteContent(QWidget* content){
    auto section = getSection(content);
    if ( !section ) return ;
    if ( section->parentWidget() == container_ ) return ;

    section->parentWidget()->layout()->removeWidget(section);
    const int stretchIndex = layout_->count() - 1 ;
    layout_->insertWidget(stretchIndex, section);
    section->setParent(container_);
    section->setIndent(0);
}