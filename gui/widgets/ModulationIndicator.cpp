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

#include "widgets/ModulationIndicator.hpp"
#include "app/Theme.hpp"

#include <QPainter>
#include <QPen>
#include <QStyleOption>

ModulationIndicator::ModulationIndicator(QWidget* parent): 
    QWidget(parent), active_(false)
{
    setFixedSize(Theme::MODULATION_INDICATOR_SIZE, Theme::MODULATION_INDICATOR_SIZE);

    setFixedSize(sizeHint());
}

void ModulationIndicator::setActive(bool active){
    if ( active_ == active ) return;
    active_ = active ;
    update();
}

bool ModulationIndicator::isActive() const { 
    return active_ ; 
}

void ModulationIndicator::paintEvent(QPaintEvent* e){
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    
    int indicatorSize = Theme::MODULATION_INDICATOR_SIZE ;
    qreal ringWidth = Theme::MODULATION_INDICATOR_RING_WIDTH ;
    QPointF center = rect().center();
    qreal radius = indicatorSize / 2.0f ;
    int glowRadius = Theme::MODULATION_INDICATOR_GLOW_RADIUS ;
    qreal fullRadius = radius + glowRadius ;
    if ( active_ ){
        QColor glowColor = Theme::MODULATION_ACTIVE ;
        glowColor.setAlpha(140);

        const float innerGlowStart = 0.0f ;
        const float innerGlowEnd = (radius - ringWidth) / fullRadius ;
        const float outerGlowStart = radius / fullRadius ;
        const float outerGlowEnd = 1.0 ;

        QRadialGradient glow(center, radius + glowRadius);
        glow.setColorAt(innerGlowStart, Qt::transparent);
        glow.setColorAt(innerGlowEnd - 0.01f, glowColor);
        glow.setColorAt(innerGlowEnd, Theme::MODULATION_ACTIVE);
        glow.setColorAt(outerGlowStart - 0.01f, Theme::MODULATION_ACTIVE);
        glow.setColorAt(outerGlowStart, glowColor);
        glow.setColorAt(outerGlowEnd, Qt::transparent);
        painter.fillRect(rect(), glow);
    } else {
        // just draw ring
        painter.setPen(QPen(Theme::MODULATION_INACTIVE, ringWidth));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(center, radius - ringWidth, radius - ringWidth);
    }
}

QSize ModulationIndicator::sizeHint() const {
    int size = Theme::MODULATION_INDICATOR_SIZE +
        Theme::MODULATION_INDICATOR_GLOW_RADIUS * 2 ;
    return QSize(size, size);
}