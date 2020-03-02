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

#include "emsstatus.h"
#include <QVariant>
#include <QDebug>
#include <QTimer>
EmsStatus::EmsStatus(QWidget *parent)
    : QDockWidget(parent)
{
    ui.setupUi(this);
    connect(ui.hardResetPushButton, SIGNAL(clicked()), this, SIGNAL(hardResetRequest()));
    connect(ui.softResetPushButton, SIGNAL(clicked()), this, SIGNAL(softResetRequest()));
    setFlag(ui.minLockLineEdit, false);
    setFlag(ui.crankLockLineEdit, false);
    setFlag(ui.camLockLineEdit, false);
    setFlag(ui.previousPrimaryValidLineEdit, false);
    setFlag(ui.previousPeriodValidLineEdit, false);
    emsMemoryTimer = new QTimer(this);
    connect(emsMemoryTimer, SIGNAL(timeout()), this, SLOT(emsMemoryTimerTick()));
    ui.memoryCleanLineEdit->setText("NO BURN REQUIRED");
    QPalette pal = ui.memoryCleanLineEdit->palette();
    pal.setColor(QPalette::Base, QColor::fromRgb(100, 255, 100));
    ui.memoryCleanLineEdit->setPalette(pal);
    ui.memoryCleanLineEdit->setVisible(false);
}
void EmsStatus::closeEvent(QCloseEvent *event)
{
    event->ignore();
    this->hide();
    emit windowHiding((QMdiSubWindow *)this->parent());
}
EmsStatus::~EmsStatus()
{
}
void EmsStatus::setFlag(QLineEdit *edit, bool green)
{
    if (green) {
        if (edit->palette().color(QPalette::Base).red() == 255) {
            QPalette pal = edit->palette();
            pal.setColor(QPalette::Base, QColor::fromRgb(100, 255, 100));
            edit->setPalette(pal);
        }
    } else {
        if (edit->palette().color(QPalette::Base).green() == 255) {
            QPalette pal = edit->palette();
            pal.setColor(QPalette::Base, QColor::fromRgb(255, 50, 50));
            edit->setPalette(pal);
        }
    }
}

void EmsStatus::passData(QVariantMap data)
{

    unsigned char decoderFlags = data["Decoder Flags"].toInt();
    setFlag(ui.minLockLineEdit, decoderFlags & 0x01);
    setFlag(ui.crankLockLineEdit, decoderFlags & 0x02);
    setFlag(ui.camLockLineEdit, decoderFlags & 0x04);
    setFlag(ui.previousPrimaryValidLineEdit, decoderFlags & 0x08);
    setFlag(ui.previousPeriodValidLineEdit, decoderFlags & 0x010);
}
void EmsStatus::setEmsMemoryDirty()
{
    ui.memoryCleanLineEdit->setText("BURN REQUIRED");
    emsMemoryTimer->start(500);
}

void EmsStatus::setEmsMemoryClean()
{
    ui.memoryCleanLineEdit->setText("NO BURN REQUIRED");
    emsMemoryTimer->stop();
    QPalette pal = ui.memoryCleanLineEdit->palette();
    pal.setColor(QPalette::Base, QColor::fromRgb(100, 255, 100));
    ui.memoryCleanLineEdit->setPalette(pal);
}
void EmsStatus::emsMemoryTimerTick()
{
    if (ui.memoryCleanLineEdit->palette().color(QPalette::Base).green() == 170) {
        QPalette pal = ui.memoryCleanLineEdit->palette();
        pal.setColor(QPalette::Base, QColor::fromRgb(255, 50, 50));
        ui.memoryCleanLineEdit->setPalette(pal);
    } else {
        QPalette pal = ui.memoryCleanLineEdit->palette();
        pal.setColor(QPalette::Base, QColor::fromRgb(255, 170, 0));
        ui.memoryCleanLineEdit->setPalette(pal);
    }
}
