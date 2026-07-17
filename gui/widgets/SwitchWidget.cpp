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

#include "widgets/SwitchWidget.hpp"
#include "app/Theme.hpp"

#include <QPainter>
#include <qevent.h>

SwitchWidget::SwitchWidget(QWidget* parent): 
    QAbstractButton(parent), 
    thumbPosition_(0.0), 
    animation_(new QPropertyAnimation(this, "thumbPosition", this))
{
    setCheckable(true);
    animation_->setDuration(Theme::SWITCH_WIDGET_ANIMATION_DURATION);
    animation_->setEasingCurve(QEasingCurve::InOutQuad);
}

QSize SwitchWidget::sizeHint() const {
    int width = Theme::SWITCH_WIDGET_HEIGHT * 2 ;
    int height = Theme::SWITCH_WIDGET_HEIGHT ;
    return QSize(width, height);
}

void SwitchWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Calculate dimensions
    int trackWidth = Theme::SWITCH_WIDGET_HEIGHT * 2 ;
    int trackHeight = Theme::SWITCH_WIDGET_HEIGHT ;
    int thumbSize = trackHeight - 2 * Theme::SWITCH_WIDGET_MARGIN ;
    
    // Draw track
    QRectF trackRect(0, 0, trackWidth, trackHeight);
    
    if (!isEnabled()) {
        p.setOpacity(Theme::SWITCH_WIDGET_OPACITY_DISABLED);
        p.setBrush(Theme::SWITCH_WIDGET_DISABLED_COLOR);
    } else if (isChecked()) {
        p.setOpacity(Theme::SWITCH_WIDGET_OPACITY_ON);
        p.setBrush(Theme::SWITCH_WIDGET_ON_COLOR);
    } else {
        p.setOpacity(Theme::SWITCH_WIDGET_OPACITY_OFF);
        p.setBrush(Theme::SWITCH_WIDGET_OFF_COLOR);
    }
    
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(trackRect, Theme::SWITCH_WIDGET_CORNER_ROUND, 
                      Theme::SWITCH_WIDGET_CORNER_ROUND);
    
    // Draw thumb
    p.setOpacity(1.0);
    qreal thumbX = Theme::SWITCH_WIDGET_MARGIN + 
                   thumbPosition_ * (trackWidth - thumbSize - 2 * Theme::SWITCH_WIDGET_MARGIN);
    qreal thumbY = Theme::SWITCH_WIDGET_MARGIN ;
    
    QRectF thumbRect(thumbX, thumbY, thumbSize, thumbSize);
    
    if (isEnabled()) {
        p.setBrush(isChecked() ? Theme::SWITCH_WIDGET_THUMB_COLOR_ON 
                                : Theme::SWITCH_WIDGET_THUMB_COLOR_OFF);
    } else {
        p.setBrush(Theme::SWITCH_WIDGET_DISABLED_COLOR);
    }
    
    p.drawEllipse(thumbRect);
}

void SwitchWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && isEnabled() ) {
        toggle();
        event->accept();
        return ;
    }
    QAbstractButton::mouseReleaseEvent(event);
}

void SwitchWidget::enterEvent(QEnterEvent* event) {
    setCursor(Qt::PointingHandCursor);
    QAbstractButton::enterEvent(event);
}

void SwitchWidget::checkStateSet() {
    QAbstractButton::checkStateSet();
    
    animation_->stop();
    animation_->setStartValue(thumbPosition_);
    animation_->setEndValue(isChecked() ? 1.0 : 0.0);
    animation_->start();
}

