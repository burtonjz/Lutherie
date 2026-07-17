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

#ifndef SWITCH_WIDGET_HPP_
#define SWITCH_WIDGET_HPP_

#include <QAbstractButton>
#include <QPropertyAnimation>

class SwitchWidget : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(qreal thumbPosition READ thumbPosition WRITE setThumbPosition)

private:
    qreal thumbPosition_ ;
    QPropertyAnimation* animation_ ;
public:
    explicit SwitchWidget(QWidget* parent = nullptr);
    
    QSize sizeHint() const override ;

protected:
    void paintEvent(QPaintEvent* event) override ;
    void mouseReleaseEvent(QMouseEvent* event) override ;
    void enterEvent(QEnterEvent* event) override ;
    void checkStateSet() override;

private:
    qreal thumbPosition() const { return thumbPosition_; }
    void setThumbPosition(qreal pos) { 
        thumbPosition_ = pos;
        update();
    }

};

#endif // SWITCH_WIDGET_HPP_