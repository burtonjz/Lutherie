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

#ifndef FILE_SELECTOR_WIDGET_HPP_
#define FILE_SELECTOR_WIDGET_HPP_

#include "widgets/PathLabel.hpp"
#include "app/Theme.hpp"

#include <QWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QHBoxLayout>

class FileSelectorWidget : public QWidget {
    Q_OBJECT

private:
    QPushButton* button_ ;
    PathLabel* label_ ;
    QHBoxLayout* layout_ ;
    
public:
    explicit FileSelectorWidget(QWidget* parent = nullptr):
        QWidget(parent),
        button_(new QPushButton()),
        label_(new PathLabel()),
        layout_(new QHBoxLayout(this))
    {
        connect(button_, &QPushButton::clicked, [this](){
            QString filePath = QFileDialog::getOpenFileName(
                this,
                tr("Select Component File"),
                QDir::homePath(),
                tr("Audio Files (*.wav, *.aiff, *.mp3);;All Files (*)")
            );
            if (filePath.isEmpty()) {
                return ; 
            }
            setPath(filePath);
            emit fileSelected(filePath.toStdString());
        });

        button_->setText("Select File...");
        label_->setFixedWidth(Theme::FILE_SELECTOR_LABEL_WIDTH);
        layout_->addWidget(button_);
        layout_->addWidget(label_);
        layout_->addStretch();
        layout_->setContentsMargins(0, 0, 0, 0);
    }

    QString getPath() const {
        return label_->text();
    }

    void setPath(QString path){
        label_->setPath(path);
    }

signals:
    void fileSelected(std::string path);

};

#endif // FILE_SELECTOR_WIDGET_HPP_
