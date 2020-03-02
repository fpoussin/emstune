/************************************************************************************
 * EMStudio - Open Source ECU tuning software                                       *
 * Copyright (C) 2013  Michael Carpenter (malcom2073@gmail.com)                     *
 *                                                                                  *
 * This file is a part of EMStudio                                                  *
 *                                                                                  *
 * EMStudio is free software; you can redistribute it and/or                        *
 * modify it under the terms of the GNU Lesser General Public                       *
 * License as published by the Free Software Foundation, version                    *
 * 2.1 of the License.                                                              *
 *                                                                                  *
 * EMStudio is distributed in the hope that it will be useful,                      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 * Lesser General Public License for more details.                                  *
 *                                                                                  *
 * You should have received a copy of the GNU Lesser General Public                 *
 * License along with this program; if not, write to the Free Software              *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA   *
 ************************************************************************************/

#ifndef READONLYRAMVIEW_H
#define READONLYRAMVIEW_H

#include <QWidget>
#include <QTimer>
#include "readonlyramdata.h"
#include "ui_readonlyramview.h"

class ReadOnlyRamView : public QWidget
{
    Q_OBJECT

public:
    explicit ReadOnlyRamView(QWidget *parent = 0);
    ~ReadOnlyRamView();
    void passData(unsigned short locationid, QByteArray data, QList<ReadOnlyRamData> metadata);
    QTimer *readRamTimer;
    double calcAxis(unsigned short val, QList<QPair<QString, double>> metadata);

private:
    unsigned short m_locationId;
    Ui::ReadOnlyRamView ui;
private slots:
    void readRamTimerTick();
signals:
    void readRamLocation(unsigned short locationid);
};

#endif // READONLYRAMVIEW_H
