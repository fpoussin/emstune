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

#ifndef GAUGEITEM_H
#define GAUGEITEM_H
#include <QPainter>
#include <QPen>
#include <QQuickView>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
class GaugeItem : public QQuickView {
    Q_OBJECT
public:
    Q_PROPERTY(double m_value READ getValue WRITE setRaw)
    Q_PROPERTY(double startAngle READ getStartAngle WRITE setStartAngle)
    Q_PROPERTY(double endAngle READ getEndAngle WRITE setEndAngle)
    Q_PROPERTY(double minimum READ getMinimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ getMaximum WRITE setMaximum)
    Q_PROPERTY(double numLabels READ getNumLabels WRITE setNumLabels)
    Q_PROPERTY(int m_style READ getStyle WRITE setStyle)
    Q_PROPERTY(QString text READ getText WRITE setText)

    GaugeItem();
    double getValue() { return m_value; }
    void setValue(double value);
    void setRaw(double value);
    void passEvent(QStringList evt);
    void setMinimum(double min) { _minValue = min; }
    double getMinimum() { return _minValue; }
    double getMaximum() { return _maxValue; }
    double getStartAngle() { return _scaleStartAngle; }
    double getEndAngle() { return _scaleEndAngle; }
    double smoothing() { return _smoothValue; }
    double getNumLabels() { return _numLabels; }
    int getStyle() { return m_style; }
    void setStyle(int style)
    {
        m_style = style;
        m_redrawBackground = true;
    }
    int fade() { return fadeAmount; }
    void setFadingOn(bool fade);
    void setMaximum(double max) { _maxValue = max; }
    void setStartAngle(double ang)
    {
        _scaleStartAngle = ang;
        m_redrawBackground = true;
    }
    void setEndAngle(double ang)
    {
        _scaleEndAngle = ang;
        m_redrawBackground = true;
    }
    void setSmoothing(double smooth) { _smoothValue = smooth; }
    void setNumLabels(int num)
    {
        _numLabels = num;
        m_redrawBackground = true;
    }
    void setColor(QColor outlinecolor, QColor centercolor, QColor topreflectioncolor, QColor bottomreflectioncolor);
    void setColor1(QColor needlecolor, QColor needleoutlinecolor, QColor normaltickcolor, QColor bigtickcolor, QColor warningtickcolor, QColor dangertickcolor);
    void setColor2(QColor fontcolor, QColor warningfontcolor, QColor dangerfontcolor, QFont font);
    void startDemo(int max);
    void setFade(int fade);
    void show();
    //void setFake(bool fake) { m_fake = fake;repaint(); }
    void setGaugeStyle(int style);
    QString getText() { return m_text; }
    void setText(QString text)
    {
        m_text = text;
        update();
    }

private:
    QString m_text;
    bool m_redrawBackground;
    double m_value;
    void geometryChanged(const QRectF& newgeometry, const QRectF& oldgeometry);
    int m_style;
    bool m_fake;
    bool m_fadeEnabled;
    int demoMax;
    int fadeAmount;
    //void paintEvent(QPaintEvent *evt);
    //void resizeEvent (QResizeEvent *evt);
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* style, QWidget* w);
    bool resizeDraw;
    void drawBackground(QPainter* tmpPainter);
    QImage* _bgImage;
    QImage* _fadedBgImage;
    QTimer* demoTimer;
    QTimer* gaugeTimer;
    QTimer* fadeTimer;
    double _scaleEndAngle;
    double _scaleStartAngle;
    double _maxValue;
    double _minValue;
    double _value;
    double _targetValue;
    double _danger;
    double _warning;
    double _smoothValue;
    bool _reverseOrder;
    int _numLabels;
    QPen outlinePen;
    QPen needleCenterPen;
    QPen needleCenterOutlinePen;
    QPen needlePen;
    QPen needleOutlinePen;
    QPen normalTickPen;
    QPen normalBigTickPen;
    QPen normalFontPen;
    QPen dangerFontPen;
    QPen warningFontPen;
    QPen dangerTickPen;
    QPen dangerBigTickPen;
    QPen warningTickPen;
    QPen warningBigTickPen;
    QFont labelFont;
    QFont labelSmallFont;
    int fadeCount;
private slots:
    void timerTick();
    void gaugeTimerTick();
    void fadeTimerTick();
};
#endif //GAUGEITEM_H
