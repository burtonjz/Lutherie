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

#ifndef UI_HPP
#define UI_HPP

#include "app/ModuleContext.hpp"

#include <QWidget>
#include <QComboBox>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

QT_BEGIN_NAMESPACE
namespace Ui {class AudioMidiSetupWidget;}
QT_END_NAMESPACE

class Setup : public QWidget {
    Q_OBJECT

public:
    Setup(ModuleContext ctx, QWidget *parent = nullptr);
    ~Setup();

    void populateSetupComboBox(QComboBox* box, const json& data);

private:
    Ui::AudioMidiSetupWidget* ui_ ;
    ModuleContext ctx_ ;

private slots:
    void onApiDataReceived(const json& json);
    void onSetupSubmit();
    void onSetupCompleted();

signals:
    void setupCompleted();

};

#endif // UI_HPP
