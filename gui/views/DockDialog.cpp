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

#include "views/DockDialog.hpp"

#include <kddockwidgets/core/DockRegistry.h>
#include <kddockwidgets/core/FloatingWindow.h>
#include <kddockwidgets/core/TitleBar.h>
#include <spdlog/spdlog.h>

DockDialog::DockDialog(QString docName, QString docTitle, QWidget* content, QObject* parent): 
    QObject(parent)
{
    dock_ = new KDDWQt::DockWidget(
        docName, {
            KDDW::DockWidgetOption_NotDockable,
            KDDW::DockWidgetOption_NotClosable,
            KDDW::DockWidgetOption_DeleteOnClose
        }
    );
    dock_->setTitle(docTitle);
    dock_->setWidget(content);
    dock_->resize(content->sizeHint());
}

DockDialog::Result DockDialog::exec(){
    QEventLoop loop ;
    loop_ = &loop ;

    dock_->show();

    // have to manually hide the close button. KDDW is weird sometimes.
    auto* title = dock_->actualTitleBar();
    title->setHideDisabledButtons(KDDW::TitleBarButtonType::Close);

    loop.exec();

    dock_->setWidget(nullptr);
    loop_ = nullptr ;
    dock_->close();

    return result_ ;
}

void DockDialog::accept(){
    result_ = Accepted ;
    if ( loop_ ) loop_->quit();
}

void DockDialog::reject(){
    result_ = Rejected ;
    if ( loop_ ) loop_->quit();
}