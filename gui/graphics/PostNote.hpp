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

#ifndef POST_NOTE_HPP_
#define POST_NOTE_HPP_

#include <QGraphicsTextItem>
#include <QColor>
#include <QGraphicsProxyWidget>
#include <QToolButton>
#include <QFontComboBox>
#include <QComboBox>

#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class PostNote : public QGraphicsTextItem {
    Q_OBJECT

private:
    bool editing_ = false ;
    double width_ ;
    QColor bgColor_ ;
    QToolButton* boldBtn_ ;
    QToolButton* italicBtn_ ;
    QToolButton* underlineBtn_ ;
    QFontComboBox* fontCombo_ ;
    QComboBox* styleCombo_ ;

public:
    explicit PostNote(QGraphicsItem* parent = nullptr);

    enum class TextStyle {Title, Header, Body};

    void setBackgroundColor(const QColor& color);

    void startEditing();
    void stopEditing();
    bool isEditing() const ;

    void insertHyperlink(const QString& url, const QString& display);

    void paint(
        QPainter* painter, 
        const QStyleOptionGraphicsItem* option, 
        QWidget* widget = nullptr
    ) override ;

    json serialize() const ;
    void deserialize(const json& msg);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override ;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override ;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override ;

public slots:
    void toggleBold();
    void toggleItalic();
    void toggleUnderline();
    void onFontChanged(const QFont& font);
    void applyTextStyle(TextStyle style);

};

#endif // POST_NOTE_HPP_