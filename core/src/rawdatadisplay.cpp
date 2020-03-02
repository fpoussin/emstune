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

#include "rawdatadisplay.h"

RawDataDisplay::RawDataDisplay(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    ui.sendCommandTableWidget->setColumnCount(4);
    ui.sendCommandTableWidget->setColumnWidth(0, 50);
    ui.sendCommandTableWidget->setColumnWidth(1, 100);
    ui.sendCommandTableWidget->setColumnWidth(2, 100);
    ui.sendCommandTableWidget->setColumnWidth(3, 500);
}

RawDataDisplay::~RawDataDisplay()
{
}
