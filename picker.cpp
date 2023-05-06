// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "picker.h"
#include "mac.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPointer>

class PickerPrivate : public QObject
{
    Q_OBJECT
    public:
        PickerPrivate();
        void init();
        void update();
        bool eventFilter(QObject* object, QEvent* event);
    
    public:
        QPixmap buffer;
        QColor color;
        QCursor cursor;
        QPointer<Picker> widget;
};

PickerPrivate::PickerPrivate()
{
}

void
PickerPrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->resize(64, 64);
    widget->setCursor(Qt::BlankCursor);
    widget->installEventFilter(this);
    mac::setupOverlay(widget->winId());
}

void
PickerPrivate::update()
{
    qreal dpr = widget->devicePixelRatio();
    buffer = QPixmap(widget->size() * dpr);
    buffer.fill(Qt::transparent);
    buffer.setDevicePixelRatio(dpr);
    // painter
    QPainter p(&buffer);
    qreal scale = 0.4;
    qreal diameter = std::min(widget->width(), widget->height()) * scale;
    qreal radius = diameter/2.0;
    QPointF center(widget->width()/2.0, widget->height()/2.0);
    QRectF rect(center.x() - radius, center.y() - radius, diameter, diameter);
    p.fillRect(widget->rect(), Qt::transparent);
    QBrush brush = QBrush(Qt::black);
    // ellipse
    {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(brush, 1.0));
        p.setBrush(QBrush(color));
        p.drawEllipse(rect);
    }
    // cross
    p.translate(center.x(), center.y());
    {
        qreal length = qMax(radius * 0.2, 0.0);
        qreal origin = length * 0.2;
        p.setPen(QPen(brush, 1));
        p.drawLine(origin, 0, length, 0);
        p.drawLine(-length, 0, -origin, 0);
        p.drawLine(0, length, 0, origin);
        p.drawLine(0, -origin, 0, -length);
    }
    p.end();
}

bool
PickerPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Escape)
        {
            widget->hide();
            widget->closed();
        }
    }
    
    if (event->type() == QEvent::QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = (QMouseEvent*)event;
        if (mouseEvent->button() == Qt::LeftButton) {
            widget->triggered();
        }
        
        if (mouseEvent->button() == Qt::RightButton) {
            widget->hide();
            widget->closed();
        }
    }
}

#include "picker.moc"

Picker::Picker()
: QWidget(nullptr,
  Qt::Window |
  Qt::FramelessWindowHint)
, p(new PickerPrivate())
{
    p->widget = this;
    p->init();
    p->update();
}

Picker::~Picker()
{
}

QColor
Picker::color()
{
    return p->color;
}

void
Picker::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, p->buffer);
    painter.end();
}

void
Picker::setColor(QColor color)
{
    p->color = color;
    p->update();
    update();
}

