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

#ifndef CONTROL_PANEL_HPP_
#define CONTROL_PANEL_HPP_

#include "widgets/CollapsibleEditor.hpp"

#include <QScrollArea>
#include <QVBoxLayout>

class ControlPanel : public QScrollArea {
    Q_OBJECT

private:
    QWidget* container_ ;
    QVBoxLayout* layout_ ;
    std::vector<CollapsibleEditor*> sections_ ;

public:
    explicit ControlPanel(QWidget* parent = nullptr);

    void addContent(const QString& title, QWidget* content);
    void removeContent(QWidget* content);
    bool hasContent(QWidget* content) const ;

    void clear();

    const std::vector<CollapsibleEditor*>& getSections() const ;
};

#endif // CONTROL_PANEL_HPP_

