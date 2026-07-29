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

#include <filesystem>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

namespace fs = std::filesystem ;

int main(int argc, char** argv){
    // get file path
    fs::path self = fs::canonical("/proc/self/exe");
    fs::path dir  = self.parent_path();

    fs::path backend = dir / "lutherie-backend" ;
    fs::path gui     = dir / "lutherie-gui" ;

    pid_t backend_pid = fork();

    if ( backend_pid == 0 ){
        execl(backend.c_str(), backend.c_str(), nullptr);
        std::perror("backend");
        std::_Exit(1);
    }

    // TODO: I should get a signal from backend indicating it is ready
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    pid_t gui_pid = fork();

    if (gui_pid == 0)
    {
        execl(gui.c_str(), gui.c_str(), nullptr);
        std::perror("gui");
        std::_Exit(1);
    }

    int status ;
    waitpid(gui_pid, &status, 0);
    kill(backend_pid, SIGTERM);
    waitpid(backend_pid, nullptr, 0);

    return 0 ;
}