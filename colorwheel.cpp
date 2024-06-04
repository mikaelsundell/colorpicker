// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorwheel.h"
#include "icctransform.h"

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPointer>

class ColorwheelPrivate : public QObject
{
    Q_OBJECT
    public:
        ColorwheelPrivate();
        void init();
        void update();
        int mapToSelected(const QPoint& point) const;
        QColor mapToColor(const QPoint& point) const;
        QColor mapToColor(const QColor& color, const QPoint& point) const;
        qreal mapToDegrees(qreal radians) const;
    
    public Q_SLOTS:
        void outputProfileChanged(const QString& outputProfile);
    
    public:
        class State
        {
            public:
            int index;
            QColor color;
            QRectF rect;
        };
        QPixmap paintColorwheel(int w, int h, qreal dpr, bool segmented);
        QPixmap paintColorring(int w, int h, qreal dpr);
        QPixmap colorwheel;
        QPixmap segmentedwheel;
        QPixmap colorring;
        QPixmap buffer;
        int width;
        int height;
        qreal angle;
        qreal markerSize;
        qreal borderOpacity;
        qreal backgroundOpacity;
        qreal scale;
        qreal zoomFactor;
        qreal offsetFactor;
        bool iqlineVisible;
        bool saturationVisible;
        bool labelsVisible;
        bool segmented;
        qsizetype selected;
        QList<QPair<QColor, QPair<QString, QString>>> colors;
        QList<State> states;
        QPointer<Colorwheel> widget;
};

ColorwheelPrivate::ColorwheelPrivate()
: width(1024)
, height(1024)
, angle(0.0)
, markerSize(0.5)
, borderOpacity(1.0)
, backgroundOpacity(0.5)
, scale(0.80)
, zoomFactor(1.0)
, offsetFactor(-90)
, iqlineVisible(false)
, saturationVisible(false)
, segmented(false)
, labelsVisible(false)
, selected(-1)
{
}

void
ColorwheelPrivate::init()
{
    colorwheel = paintColorwheel(width, height, 2.0, false);
    segmentedwheel = paintColorwheel(width, height, 2.0, true);
    colorring = paintColorring(width, height, 2.0);
    // icc profile
    ICCTransform* transform = ICCTransform::instance();
    // connect
    connect(transform, &ICCTransform::outputProfileChanged, this, &ColorwheelPrivate::outputProfileChanged);
}

void
ColorwheelPrivate::update()
{
    qreal dpr = widget->devicePixelRatio();
    buffer = QPixmap(widget->size() * dpr);
    buffer.fill(Qt::transparent);
    buffer.setDevicePixelRatio(dpr);
    states.clear();
    // painter
    QPainter p(&buffer);
    qreal diameter = std::min(widget->width(), widget->height()) * scale;
    qreal radius = diameter/2.0;
    QPointF center(widget->width()/2.0, widget->height()/2.0);
    QRectF rect(center.x() - radius, center.y() - radius, diameter, diameter);
    p.fillRect(widget->rect(), Qt::transparent);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.translate(center.x(), center.y());
    p.rotate(angle * 360 + offsetFactor);
    // background
    {
        p.save();
        p.translate(-center.x(), -center.y());
        // colorwheel
        {
            QPixmap pixmap;
            if (segmented) {
                pixmap = segmentedwheel.scaled(
                    rect.width(), rect.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation
                );
            } else {
                pixmap = colorwheel.scaled(
                    rect.width(), rect.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation
                );
                
            }
            p.setOpacity(backgroundOpacity);
            p.drawPixmap(rect.toRect(), pixmap);
        }
        // colorring
        {
            QPixmap pixmap = colorring.scaled(
                rect.width(), rect.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation
            );
            p.setOpacity(1.0);
            p.drawPixmap(rect.toRect(), pixmap);
        }
        p.restore();
    }
    // colors
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(1.0);
    qreal stroke = 1.5;
    QBrush brush = QBrush(Qt::white);
    QString selectedlabel;
    // selection
    bool hasselection = selected > -1 ? true : false;
    if (hasselection)
        selectedlabel = colors[selected].second.first;
    // angles
    {
        p.save();
        p.setPen(QPen(brush, stroke/2));
        int step = 30;
        qreal length = radius * 1.05;
        int r=360, y=60, g=120, c=180, b=240, m=300;
        for(int span=0; span<360; span++) {
            if ((span % step) == 0) {
                p.drawLine(radius, 0, length, 0);
                QTransform transform = p.transform();
                p.save();
                {
                    p.resetTransform();
                    QFont font = p.font();
                    int angle = 360-span;
                    if (angle == r || angle == g || angle == b) // primary colors
                        font.setPointSizeF(font.pointSizeF() * 0.9);
                    else
                        font.setPointSizeF(font.pointSizeF() * 0.70);
                    
                    p.setFont(font);
                    QFontMetrics metrics(p.font());
                    QString label;
                    if (angle == r)
                        label = "R-";
                    if (angle == m)
                        label = "M-";
                    if (angle == b)
                        label = "B-";
                    if (angle == c)
                        label = "C-";
                    if (angle == g)
                        label = "G-";
                    if (angle == y)
                        label = "Y-";
                
                    QString text = QString("%1%2°").arg(label).arg(angle);
                    QPointF pos = transform.map(QPointF(length, 0));
                    
                    qreal pad = 5; // bit more space around text
                    QRect rect = metrics.boundingRect(text);
                    rect.adjust(-pad, -pad, pad, pad);
                    
                    if (qAbs(pos.x() - center.x()) < rect.width())
                        pos.setX(pos.x() - rect.width()/2);
                    else if (pos.x() < center.x())
                        pos.setX(pos.x() - rect.width());

                    if (qAbs(pos.y() - center.y()) < rect.height())
                        pos.setY(pos.y() - rect.height()/2);
                    else if (pos.y() < center.y())
                        pos.setY(pos.y() - rect.height());
                    
                    rect.moveTo(pos.toPoint());
                    p.drawText(rect, Qt::AlignCenter, text);
                }
                p.restore();
                p.rotate(step);
            }
        }
        p.restore();
    }
    // lines
    for (QPair<QColor,QPair<QString, QString>> pair : colors)
    {
        QColor color = pair.first;
        QString label = pair.second.first;
        QString filename = pair.second.second;
        // painter
        {
            p.save();
            p.setPen(QPen(brush, stroke/2));
            if (hasselection && label != selectedlabel)
                p.setOpacity(0.5);
            
            p.rotate((1-color.hueF())*360);
            {
                qreal ellipse = radius * markerSize * 0.2;
                qreal length = qMin(radius, qMax(0.0, radius * color.saturationF() * zoomFactor - ellipse/2));
                {
                    p.setPen(QPen(brush, 0.5));
                    p.drawLine(0, 0, 0 + length, 0);
                }
            }
            p.restore();
        }
    }
    // iq lines, roughly -33% from yuv vectorscope
    if (iqlineVisible)
    {
        p.save();
        p.setPen(QPen(brush, stroke/2));
        p.rotate(-27);
        p.drawLine(0, 0, radius, 0);
        p.restore();
    }
    // colors
    for (QPair<QColor,QPair<QString, QString>> pair : colors)
    {
        QColor color = pair.first;
        QString label = pair.second.first;
        QString filename = pair.second.second;
        // paint
        {
            p.save();
            p.setPen(QPen(brush, 2.0));
            p.rotate((1-color.hueF())*360);
            qreal length = qMin(radius, qMax(0.0, radius * color.saturationF() * zoomFactor));
            qreal ellipse = radius * markerSize * 0.2;
            
            // selected - find index from states length
            bool isselected = states.length() == selected ? true : false;
            if (isselected)
                ellipse = ellipse * 1.4;
            
            // ellipse
            {
                QColor mapped = color;
                // icc profile
                ICCTransform* transform = ICCTransform::instance();
                if (filename != transform->outputProfile()) {
                    mapped = transform->map(mapped.rgb(), filename, transform->outputProfile());
                }
                p.setBrush(QBrush(mapped)); // we draw the mapped color to output profile
                p.setPen(QPen(brush, stroke));
                QRectF rect(-ellipse/2 + length, -ellipse/2, ellipse, ellipse);
                p.drawEllipse(rect);
                // state
                {
                    QTransform world = p.worldTransform();
                    states.push_back(State {
                        (int)states.count(),
                        color,
                        world.mapRect(rect)
                    });
                }
            }
            // triangle
            if (isselected) {
                qreal arrow = ellipse * 0.15;
                qreal stretch = 1.8;
                QRectF rect = QRectF(length-ellipse/2+stroke/2, -arrow*stretch/2, arrow, arrow*stretch);
                QPainterPath path;
                path.moveTo(rect.topLeft());
                path.lineTo(rect.right(), rect.top() + rect.height()/2);
                path.lineTo(rect.bottomLeft());
                path.moveTo(rect.topLeft());
                p.fillPath(path, brush);
            }
            // labels
            if (labelsVisible) {
                p.save();
                p.setPen(QPen(brush, stroke/2));
                QTransform transform = p.transform();
                p.resetTransform();
                QFontMetrics metrics(p.font());
                QString text = QString("%1").arg(label);
                if (text.length() > 20) { // limit label lengths
                    text = text.left(20) + "...";
                }
                QRectF bounding = metrics.boundingRect(text);
                QPointF pos = transform.map(QPointF(length, 0));
                QRectF rect = QRectF(-ellipse/2+length, -ellipse/2, ellipse, ellipse);
                rect.moveCenter(pos);
                bounding.moveTo(QPointF(rect.right()+4, rect.center().y()-bounding.height()/2));
                QPainterPath path;
                path.addRoundedRect(bounding, 4, 4);
                p.save();
                p.setOpacity(0.5);
                p.fillPath(path, widget->palette().base());
                p.restore();
                QFont font = p.font();
                font.setPointSizeF(font.pointSizeF() * 0.75);
                p.setFont(font);
                p.drawText(bounding, Qt::AlignCenter, text);
                p.restore();
            }
            p.restore();
        }
    }
    // saturation
    if (saturationVisible)
    {
        p.save();
        p.setPen(QPen(brush, stroke/2));
        p.rotate(-33);
        QTransform transform = p.transform();
        p.resetTransform();
        QList<qreal> values
        {
            0.25,
            0.5,
            0.75
        };
        for (qreal value : values) {            
            // ellipse
            p.save();
            {
                qreal length = radius * value;
                QRectF rect(center.x() - length, center.y() - length, length * 2, length * 2);
                p.setOpacity(0.5);
                p.drawEllipse(rect);
            }
            p.restore();
            // labels
            p.save();
            {
                QFontMetrics metrics(p.font());
                QString text = QString("%1%").arg(value * 100 * zoomFactor);
                QRect rect = metrics.boundingRect(text);
                qreal length = radius * value;
                QPointF pos = transform.map(QPointF(length, 0));
                rect.moveTo(pos.toPoint());
                QPainterPath path;
                path.addRoundedRect(rect, 4, 4);
                p.save();
                p.setOpacity(0.5);
                p.fillPath(path, widget->palette().base());
                p.restore();
                QFont font = p.font();
                font.setPointSizeF(font.pointSizeF() * 0.75);
                p.setFont(font);
                p.drawText(rect, Qt::AlignCenter, text);
            }
            p.restore();
        }
        p.restore();
    }
    p.end();
}

QPixmap
ColorwheelPrivate::paintColorwheel(int w, int h, qreal dpr, bool segmented)
{
    QPointF center(w/2.0, h/2.0);
    int diameter = std::min(w, h);
    int radius = diameter/2.0;
    QPixmap colorwheel(w * dpr, h * dpr);
    colorwheel.fill(Qt::transparent);
    colorwheel.setDevicePixelRatio(dpr);
    {
        QPainter p(&colorwheel);
        if (segmented) {
            int count = 12;
            int span = 5760 / count; // 5760 spans represent 360 degrees in Qt's angle unit
            QRect rect = colorwheel.rect();
            for (int segment = 0; segment < count; ++segment) {
                // Calculate the color for this segment
                QColor color(QColor::fromHsvF((segment) / static_cast<qreal>(count), 1.0, 1.0));
                // Set up a radial gradient with the color
                QRadialGradient gradient(center, radius);
                gradient.setColorAt(0, Qt::white);
                gradient.setColorAt(1, color);
                // Set the brush to the gradient
                QBrush brush(gradient);
                QPen pen(brush, 1.0);
                p.setPen(Qt::NoPen);
                p.setBrush(brush);

                // Draw the pie slice
                p.drawPie(
                    QRect(rect.topLeft(), rect.size() / dpr),
                    segment * span - (span / 2), span
                );
            }
        } else {
            // we draw the full 5760 pie spans to get a smooth good
            // looking pie gradient without visible transitions
            QRect rect = colorwheel.rect();
            for (int span=0; span<5760; ++span) {
                QColor color(QColor::fromHsvF(span/5760.0, 1.0, 1.0));
                QRadialGradient gradient(center, radius);
                gradient.setColorAt(0, Qt::white);
                gradient.setColorAt(1, color);
                QBrush brush(gradient);
                QPen pen(brush, 1.0);
                p.setPen(Qt::NoPen);
                p.setBrush(brush);
                p.drawPie(
                    QRect(rect.topLeft(), rect.size() / dpr), span, 1
                );
            }
            p.end();
        }
    }
    return colorwheel;
}

QPixmap
ColorwheelPrivate::paintColorring(int w, int h, qreal dpr)
{
    QPointF center(w/2.0, h/2.0);
    int diameter = std::min(w, h);
    int radius = diameter/2.0;
    float inner = 0.95;
    
    QPixmap colorring(w * dpr, h * dpr);
    colorring.fill(Qt::transparent);
    colorring.setDevicePixelRatio(dpr);
    {
        QPainter p(&colorring);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        int stops = 12;
        qreal steps = 1.0/(stops-1.0);
        QConicalGradient gradient(0, 0, 0);
        if (gradient.stops().size() < stops) {
            for (qreal stop=0; stop<1.0; stop+=steps) {
                gradient.setColorAt(stop, QColor::fromHsvF(stop, 1.0, 1.0));
            }
            gradient.setColorAt(1, QColor::fromHsvF(0, 1.0, 1.0));
        }
        p.translate(center);
        p.setPen(Qt::NoPen);
        p.setBrush(QBrush(gradient));
        p.drawEllipse(QPointF(0,0), radius, radius);
        p.setBrush(Qt::transparent);
        p.drawEllipse(QPointF(0,0), radius*inner, radius*inner);
        p.end();
    }
    return colorring;
}

int
ColorwheelPrivate::mapToSelected(const QPoint& point) const
{
    // return state index from world space
    for(State state : states) {
        QRectF rect = state.rect;
        if (rect.contains(point))
            return state.index;
    }
    return -1;
}

QColor
ColorwheelPrivate::mapToColor(const QPoint& point) const
{
    qreal diameter = std::min(widget->width(), widget->height()) * scale;
    qreal radius = diameter/2.0;
    QPointF center = widget->rect().center();
    QTransform transform;
    transform.rotate(angle * 360 + offsetFactor);
    // pos - map without rotation applied
    QPointF pos = transform.inverted().map(point - center);
    qreal hue = mapToDegrees(atan2(-pos.y(), pos.x()));
    qreal distance = sqrt(
        pos.x() * pos.x() + pos.y() * pos.y()
    );
    qreal saturation = qBound(0.0, distance / radius, 1.0);
    return QColor::fromHsvF(hue / 360.0, saturation, 1.0);
}

QColor
ColorwheelPrivate::mapToColor(const QColor& color, const QPoint& point) const
{
    QColor mapcolor = mapToColor(point);
    // combine value of color with new map color as colorwheel is 2D
    return QColor::fromHsvF(mapcolor.hueF(), mapcolor.saturationF(), color.valueF());
}

qreal
ColorwheelPrivate::mapToDegrees(qreal radians) const
{
    qreal degree = (radians * (180 / M_PI)) ;
    if (degree < 0.0) {
        degree += 360.0;
    }
    return degree;
}

void
ColorwheelPrivate::outputProfileChanged(const QString& outputProfile)
{
    update();
    widget->update();
}

#include "colorwheel.moc"

Colorwheel::Colorwheel(QWidget* parent)
: QWidget(parent)
, p(new ColorwheelPrivate())
{
    p->widget = this;
    p->init();
}

Colorwheel::~Colorwheel()
{
}


qreal
Colorwheel::angle() const
{
    return p->angle;
}

void
Colorwheel::setAngle(qreal angle)
{
    p->angle = angle;
    p->update();
    update();
}

qreal
Colorwheel::borderOpacity() const
{
    return p->borderOpacity;
}

void
Colorwheel::setBorderOpacity(qreal opacity)
{
    p->borderOpacity = opacity;
    p->update();
    update();
}

qreal
Colorwheel::backgroundOpacity() const
{
    return p->backgroundOpacity;
}

QList<QPair<QColor,QPair<QString,QString>>>
Colorwheel::colors()
{
    return p->colors;
}

void
Colorwheel::setColors(const QList<QPair<QColor,QPair<QString, QString>>> colors, bool selected)
{
    p->colors = colors;
    if (selected)
        p->selected = static_cast<int>(colors.count()) - 1;
    p->update();
    update();
}

int
Colorwheel::mapToSelected(const QPoint& point) const
{
    return p->mapToSelected(point);
}

QColor
Colorwheel::mapToColor(const QPoint& point) const
{
    return p->mapToColor(point);
}

QColor
Colorwheel::mapToColor(const QColor& color, const QPoint& point) const
{
    return p->mapToColor(color, point);
}

void
Colorwheel::setBackgroundOpacity(qreal opacity)
{
    p->backgroundOpacity = opacity;
    p->update();
    update();
}

qreal
Colorwheel::markerSize() const
{
    return p->markerSize;
}

void
Colorwheel::setMarkerSize(qreal size)
{
    p->markerSize = size;
    p->update();
    update();
}

bool
Colorwheel::isIQLineVisible() const
{
    return p->iqlineVisible;
}

void
Colorwheel::setIQLineVisible(bool visible)
{
    p->iqlineVisible = visible;
    p->update();
    update();
}

bool
Colorwheel::isSaturationVisible() const
{
    return p->saturationVisible;
}

void
Colorwheel::setSaturationVisible(bool visible)
{
    p->saturationVisible = visible;
    p->update();
    update();
}

bool
Colorwheel::isSegmented() const
{
    return p->segmented;
}

void
Colorwheel::setSegmented(bool segmented)
{
    p->segmented = segmented;
    p->update();
    update();
}

bool
Colorwheel::isLabelsVisible() const
{
    return p->labelsVisible;
}

void
Colorwheel::setLabelsVisible(bool visible)
{
    p->labelsVisible = visible;
    p->update();
    update();
}

qsizetype
Colorwheel::selected() const
{
    return p->selected;
}

bool
Colorwheel::hasSelection() const
{
    return p->selected > -1;
}

void
Colorwheel::setSelected(qsizetype selected)
{
    p->selected = selected;
    p->update();
    update();
}

qreal
Colorwheel::zoomFactor() const
{
    return p->zoomFactor;
}

void
Colorwheel::setZoomFactor(qreal factor)
{
    p->zoomFactor = factor;
    p->update();
    update();
}

void
Colorwheel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, p->buffer);
    painter.end();
}
