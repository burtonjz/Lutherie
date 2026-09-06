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

#include "widgets/TextToolbar.hpp"
#include "widgets/HyperlinkDialog.hpp"
#include "app/Theme.hpp"

#include <QTextCursor>
#include <QGraphicsObject>
#include <spdlog/spdlog.h>

TextToolbar::TextToolbar(QGraphicsView* view):
    QWidget(view->viewport()),
    view_(view),
    boldBtn_(new QToolButton(this)),
    italicBtn_(new QToolButton(this)),
    underlineBtn_(new QToolButton(this)),
    linkBtn_(new QToolButton(this)),
    styleCombo_(new QComboBox(this)),
    fontCombo_(new QFontComboBox(this)),
    layout_(new QGridLayout(this)),
    format_(TextFormat())
{
    setAttribute(Qt::WA_TranslucentBackground);

    build();
}

void TextToolbar::build(){
    boldBtn_->setCheckable(true);
    boldBtn_->setProperty("checkableBtn", true);
    boldBtn_->setText(Theme::POST_NOTE_BOLD_BTN_TEXT);
    boldBtn_->setFont(QFont(boldBtn_->font().family(), -1, QFont::Bold));
    connect(
        boldBtn_, &QToolButton::clicked, this, [this]{
            format_.bolded = !format_.bolded ;
            if ( anchor_ ) anchor_->applyTextFormat(format_);
        }
    );

    italicBtn_->setCheckable(true);
    italicBtn_->setProperty("checkableBtn", true);
    italicBtn_->setText(Theme::POST_NOTE_ITALIC_BTN_TEXT);
    QFont italic = italicBtn_->font();
    italic.setItalic(true);
    italicBtn_->setFont(italic);
    connect(
        italicBtn_, &QToolButton::clicked, this, [this]{
            format_.italicized = !format_.italicized ;
            if ( anchor_ ) anchor_->applyTextFormat(format_);
        }
    );
    
    underlineBtn_->setCheckable(true);
    underlineBtn_->setProperty("checkableBtn", true);
    underlineBtn_->setText(Theme::POST_NOTE_UNDERLINE_BTN_TEXT);
    connect(
        underlineBtn_, &QToolButton::clicked, this, [this]{
            format_.underlined = !format_.underlined ;
            if ( anchor_ ) anchor_->applyTextFormat(format_);
        }
    );

    linkBtn_->setText("🔗");
    linkBtn_->setToolTip("Insert Hyperlink");
    connect(
        linkBtn_, &QToolButton::clicked, this, [this]{
            QString selectedText ;
            if ( anchor_ ){
                selectedText = anchor_->selectedText();
            }

            HyperlinkDialog dialog(selectedText, this);
            if ( dialog.exec() == QDialog::Accepted && !dialog.url().isEmpty() ){
                anchor_->createHyperlink(dialog.url(), dialog.display());
            }

            view_->setFocus();
        }
    );

    styleCombo_->addItem("Title");
    styleCombo_->addItem("Header");
    styleCombo_->addItem("Body");
    connect(
        styleCombo_, &QComboBox::currentIndexChanged, 
        this, [this](int index){
            if ( anchor_ ){
                format_.fontSize = static_cast<TextFormat::FontSize>(index);
                anchor_->applyTextFormat(format_);
                view_->setFocus();
            } 
        }
    );

    connect(
        fontCombo_, &QFontComboBox::currentFontChanged, 
        this, [this](const QFont &font){
            if ( anchor_ ){
                format_.font = font ;
                anchor_->applyTextFormat(format_);
                view_->setFocus();
            }
        }
    );
    
    layout_->setContentsMargins(4,2,4,2);
    layout_->setSpacing(2);
    layout_->addWidget(boldBtn_, 0, 0);
    layout_->addWidget(italicBtn_, 0, 1);
    layout_->addWidget(underlineBtn_, 0, 2);
    layout_->addWidget(linkBtn_, 0, 3);
    layout_->addWidget(styleCombo_, 0, 4);
    layout_->addWidget(fontCombo_, 1, 0, 1, 5);

    setLayout(layout_);
    adjustSize();
    hide();
}

void TextToolbar::syncControls(){
    {
        QSignalBlocker b(boldBtn_);
        boldBtn_->setChecked(format_.bolded);
    }
    {
        QSignalBlocker b(italicBtn_);
        italicBtn_->setChecked(format_.italicized);
    }
    {
        QSignalBlocker b(underlineBtn_);
        underlineBtn_->setChecked(format_.underlined);
    }
    {
        QSignalBlocker b(fontCombo_);
        fontCombo_->setCurrentFont(format_.font);
    }
    {
        QSignalBlocker b(styleCombo_);
        styleCombo_->setCurrentIndex(static_cast<int>(format_.fontSize));
    }
}

bool TextToolbar::bold() const {
    return boldBtn_->isChecked();
}

bool TextToolbar::italic() const {
    return italicBtn_->isChecked();
}

bool TextToolbar::underline() const {
    return underlineBtn_->isChecked();
}

void TextToolbar::reposition(){
    if ( !anchor_ ) return ;

    QGraphicsObject* obj = dynamic_cast<QGraphicsObject*>(anchor_);
    if ( !obj ) return ;

    QGraphicsScene* scene = obj->scene();
    if ( !scene ) return ;

    QRectF sceneRect = obj->sceneBoundingRect();
    QPointF topLeft = view_->mapFromScene(sceneRect.topLeft());

    adjustSize();
    move(topLeft.x(), topLeft.y() - height() - 4);
}

void TextToolbar::onEditingStarted(TextFormat format){
    anchor_ = dynamic_cast<IToolbarTarget*>(sender());
    if ( !anchor_ ) return ;
    
    format_ = format ;
    syncControls();
    reposition();
    show();
    raise();
}

void TextToolbar::onEditingFinished(){
    anchor_ = nullptr ;
    hide();
}

void TextToolbar::onFormatUpdated(TextFormat format){
    format_ = format ;
    syncControls();
}
