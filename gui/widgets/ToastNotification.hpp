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

#ifndef TOAST_NOTIFICATION_HPP_
#define TOAST_NOTIFICATION_HPP_

#include <QObject>
#include <QGraphicsItem>
#include <QTimer>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsView>

class ToastNotification : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_PROPERTY(float toastOpacity READ toastOpacity WRITE setToastOpacity)

private:
    float opacity_ ;
    QString message_ ;
    QFont font_ ;
    int width_ ;
    int height_ ;

public:
    static void show(QGraphicsScene* scene, QGraphicsView* view, const QString& message);

    float toastOpacity() const ;
    void setToastOpacity(float o);

    QRectF boundingRect() const override ;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override ;
    
private:
    ToastNotification(const QString& message);

    void reposition(QGraphicsView* view);
    void popup();

};

#endif // TOAST_NOTIFICATION_HPP_