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
class GaugeItem : public QQuickView
{
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
    void setMinimum(double min) { m_minValue = min; }
    double getMinimum() { return m_minValue; }
    double getMaximum() { return m_maxValue; }
    double getStartAngle() { return m_scaleStartAngle; }
    double getEndAngle() { return m_scaleEndAngle; }
    double smoothing() { return m_smoothValue; }
    double getNumLabels() { return m_numLabels; }
    int getStyle() { return m_style; }
    void setStyle(int style)
    {
        m_style = style;
        m_redrawBackground = true;
    }
    int fade() { return m_fadeAmount; }
    void setFadingOn(bool fade);
    void setMaximum(double max) { m_maxValue = max; }
    void setStartAngle(double ang)
    {
        m_scaleStartAngle = ang;
        m_redrawBackground = true;
    }
    void setEndAngle(double ang)
    {
        m_scaleEndAngle = ang;
        m_redrawBackground = true;
    }
    void setSmoothing(double smooth) { m_smoothValue = smooth; }
    void setNumLabels(int num)
    {
        m_numLabels = num;
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
    void geometryChanged(const QRectF &newgeometry, const QRectF &oldgeometry);
    void drawBackground(QPainter *tmpPainter);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *style, QWidget *w);
    //void paintEvent(QPaintEvent *evt);
    //void resizeEvent (QResizeEvent *evt);
    QString m_text;
    bool m_redrawBackground;
    double m_value;
    int m_style;
    bool m_fake;
    bool m_fadeEnabled;
    int m_demoMax;
    int m_fadeAmount;
    bool m_resizeDraw;
    QImage *m_bgImage;
    QImage *m_fadedBgImage;
    QTimer *m_demoTimer;
    QTimer *m_gaugeTimer;
    QTimer *m_fadeTimer;
    double m_scaleEndAngle;
    double m_scaleStartAngle;
    double m_maxValue;
    double m_minValue;
    double _value;
    double m_targetValue;
    double m_danger;
    double m_warning;
    double m_smoothValue;
    bool m_reverseOrder;
    int m_numLabels;
    QPen m_outlinePen;
    QPen m_needleCenterPen;
    QPen m_needleCenterOutlinePen;
    QPen m_needlePen;
    QPen m_needleOutlinePen;
    QPen m_normalTickPen;
    QPen m_normalBigTickPen;
    QPen m_normalFontPen;
    QPen m_dangerFontPen;
    QPen m_warningFontPen;
    QPen m_dangerTickPen;
    QPen m_dangerBigTickPen;
    QPen m_warningTickPen;
    QPen m_warningBigTickPen;
    QFont m_labelFont;
    QFont m_labelSmallFont;
    int m_fadeCount;

private slots:
    void timerTick();
    void gaugeTimerTick();
    void fadeTimerTick();
};
#endif //GAUGEITEM_H
