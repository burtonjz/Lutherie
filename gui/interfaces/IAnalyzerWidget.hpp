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

#ifndef INTERFACE_ANALYZER_WIDGET_HPP_
#define INTERFACE_ANALYZER_WIDGET_HPP_

#include <QString>
class IAnalyzerWidget {
public:
    virtual ~IAnalyzerWidget() = default ;
    virtual void addLayer(int componentId, const QString& label) = 0 ;
    virtual void removeLayer(int componentId) = 0 ;
    virtual void renameLayer(int componentId, const QString& label) = 0 ;
    virtual void onData(int componentId, const float* data, size_t count) = 0 ;
};


#endif // INTERFACE_ANALYZER_WIDGET_HPP_