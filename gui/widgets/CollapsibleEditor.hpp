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

#ifndef COLLAPSIBLE_EDITOR_HPP_
#define COLLAPSIBLE_EDITOR_HPP_

#include <QWidget>
#include <QVBoxLayout>
#include <QToolButton>
#include <QLabel>

class CollapsibleEditor : public QWidget {
    Q_OBJECT

private:
    QVBoxLayout* layout_ ;

    QWidget* header_ ;
    QToolButton* expandButton_ ;
    QLabel* titleLabel_ ;
    QWidget* content_ ;

    bool collapsed_ ;

public:
    explicit CollapsibleEditor(QString title, QWidget* content, QWidget* parent = nullptr);

    QWidget* getContent() const ;
    
    QString getTitle() const ;
    void setTitle(QString name);

    bool isCollapsed() const ;
    void setCollapsed(bool collapsed);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override ;

private:
    void setupLayout();
    void toggle();

};

#endif // COLLAPSIBLE_EDITOR_HPP_