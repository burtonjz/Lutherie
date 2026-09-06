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

#include "interfaces/IToolbarTarget.hpp"

#include <QGraphicsTextItem>
#include <QColor>
#include <QGraphicsProxyWidget>
#include <QToolButton>
#include <QFontComboBox>
#include <QComboBox>

#include <nlohmann/json.hpp>

using json = nlohmann::json ;
class PostNote : public QGraphicsTextItem, public IToolbarTarget {
    Q_OBJECT

private:
    bool editing_ = false ;
    double width_ ;
    QColor bgColor_ ;
    TextFormat currentFormat_ ;

public:
    explicit PostNote(QGraphicsItem* parent = nullptr);

    void setBackgroundColor(const QColor& color);
    
    void startEditing();
    void stopEditing();

    void paint(
        QPainter* painter, 
        const QStyleOptionGraphicsItem* option, 
        QWidget* widget = nullptr
    ) override ;

    json serialize() const ;
    void deserialize(const json& msg);

    // IToolbarInterface
    bool editing() const override ;
    virtual QString selectedText() const override ;
    const TextFormat& textFormat() override ;
    void createHyperlink(const QString &url, const QString &display) override ;
    void applyTextFormat(const TextFormat &format) override ;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override ;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override ;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override ;
    void keyPressEvent(QKeyEvent* event) override ;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override ;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override ;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override ;

private:
    void syncFormatFromCursor();

signals:
    void requestStartEditing();
    void editingStarted(TextFormat format);
    void editingFinished();
    void formatUpdated(TextFormat format);

};

#endif // POST_NOTE_HPP_