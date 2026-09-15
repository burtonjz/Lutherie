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

#include "widgets/ToastNotification.hpp"
#include "app/Theme.hpp"
#include "app/Synth.hpp"

void ToastNotification::show(const QString& message, QWidget* parent){
    if ( !parent ) parent = QApplication::activeWindow();
    if ( !parent ) parent = Synth::current();
    auto toast = new ToastNotification(parent, message);
    toast->reposition(parent);
    toast->popup();
}

ToastNotification::ToastNotification(QWidget* parent, const QString& message):
    QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
    opacity_(1.0f),
    message_(message)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);

    font_.setPointSize(Theme::TOAST_NOTIFICATION_FONT_SIZE);
    QFontMetrics fm(font_);

    width_ = fm.horizontalAdvance(message) + Theme::TOAST_NOTIFICATION_PADDING_H * 2 ;
    height_ = fm.height() + Theme::TOAST_NOTIFICATION_PADDING_V * 2 ;
    resize(width_, height_);
}

float ToastNotification::toastOpacity() const {
    return opacity_ ;
}

void ToastNotification::setToastOpacity(float o){
    opacity_ = o ;
    update();
}

void ToastNotification::reposition(QWidget* parent){
    QPoint topCenter = parent->mapToGlobal(
    QPoint(parent->width() / 2 - width_ / 2, Theme::TOAST_NOTIFICATION_MARGIN)
        );
    move(topCenter);
}

void ToastNotification::popup(){
    QWidget::show();

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
    close();
            });
    anim->start();
        });
}


void ToastNotification::paintEvent(QPaintEvent*){
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bg = Theme::TOAST_NOTIFICATION_BG ;
    bg.setAlpha(static_cast<int>(opacity_ * Theme::TOAST_NOTIFICATION_BG_MAX_ALPHA));
    painter.setBrush(bg);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(
    rect(),
    Theme::TOAST_NOTIFICATION_CORNER_RADIUS,
    Theme::TOAST_NOTIFICATION_CORNER_RADIUS
        );

    QColor txt = Theme::TOAST_NOTIFICATION_TEXT ;
    txt.setAlpha(static_cast<int>(opacity_ * 255));
    painter.setPen(txt);
    painter.setFont(font_);
    painter.drawText(rect(), Qt::AlignCenter, message_);
}