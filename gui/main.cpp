/*
 * Copyright (C) 2025 Jared Burton
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

#include "api/ApiClient.hpp"
#include "app/Theme.hpp"
#include "app/Synth.hpp"

#include <QApplication>
#include <qlogging.h>
#include <qobject.h>

int main(int argc, char *argv[]){
    Theme::applyDarkTheme();
    qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss.zzz}] %{type}: %{message}");
    
    QApplication app(argc, argv);
    ApiClient::instance() ; // initialize ApiClient singleton

    Synth* synth = new Synth() ;

    ApiClient::instance()->connectToBackend();
    synth->show();

    return app.exec() ;
}
