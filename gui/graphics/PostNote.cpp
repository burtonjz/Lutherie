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
#include "views/GraphPanel.hpp"

#include <QTextCursor>
#include <QPainter>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include <QGridLayout>
#include <QGraphicsScene>
#include <QTimer>
#include <QDesktopServices>
#include <QAbstractTextDocumentLayout>
#include <QToolTip>

#include <spdlog/spdlog.h>

PostNote::PostNote(QGraphicsItem* parent):
    QGraphicsTextItem(parent),
    width_(Theme::POST_NOTE_DEFAULT_WIDTH),
    currentFormat_()
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

void PostNote::startEdit(){
    editing_ = true ;
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setFocus(Qt::MouseFocusReason);
    syncFormatFromCursor();
    emit editStarted(currentFormat_);   
}

void PostNote::endEdit(){
    editing_ = false ;
    setTextInteractionFlags(Qt::NoTextInteraction);
    auto cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
    clearFocus();

    emit editFinished();
}

void PostNote::startResize(const QPointF pos){
    resizing_ = true ;

    resizeLeft_ = std::abs(pos.x()) < std::abs(pos.x() - width_);
}

void PostNote::updateResize(const QPointF pos){
    if ( resizeLeft_ ){
        width_ = std::max(width_ - pos.x(), Theme::POST_NOTE_MIN_WIDTH);
        if ( width_ != Theme::POST_NOTE_MIN_WIDTH ){
            moveBy(pos.x(),0.0);    
        }
    } else {
        width_ = std::max(pos.x(), Theme::POST_NOTE_MIN_WIDTH);
    }

    setTextWidth(width_);
}

void PostNote::endResize(){
    resizing_ = false ;
}


bool PostNote::editing() const {
    return editing_ ;
}

const TextFormat& PostNote::textFormat(){
    return currentFormat_ ; 
}

QString PostNote::selectedText() const {
    QTextCursor cursor = textCursor();
    QString selectedText = cursor.hasSelection() ? cursor.selectedText() : QString();
    return selectedText ;
}

void PostNote::createHyperlink(const QString& url, const QString& display){
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
    msg["hidden"] = !isVisible();
    msg["width"] = width_ ;

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

    if ( msg.contains("hidden") && msg.at("hidden").is_boolean() ){
        setVisible(!msg.at("hidden"));
    }

    if ( msg.contains("width") && msg.at("width").is_number() ){
        width_ = msg.at("width");
        setTextWidth(width_);
    }
}

void PostNote::mousePressEvent(QGraphicsSceneMouseEvent* event){
    // hyperlink handling
    if ( textInteractionFlags() & Qt::TextEditable ){
        QString anchor = document()->documentLayout()->anchorAt(event->pos());
        if ( !anchor.isEmpty() ){
            QDesktopServices::openUrl(QUrl(anchor));
            event->accept();
            return ;
        }
    }

    if ( !editing_ && !resizing_ && cursor() == Qt::SizeHorCursor ){
        startResize(event->pos());
        event->accept();
        return ;
    }

    QGraphicsTextItem::mousePressEvent(event);
    if ( editing_ ) syncFormatFromCursor();
}

void PostNote::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event){
    if ( !editing_ ){
        emit requestStartEdit();
        event->accept();
        return ;
    }
    
    QGraphicsTextItem::mouseDoubleClickEvent(event);
    syncFormatFromCursor();
}

void PostNote::mouseMoveEvent(QGraphicsSceneMouseEvent* event){
    if ( resizing_ ){
        updateResize(event->pos());
    }

    QGraphicsTextItem::mouseMoveEvent(event);
}

void PostNote::mouseReleaseEvent(QGraphicsSceneMouseEvent* event){
    if ( resizing_ ){
        endResize();
        event->accept();
        return ;
    }

    QGraphicsTextItem::mouseReleaseEvent(event);
    if ( editing_ ) syncFormatFromCursor();
}

void PostNote::keyPressEvent(QKeyEvent* event){
    if ( editing_ && event->key() == Qt::Key_Escape ){
            endEdit();
            event->accept();
            return ;
        }
    
    if ( event->modifiers() & Qt::ControlModifier ){
        TextFormat fmt = currentFormat_ ;
        switch (event->key()){
        case Qt::Key_B:
            fmt.bolded = !currentFormat_.bolded ;
            applyTextFormat(fmt);
            emit formatUpdated(currentFormat_);
            event->accept();
            return ;
        case Qt::Key_I:
            fmt.italicized = !currentFormat_.italicized ;
            applyTextFormat(fmt);
            emit formatUpdated(currentFormat_);
            event->accept();
            return ;
        case Qt::Key_U:
            fmt.underlined = !currentFormat_.underlined ;
            applyTextFormat(fmt);
            emit formatUpdated(currentFormat_);
            event->accept();
            return ;
        default:
            break ;
        }
    }

    QGraphicsTextItem::keyPressEvent(event);
    
    // handle navigation key formats (after key press so cursor resolves)
    switch (event->key()){
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        syncFormatFromCursor();
        break ;
    default:
        break ;
    }
}

void PostNote::hoverMoveEvent(QGraphicsSceneHoverEvent* event){
    // hyperlink hover
    QString anchor = document()->documentLayout()->anchorAt(event->pos());
    if ( anchor.isEmpty() ){
        setCursor(Qt::IBeamCursor);
        QToolTip::hideText();
    } else {
        setCursor(Qt::PointingHandCursor);
        QToolTip::showText(event->screenPos(), anchor);
    }

    // resize
    if ( !editing_ && !resizing_ ){
        int dist = std::min(
            abs(event->pos().x() - boundingRect().left()),
            abs(event->pos().x() - boundingRect().right())
        );
        if ( dist < Theme::POST_NOTE_EDGE_THRESHOLD ){
            setCursor(Qt::SizeHorCursor);
        } else {
            unsetCursor();
        }
    }

    QGraphicsTextItem::hoverMoveEvent(event);
}

void PostNote::hoverLeaveEvent(QGraphicsSceneHoverEvent* event){
    QToolTip::hideText();
    QGraphicsTextItem::hoverLeaveEvent(event);
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

void PostNote::applyTextFormat(const TextFormat& format){
    if ( format.bolded != currentFormat_.bolded ){
        QTextCursor cursor = textCursor();
        QTextCharFormat fmt ;
        fmt.setFontWeight(format.bolded ? QFont::Bold : QFont::Normal);
        cursor.mergeCharFormat(fmt);
        setTextCursor(cursor);
    }

    if ( format.italicized != currentFormat_.italicized ){
        QTextCursor cursor = textCursor();
        QTextCharFormat fmt ;
        fmt.setFontItalic(format.italicized);
        cursor.mergeCharFormat(fmt);
        setTextCursor(cursor);
    }

    if ( format.underlined != currentFormat_.underlined ){
        QTextCursor cursor = textCursor();
        QTextCharFormat fmt ;
        fmt.setFontUnderline(format.underlined);
        cursor.mergeCharFormat(fmt);
        setTextCursor(cursor);
    }

    if ( format.font != currentFormat_.font ){
        QTextCursor cursor = textCursor();
        QTextCharFormat fmt ;
        fmt.setFontFamilies(format.font.families());
        cursor.mergeCharFormat(fmt);
        setTextCursor(cursor);
    }

    if ( format.fontSize != currentFormat_.fontSize ){
        QTextCharFormat fmt ;
        switch (format.fontSize){
        case TextFormat::FontSize::Title:
            fmt.setFontPointSize(Theme::POST_TITLE_FONT_SIZE);
            break ;
        case TextFormat::FontSize::Header:
            fmt.setFontPointSize(Theme::POST_HEADER_FONT_SIZE);
            break ;
        case TextFormat::FontSize::Body:
            fmt.setFontPointSize(Theme::POST_BODY_FONT_SIZE);
            break ;
        }
        QTextCursor cursor = textCursor();
        if ( !cursor.hasSelection() ){
            cursor.select(QTextCursor::BlockUnderCursor); 
        }
        cursor.mergeCharFormat(fmt) ;
        setTextCursor(cursor);
    }

    currentFormat_ = format ;
}

void PostNote::syncFormatFromCursor(){
    QTextCursor cursor = textCursor();
    QTextCharFormat charFmt = cursor.charFormat();

    TextFormat fmt ;
    fmt.bolded = charFmt.fontWeight() == QFont::Bold ;
    fmt.italicized = charFmt.fontItalic();
    fmt.underlined = charFmt.fontUnderline();
    fmt.font = charFmt.font();
    
    double fontSize = charFmt.fontPointSize();
    if ( fontSize == Theme::POST_TITLE_FONT_SIZE ){
        currentFormat_.fontSize = TextFormat::FontSize::Title ;
    } else if ( fontSize == Theme::POST_HEADER_FONT_SIZE ){
        currentFormat_.fontSize = TextFormat::FontSize::Header ;
    } else {
        currentFormat_.fontSize = TextFormat::FontSize::Body ;
    }

    if ( fmt != currentFormat_ ){
        currentFormat_ = fmt ;
        emit formatUpdated(currentFormat_);
    }
}