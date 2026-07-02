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

#include "api/ControlApiClient.hpp"
#include "api/DataApiClient.hpp"
#include "app/Theme.hpp"
#include "app/Synth.hpp"

#include <kddockwidgets/KDDockWidgets.h>
#include <kddockwidgets/core/ViewFactory.h>
#include <kddockwidgets/core/DockWidget.h>
#include <kddockwidgets/Config.h>
#include <QApplication>
#include <qlogging.h>
#include <qobject.h>
#include <spdlog/spdlog.h>

namespace KDDW   = KDDockWidgets ;

int main(int argc, char *argv[]){
#ifdef __linux__
    qputenv("QT_QPA_PLATFORM","xcb"); // remove this when wayland/gnome/docking support matures
#endif 

    auto logger = spdlog::default_logger();
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    logger->set_level(spdlog::level::debug);
    
    QApplication app(argc, argv);

    KDDW::initFrontend(KDDW::FrontendType::QtWidgets);
    KDDW::Core::ViewFactory::s_dropIndicatorType = KDDW::DropIndicatorType::Segmented ;
    KDDW::Config::self().setFlags(
       KDDW::Config::Flag_HideTitleBarWhenTabsVisible 
    );
    
    KDDW::Config::self().setDropIndicatorAllowedFunc(
    [](KDDW::DropLocation location,
       const KDDW::Core::DockWidget::List& source,
       const KDDW::Core::DockWidget::List& target,
       KDDW::Core::DropArea* ) -> bool
    {
        const bool isControlPanel = std::any_of(
            source.cbegin(), source.cend(),
            [](KDDW::Core::DockWidget *dw) {
                return dw->uniqueName() == "__modulationDock"
                    || dw->uniqueName() == "__parameterDock";
            });
        
        // editors cannot be placed on top/bottom docks.
        if (isControlPanel) {
            constexpr auto denied = 
                KDDW::DropLocation_OutterBottom |
                KDDW::DropLocation_OutterTop ;
            return !(location & denied);
        }

        return true ;
    });

    Theme::applyDarkTheme();
    qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss.zzz}] %{type}: %{message}");
    
    // initiate TCP clients
    ControlApiClient::instance() ; 
    DataApiClient::instance();

    Synth* synth = new Synth() ;

    ControlApiClient::instance()->connectToBackend();
    DataApiClient::instance()->connectToBackend();
    
    synth->showMaximized();

    return app.exec() ;
}
