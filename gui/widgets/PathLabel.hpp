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

#ifndef PATH_LABEL_HPP_
#define PATH_LABEL_HPP_

#include <QLabel>

class PathLabel : public QLabel {
    Q_OBJECT
private:
    QString fullPath_ ;

public:
    explicit PathLabel(QWidget* parent = nullptr): 
        QLabel(parent) 
    {}

    void setPath(const QString& path){
        fullPath_ = path ;
        setToolTip(fullPath_);
        updateElided();
    }

    QString path() const { 
        return fullPath_ ; 
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        updateElided();
    }

private:
    void updateElided(){
        QFontMetrics metrics(font());
        QString elided = metrics.elidedText(fullPath_, Qt::ElideLeft, width());
        setText(elided);
    }

};

#endif // PATH_LABEL_HPP_