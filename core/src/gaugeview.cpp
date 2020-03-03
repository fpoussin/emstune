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

#include "gaugeview.h"
#include <QMdiSubWindow>

GaugeView::GaugeView(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    m_widget = new GaugeWidget(this);
    m_widget->setGeometry(0, 0, 1200, 700);
    m_widget->show();

    m_guiUpdateTimer = new QTimer(this);
    connect(m_guiUpdateTimer, SIGNAL(timeout()), this, SLOT(guiUpdateTimerTick()));
    m_guiUpdateTimer->start(250);
}

GaugeView::~GaugeView()
{
}

void GaugeView::closeEvent(QCloseEvent *event)
{
    event->ignore();
    ((QMdiSubWindow *)this->parent())->hide();
    emit windowHiding((QMdiSubWindow *)this->parent());
}

void GaugeView::passData(QVariantMap data)
{
    m_valueMap = data;
}

QString GaugeView::setFile(QString file)
{
    QString result = m_widget->setFile(file);
    m_propertiesInUse = m_widget->getPropertiesInUse();
    return result;
}

void GaugeView::passDecoder(DataPacketDecoder *decoder)
{
    Q_UNUSED(decoder)
}

void GaugeView::guiUpdateTimerTick()
{

    QVariantMap::const_iterator i = m_valueMap.constBegin();
    while (i != m_valueMap.constEnd()) {
        if (m_propertiesInUse.contains(i.key())) {
            m_widget->propertyMap.setProperty(i.key().toLatin1(), i.value());
        }
        //ui.tableWidget->item(m_nameToIndexMap[i.key()],1)->setText(QString::number(i.value()));
        i++;
    }
}
