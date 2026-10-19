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


#include <math.h>

namespace decibel {
    // About -80 dB. Floor used wherever a linear gain undergoes division / log
    inline constexpr double silenceFloor = 1e-4 ;

    // dB -> linear amplitude gain: 0 dB == 1.0, -24 dB ~= 0.0631, +24 dB ~= 15.85
    inline double toLinear(double db){
        return std::pow(10.0, db / 20.0);
    }

    // Linear amplitude gain -> dB, with the floor applied to avoid log10(0).
    inline double fromLinear(double g) {
        return 20.0 * std::log10(std::max(g, silenceFloor));
    }
};