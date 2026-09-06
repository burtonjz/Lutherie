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

#ifndef TEXT_FORMAT_HPP_
#define TEXT_FORMAT_HPP_

#include <QFont>

struct TextFormat {
    enum class FontSize {
        Title,
        Header,
        Body
    };

    bool bolded = false ;
    bool italicized = false ;
    bool underlined = false ;
    FontSize fontSize = FontSize::Body ;
    QFont font = QFont() ;

    bool operator==(const TextFormat&) const = default ;
    bool operator!=(const TextFormat&) const = default ;
};

#endif // TEXT_FORMAT_HPP_