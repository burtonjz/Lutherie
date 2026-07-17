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

 #ifndef RANGE_SLIDER_HPP_
 #define RANGE_SLIDER_HPP_

#include <QWidget>
#include <QPushButton>

class RangeSlider : public QWidget {
    Q_OBJECT

private:
    double minimum_ ;
    double maximum_ ;
    size_t precision_ ;

    double lower_ ;
    double higher_ ;

    double lowerPos_ ;
    double higherPos_ ;

    bool isDragging_ ;
    bool dragLower_ ;

    double handlePosY_ ;
    double textPosY_ ;

public:
    explicit RangeSlider(double absMin, double absMax, size_t precision, QWidget* parent = nullptr);
    explicit RangeSlider(
        double absMin, double absMax, 
        double curMin, double curMax,
        size_t precision, QWidget* parent = nullptr
    );
    
    QSize sizeHint() const override ;

    double getMinimum() const ;
    void setMinimum(double min);
    double getMaximum() const ;
    void setMaximum(double max);
    size_t getPrecision() const ;
    void setPrecision(size_t s);
    
protected:
    void paintEvent(QPaintEvent* event) override ;
    void mousePressEvent(QMouseEvent* e) override ;
    void mouseMoveEvent(QMouseEvent* e) override ;
    void mouseReleaseEvent(QMouseEvent* e) override ;

private:
    double valToPos(double val) const ;
    double posToVal(double posX) const ;

    void startDrag(QPointF pos);
    void updateDrag(QPointF pos);
    void finishDrag(QPointF pos);

signals:
    void rangeUpdated(double min, double max);
    
};

 #endif // RANGE_SLIDER_HPP_

