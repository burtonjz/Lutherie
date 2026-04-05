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

#ifndef WHEEL_GUARD_HPP_
#define WHEEL_GUARD_HPP_

#include <QObject>
#include <QEvent>
#include <QWidget>
#include <QApplication>

class WheelGuard : public QObject {
public:
    explicit WheelGuard(QObject* parent = nullptr):
        QObject(parent)
    {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if ( event->type() == QEvent::Wheel ){
            auto* w = qobject_cast<QWidget*>(obj);
            qDebug() << "Wheel event on:" << obj
                 << "hasFocus:" << (w ? w->hasFocus() : false)
                 << "focusWidget:" << QApplication::focusWidget();
            if ( w && QApplication::focusWidget() != w ){
                event->ignore();
                return true ;
            }
        }
        return false ;
    }
};

#endif // WHEEL_GUARD_HPP_