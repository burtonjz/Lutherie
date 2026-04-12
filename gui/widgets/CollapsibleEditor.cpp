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

#include "widgets/CollapsibleEditor.hpp"
#include "app/Theme.hpp"

#include <QEvent>

CollapsibleEditor::CollapsibleEditor(QString title, QWidget* content, QWidget* parent):
    QWidget(parent),
    layout_(new QVBoxLayout(this)),
    header_(new QWidget(this)),
    headerLine_(new QFrame(this)),
    expandButton_(new QToolButton(header_)),
    content_(content),
    titleLabel_(new QLabel(this)),
    collapsed_(false)
{
    content_->setParent(this);
    setupLayout();
    titleLabel_->setText(title);
}

QWidget* CollapsibleEditor::getContent() const {
    return content_ ;
}

QString CollapsibleEditor::getTitle() const {
    return titleLabel_->text();
}

void CollapsibleEditor::setTitle(QString name){
    titleLabel_->setText(name);
}

bool CollapsibleEditor::isCollapsed() const {
    return collapsed_ ;
}

void CollapsibleEditor::setCollapsed(bool collapsed){
    if ( collapsed_ == collapsed) return ;

    collapsed_ = collapsed ;
    content_->setVisible(!collapsed_);
    expandButton_->setArrowType(collapsed_ ? Qt::RightArrow : Qt::DownArrow);
    updateGeometry();
    if ( parentWidget() ) parentWidget()->updateGeometry();
}

void CollapsibleEditor::setIndent(int level){
    auto headerLayout = dynamic_cast<QHBoxLayout*>(header_->layout());
    if ( !headerLayout ) return ;

    auto indent = Theme::COLLAPSIBLE_EDITOR_MARGIN + 
        Theme::COLLAPSIBLE_EDITOR_INDENT * level ;

    headerLayout->setContentsMargins(
        indent,
        Theme::COLLAPSIBLE_EDITOR_MARGIN,
        Theme::COLLAPSIBLE_EDITOR_MARGIN,
        Theme::COLLAPSIBLE_EDITOR_MARGIN
    );

    headerLine_->setContentsMargins(indent, 0, 0, 0);
}

bool CollapsibleEditor::eventFilter(QObject* watched, QEvent* event){
    if ( watched == header_ && event->type() == QEvent::MouseButtonRelease ){
        toggle();
        return true ;
    }
    return QWidget::eventFilter(watched, event);
}

QSize CollapsibleEditor::sizeHint() const {
    if ( !content_->isVisible() ){
        return QSize(
            QWidget::sizeHint().width(), 
            header_->sizeHint().height() + headerLine_->sizeHint().height()
        );
    }
    return QWidget::sizeHint();
}

QSize CollapsibleEditor::minimumSizeHint() const {
    return sizeHint();
}

void CollapsibleEditor::setupLayout(){
    layout_->setContentsMargins(0,0,0,0);
    layout_->setSpacing(0);

    header_->setCursor(Qt::PointingHandCursor);
    header_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto headerLayout = new QHBoxLayout();
    header_->setLayout(headerLayout);

    headerLayout->setContentsMargins(
        Theme::COLLAPSIBLE_EDITOR_MARGIN,
        Theme::COLLAPSIBLE_EDITOR_MARGIN,
        Theme::COLLAPSIBLE_EDITOR_MARGIN,
        Theme::COLLAPSIBLE_EDITOR_MARGIN
    );
    headerLayout->setSpacing(Theme::COLLAPSIBLE_EDITOR_MARGIN);

    expandButton_->setArrowType(Qt::DownArrow);
    expandButton_->setAutoRaise(true);
    expandButton_->setFixedSize(
        Theme::TOOL_BUTTON_SIZE, 
        Theme::TOOL_BUTTON_SIZE
    );
    connect(
        expandButton_, &QToolButton::clicked, 
        this, &CollapsibleEditor::toggle
    );

    headerLayout->addWidget(expandButton_);
    headerLayout->addWidget(titleLabel_);
    header_->installEventFilter(this);

    headerLine_->setFrameShape(QFrame::HLine);
    headerLine_->setStyleSheet(
        "color: " + 
        Theme::COMPONENT_BORDER.name() + ";"
    );
    headerLine_->setContentsMargins(
        Theme::COLLAPSIBLE_EDITOR_MARGIN,
        0, 0, 0
    );

    layout_->addWidget(header_);
    layout_->addWidget(headerLine_);
    layout_->addWidget(content_);

    setLayout(layout_);
}

void CollapsibleEditor::toggle(){
    setCollapsed(!collapsed_);
}