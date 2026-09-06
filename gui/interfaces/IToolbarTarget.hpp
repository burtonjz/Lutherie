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

#ifndef INTERFACE_TOOLBAR_TARGET_HPP_
#define INTERFACE_TOOLBAR_TARGET_HPP_

#include "util/TextFormat.hpp"

class IToolbarTarget {
public:
    virtual ~IToolbarTarget() = default;

    virtual bool editing() const = 0 ;
    virtual QString selectedText() const = 0 ;
    virtual const TextFormat& textFormat() = 0 ;

    virtual void createHyperlink(const QString &url, const QString &display) = 0 ;
    virtual void applyTextFormat(const TextFormat &format) = 0 ;
};

#endif // INTERFACE_TOOLBAR_TARGET_HPP_