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

#include "widgets/HyperlinkDialog.hpp"

#include <QFormLayout>
#include <QDialogButtonBox>

HyperlinkDialog::HyperlinkDialog(const QString& initialDisplayText, QWidget* parent):
    QDialog(parent),
    url_(new QLineEdit()),
    display_(new QLineEdit())
{
    setWindowTitle("Insert Hyperlink");

    url_->setPlaceholderText("https://...");
    
    display_->setText(initialDisplayText);
    display_->setPlaceholderText("link text");

    QFormLayout* form = new QFormLayout();
    form->addRow("URL:", url_);
    form->addRow("Display:", display_);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(
        buttons, &QDialogButtonBox::accepted, 
        this, &QDialog::accept
    );
    connect(
        buttons, &QDialogButtonBox::rejected, 
        this, &QDialog::reject
    );

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    url_->setFocus();
}

QString HyperlinkDialog::url() const {
    return url_->text().trimmed() ;
}

QString HyperlinkDialog::display() const {
    return display_->text();
}
