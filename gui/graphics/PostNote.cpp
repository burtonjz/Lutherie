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

#include "graphics/PostNote.hpp"
#include "app/Theme.hpp"

#include <QTextCursor>
#include <QPainter>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include <QGridLayout>
#include <QGraphicsScene>
#include <QTimer>
#include <QDesktopServices>
#include <QAbstractTextDocumentLayout>

#include <spdlog/spdlog.h>

PostNote::PostNote(QGraphicsItem* parent):
    QGraphicsTextItem(parent),
    width_(Theme::POST_NOTE_MIN_WIDTH)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsFocusable);
    setAcceptHoverEvents(true);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setDefaultTextColor(Theme::TEXT_PRIMARY);
    document()->setDocumentMargin(8.0);

    bgColor_ = Theme::POST_NOTE_COLORS[0] ;

    setTextWidth(width_);
    setPlainText(" ");
}

void PostNote::setBackgroundColor(const QColor& color){
    if ( bgColor_ == color ) return ;
    bgColor_ = color ;
    update();
}

void PostNote::startEditing(){
    editing_ = true ;
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setFocus(Qt::MouseFocusReason);
}

void PostNote::stopEditing(){
    editing_ = false ;
    setTextInteractionFlags(Qt::NoTextInteraction);
    auto cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
    clearFocus();
}

bool PostNote::isEditing() const {
    return editing_ ;
}

void PostNote::insertHyperlink(const QString& url, const QString& display){
    QTextCursor cursor = textCursor();

    QTextCharFormat originalFormat = cursor.charFormat();

    QTextCharFormat linkFormat ;
    linkFormat.setAnchor(true);
    linkFormat.setAnchorHref(url);
    linkFormat.setForeground(Theme::ACCENT_COLOR);
    linkFormat.setFontUnderline(true);

    if ( cursor.hasSelection() ){
        cursor.mergeCharFormat(linkFormat);
    } else {
        cursor.insertText(display.isEmpty() ? url : display, linkFormat);
    }

    cursor.setCharFormat(originalFormat);
    setTextCursor(cursor);
}

void PostNote::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget){
    // draw bg, then let base class handle rest.
    painter->setRenderHint(QPainter::Antialiasing);

    if ( isSelected() ){   
        painter->setPen(QPen(
            Theme::COMPONENT_BORDER_SELECTED, 
            Theme::COMPONENT_HIGHLIGHT_WIDTH, Qt::SolidLine
        ));
    } else {
        painter->setPen(QPen(
            bgColor_.lighter(150),
            Theme::COMPONENT_BORDER_WIDTH, Qt::SolidLine
        ));
    }
    
    double w = painter->pen().width();
    QRectF rect = boundingRect().adjusted(w,w,-w,-w);

    painter->setBrush(bgColor_);
    painter->drawRoundedRect(rect, 4, 4);

    // strip out the default selection border
    QStyleOptionGraphicsItem opt(*option);
    opt.state &= ~QStyle::State_Selected ;
    opt.state &= ~QStyle::State_HasFocus ;

    QGraphicsTextItem::paint(painter, &opt, widget);
}

json PostNote::serialize() const {
    json msg ;
    msg["background_color"] = bgColor_.name().toStdString();
    msg["xpos"] = pos().x();
    msg["ypos"] = pos().y();
    msg["text"] = document()->toHtml().toStdString();

    return msg ;
}

void PostNote::deserialize(const json& msg){
    if ( msg.contains("background_color") ){
        bgColor_ = QColor(QString::fromStdString(msg.at("background_color")));
    }

    if ( msg.contains("xpos") && msg.contains("ypos") ){
        setPos(msg.at("xpos"), msg.at("ypos"));
    }

    if ( msg.contains("text") ){
        document()->setHtml(QString::fromStdString(msg.at("text")));
    }
}

void PostNote::mousePressEvent(QGraphicsSceneMouseEvent* event){
    if ( textInteractionFlags() & Qt::TextEditable ){
        QString anchor = document()->documentLayout()->anchorAt(event->pos());
        if ( !anchor.isEmpty() ){
            QDesktopServices::openUrl(QUrl(anchor));
            event->accept();
            return ;
        }
    }
    QGraphicsTextItem::mousePressEvent(event);
}

void PostNote::hoverMoveEvent(QGraphicsSceneHoverEvent* event){
    QString anchor = document()->documentLayout()->anchorAt(event->pos());
    setCursor(anchor.isEmpty() ? Qt::IBeamCursor : Qt::PointingHandCursor);
    QGraphicsTextItem::hoverMoveEvent(event);
}

void PostNote::contextMenuEvent(QGraphicsSceneContextMenuEvent* event){
    QMenu menu ;
    
    for ( const QColor& color : Theme::POST_NOTE_COLORS ){
        QPixmap pixmap(12, 12);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(0,0,0,60), 1));
        painter.setBrush(color);
        painter.drawRoundedRect(pixmap.rect().adjusted(1,1,-1,-1), 3,3);
        QAction* action = menu.addAction(QIcon(pixmap), "");
        connect(action, &QAction::triggered, this, [this, color]{
            setBackgroundColor(color);
        });
    }

    menu.exec(event->screenPos());

}

void PostNote::toggleBold(){
    QTextCursor cursor = textCursor();
    QTextCharFormat fmt ;
    bool isBold = cursor.charFormat().fontWeight() == QFont::Bold ;
    fmt.setFontWeight(isBold ? QFont::Normal : QFont::Bold);
    cursor.mergeCharFormat(fmt);
    setTextCursor(cursor);
}

void PostNote::toggleItalic(){
    QTextCursor cursor = textCursor();
    QTextCharFormat fmt ;
    fmt.setFontItalic(!cursor.charFormat().fontItalic());
    cursor.mergeCharFormat(fmt);
    setTextCursor(cursor);
}

void PostNote::toggleUnderline(){
    QTextCursor cursor = textCursor();
    QTextCharFormat fmt ;
    fmt.setFontUnderline(!cursor.charFormat().fontUnderline());
    cursor.mergeCharFormat(fmt);
    setTextCursor(cursor);
}

void PostNote::onFontChanged(const QFont& font){
    QTextCursor cursor = textCursor();
    QTextCharFormat fmt ;
    fmt.setFontFamilies(font.families());
    cursor.mergeCharFormat(fmt);
    setTextCursor(cursor);
}

void PostNote::applyTextStyle(TextStyle style){
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()){
        cursor.select(QTextCursor::LineUnderCursor); 
    }

    QTextCharFormat fmt;
    switch (style){
        case TextStyle::Title:
            fmt.setFontPointSize(20);
            fmt.setFontWeight(QFont::Bold);
            break ;
        case TextStyle::Header:
            fmt.setFontPointSize(15);
            fmt.setFontWeight(QFont::DemiBold);
            break ;
        case TextStyle::Body:
            fmt.setFontPointSize(11);
            fmt.setFontWeight(QFont::Normal);
            break ;
    }
    cursor.mergeCharFormat(fmt);
    setTextCursor(cursor);
    
    // combo boxes have quirky focus logic so deferring this resolves weird bugs
    QTimer::singleShot(0, this, [this](){
        setFocus(Qt::OtherFocusReason);
    });
}