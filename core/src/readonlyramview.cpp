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

#include "readonlyramview.h"
#include "QsLog.h"

ReadOnlyRamView::ReadOnlyRamView(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    readRamTimer = 0;
    ui.tableWidget->setColumnCount(2);
    ui.tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("Title"));
    ui.tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("Value"));
}
void ReadOnlyRamView::passData(unsigned short locationid, QByteArray data, QList<ReadOnlyRamData> metadata)
{
    QLOG_DEBUG() << "ReadOnlyRamView. Meta data:" << metadata.size() << "Data:" << data.size();
    m_locationId = locationid;
    if (readRamTimer) {
        //We're currently already polling.
    } else {
        readRamTimer = new QTimer();
        connect(readRamTimer, SIGNAL(timeout()), this, SLOT(readRamTimerTick()));
        readRamTimer->start(250);
    }
    //ui.tableWidget->clearContents();
    if (ui.tableWidget->rowCount() != metadata.size() && ui.tableWidget->rowCount() != 0) {
        QLOG_DEBUG() << "TableWidget rows does NOT equal metadata.size(). This should not happen!!";
        ui.tableWidget->setRowCount(metadata.size());
        for (int i = 0; i < metadata.size(); i++) {
            ui.tableWidget->setItem(i, 0, new QTableWidgetItem(metadata[i].dataTitle));
            ui.tableWidget->setItem(i, 1, new QTableWidgetItem());
        }
    } else {
        ui.tableWidget->setRowCount(metadata.size());
        for (int i = 0; i < metadata.size(); i++) {
            ui.tableWidget->setItem(i, 0, new QTableWidgetItem(metadata[i].dataTitle));
            ui.tableWidget->setItem(i, 1, new QTableWidgetItem());
        }
    }

    for (int i = 0; i < metadata.size(); i++) {
        for (int j = 0; j < ui.tableWidget->rowCount(); j++) {
            if (ui.tableWidget->item(j, 0)->text() == metadata[i].dataTitle) {
                unsigned short ushortval;
                unsigned short ucharval;
                double dval = 0;
                if (metadata[i].size == 1) {
                    ucharval = (unsigned char)data[metadata[i].offset];
                    dval = calcAxis(ucharval, metadata[i].calc);
                } else if (metadata[i].size == 2) {
                    ushortval = (unsigned char)data[metadata[i].offset + 1] + (((unsigned char)data[metadata[i].offset]) << 8);
                    dval = calcAxis(ushortval, metadata[i].calc);
                }
                ui.tableWidget->item(j, 1)->setText(QString::number(dval, 'f', 2));
            }
        }
    }
}
double ReadOnlyRamView::calcAxis(unsigned short val, QList<QPair<QString, double>> metadata)
{
    double newval = val;
    for (int j = 0; j < metadata.size(); j++) {
        if (metadata[j].first == "add") {
            newval += metadata[j].second;
        } else if (metadata[j].first == "sub") {
            newval -= metadata[j].second;
        } else if (metadata[j].first == "mult") {
            newval *= metadata[j].second;
        } else if (metadata[j].first == "div") {
            newval /= metadata[j].second;
        }
    }
    return newval;
}
ReadOnlyRamView::~ReadOnlyRamView()
{
}
void ReadOnlyRamView::readRamTimerTick()
{
    emit readRamLocation(m_locationId);
}
