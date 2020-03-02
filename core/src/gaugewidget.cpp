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

#include "gaugewidget.h"
#include "QsLog.h"
#include "bargaugeitem.h"
#include "roundgaugeitem.h"
#include <QDir>
#include <QFile>
#include <QGraphicsItem>
#include <QMetaType>
#include <QQmlContext>
GaugeWidget::GaugeWidget(QWidget *parent)
    : QQuickView((QWindow *)parent->window())
{
    qmlRegisterType<GaugeItem>("GaugeImage", 0, 1, "GaugeImage");
    qmlRegisterType<RoundGaugeItem>("AviatorGauges", 0, 1, "RoundGauge");
    qmlRegisterType<BarGaugeItem>("AviatorGauges", 0, 1, "BarGauge");
    this->rootContext()->setContextProperty("propertyMap", &propertyMap);
    /*if (QFile::exists("gauges.qml"))
    {
        setSource(QUrl("gauges.qml"));
    }
    else if (QFile::exists("src/gauges.qml"))
    {
        setSource(QUrl("src/gauges.qml"));
    }
    else if (QFile::exists("/etc/emstudio/gauges.qml"))
    {
        setSource(QUrl("/etc/emstudio/gauges.qml"));
    }
    else
    {
    }*/
}

QString GaugeWidget::setFile(QString file)
{
    QLOG_DEBUG() << "GaugeWidget loading " << file;
    setSource(QUrl::fromLocalFile(file));
    if (rootObject()) {
        for (int i = 0; i < rootObject()->childItems().size(); i++) {
            QGraphicsObject *obj = qobject_cast<QGraphicsObject *>(rootObject()->childItems()[i]);
            if (obj) {
                propertylist.append(obj->property("propertyMapProperty").toString());
            }
        }
        if (rootObject()->property("plugincompat").isValid()) {
            QString plugincompat = rootObject()->property("plugincompat").toString();
            QLOG_DEBUG() << "Plugin compatability:" << plugincompat;
            return plugincompat;
        }
    }
    return "";
}
