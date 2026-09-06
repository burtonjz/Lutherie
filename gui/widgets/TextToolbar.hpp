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

#ifndef TEXT_TOOLBAR_HPP_
#define TEXT_TOOLBAR_HPP_


#include <QWidget>
#include <QGraphicsView>
#include <QToolButton>
#include <QComboBox>
#include <QFontComboBox>
#include <QGridLayout>
#include "util/TextFormat.hpp"
#include "interfaces/IToolbarTarget.hpp"

class TextToolbar : public QWidget {
    Q_OBJECT

private:
    QGraphicsView* view_ ;
    QToolButton* boldBtn_ ;
    QToolButton* italicBtn_ ;
    QToolButton* underlineBtn_ ;
    QToolButton* linkBtn_ ;
    QComboBox* styleCombo_ ;
    QFontComboBox* fontCombo_ ;
    QGridLayout* layout_ ;

    TextFormat format_ ;
    IToolbarTarget* anchor_ = nullptr ;

public:
    explicit TextToolbar(QGraphicsView* view);

    bool bold() const ;
    bool italic() const ;
    bool underline() const ;

    void reposition();

private:
    void build();
    void syncControls();

public slots:
    void onEditingStarted(TextFormat format);
    void onEditingFinished();
    void onFormatUpdated(TextFormat format);

};

#endif // TEXT_TOOLBAR_HPP_
