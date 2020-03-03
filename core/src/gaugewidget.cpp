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
#include <QQmlEngine>

GaugeWidget::GaugeWidget(QWidget *parent)
    : QQuickWidget(parent)
{
    setWindowTitle("Gauges");
    qmlRegisterType<GaugeItem>("Emstune.Gauges", 0, 1, "GaugeItem");
    qmlRegisterType<RoundGaugeItem>("Emstune.Gauges", 0, 1, "RoundGauge");
    qmlRegisterType<BarGaugeItem>("Emstune.Gauges", 0, 1, "BarGauge");
    rootContext()->setContextProperty("propertyMap", &propertyMap);

    auto engine = (QQmlEngine *)this->engine();
    engine->addImportPath(":/qml");

    const QStringList gauges({ "gauges.qml", "/etc/emstudio/gauges.qml", ":/qml/gauges.qml" });
    QString path;
    for (int i = 0; i < gauges.size(); i++) {
        if (QFile::exists(gauges.at(i))) {
            path = gauges.at(i);
            break;
        }
    }
    if (!path.isEmpty()) {
        setFile(path);
    } else {
        QLOG_WARN() << "GaugeWidget file not found";
        close();
    }
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
            QLOG_DEBUG() << "Plugin compatibility:" << plugincompat;
            return plugincompat;
        }
    }
    return QString();
}
