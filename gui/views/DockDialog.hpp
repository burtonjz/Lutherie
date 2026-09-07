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

#ifndef DOCK_DIALOG_HPP_
#define DOCK_DIALOG_HPP_

#include <QObject>
#include <kddockwidgets/DockWidget.h>

namespace KDDW = KDDockWidgets ;
namespace KDDWQt = KDDW::QtWidgets ;


class DockDialog : public QObject {
    Q_OBJECT

public:
    enum Result {
        Accepted,
        Rejected
    };

private:
    KDDWQt::DockWidget* dock_ = nullptr ;
    QEventLoop* loop_ = nullptr ;
    Result result_ = Rejected ;

public:
    explicit DockDialog(QString docName, QString docTitle, QWidget* content, QObject* parent = nullptr);
    Result exec();

public slots:
    void accept();
    void reject();
};

#endif // DOCK_DIALOG_HPP_
