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

#include "graphics/ToastNotification.hpp"
#include "app/Theme.hpp"

void ToastNotification::show(QGraphicsScene* scene, QGraphicsView* view, const QString& message){
    auto toast = new ToastNotification(message);
    scene->addItem(toast);
    toast->reposition(view);
    toast->popup();
}

ToastNotification::ToastNotification(const QString& message):
        QGraphicsItem(nullptr),
        opacity_(1.0f),
        message_(message)
{
    setZValue(9999);
    
    font_.setPointSize(Theme::TOAST_NOTIFICATION_FONT_SIZE);
    QFontMetrics fm(font_);
    width_ = fm.horizontalAdvance(message) + Theme::TOAST_NOTIFICATION_PADDING_H * 2 ;
    height_ = fm.height() + Theme::TOAST_NOTIFICATION_PADDING_V * 2 ;
    setFlag(QGraphicsItem::ItemIgnoresTransformations);
}

float ToastNotification::toastOpacity() const {
    return opacity_ ;
}

void ToastNotification::setToastOpacity(float o){
    opacity_ = o ;
    update();
}

QRectF ToastNotification::boundingRect() const {
    return QRectF(0, 0, width_, height_);
}

void ToastNotification::reposition(QGraphicsView* view){
    QPointF topCenter = view->mapToScene(
        view->viewport()->width() / 2 - width_ / 2,
        Theme::TOAST_NOTIFICATION_MARGIN
    );
    setPos(topCenter);
}

void ToastNotification::popup(){
    QGraphicsItem::show();

    // linger then fade
    QTimer::singleShot(
        Theme::TOAST_NOTIFICATION_DURATION, this, [this]() 
    {
        auto* anim = new QPropertyAnimation(
            this, 
            "toastOpacity", 
            this
        );
        anim->setDuration(Theme::TOAST_NOTIFICATION_FADE_DURATION);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        anim->setEasingCurve(QEasingCurve::InQuad);
        connect(
            anim, &QPropertyAnimation::finished, 
            this, [this]()
        {
            scene()->removeItem(this);
            delete this ;
        });
        anim->start();
    });
}


void ToastNotification::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*){
    painter->setRenderHint(QPainter::Antialiasing);

    QColor bg = Theme::TOAST_NOTIFICATION_BG ;
    bg.setAlpha(static_cast<int>(opacity_ * Theme::TOAST_NOTIFICATION_BG_MAX_ALPHA));
    painter->setBrush(bg);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(
        boundingRect(),
        Theme::TOAST_NOTIFICATION_CORNER_RADIUS,
        Theme::TOAST_NOTIFICATION_CORNER_RADIUS
    );

    QColor txt = Theme::TOAST_NOTIFICATION_TEXT ;
    txt.setAlpha(static_cast<int>(opacity_ * 255));
    painter->setPen(txt);
    painter->setFont(font_);
    painter->drawText(boundingRect(), Qt::AlignCenter, message_);
}