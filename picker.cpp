// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "picker.h"
#include "mac.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QtGlobal>

class PickerPrivate : public QObject
{
    Q_OBJECT
    public:
        PickerPrivate();
        void init();
        void mapToGeometry();
        QSize mapToSize() const;
        bool eventFilter(QObject* object, QEvent* event);
    
    public:
        void paintPicker();
        QPixmap buffer;
        QColor color;
        QPoint offset;
        QPoint position;
        QSize baseSize;
        qreal factor;
        qreal scale;
        QPointer<Picker> widget;
};

PickerPrivate::PickerPrivate()
: color(Qt::white)
, offset(QPoint(0.0, 0.0))
, baseSize(256, 256)
, scale(0.4)
, factor(0.5)
{
}

void
PickerPrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->resize(mapToSize());
    widget->installEventFilter(this);
    mac::setTopLevel(widget->winId());
}

void
PickerPrivate::mapToGeometry()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    QSize size = mapToSize();
    
    int x = position.x() - size.width() / 2;
    int y = position.y() - size.height() / 2;
    int width;
    int height;
    QRect screenGeometry = screen->geometry();
    // left
    if (x < screenGeometry.left()) {
        width = (x + size.width()) - screenGeometry.left();
        offset.setX(width-size.width());
        x = screenGeometry.left();
    }
    // right
    else if (x + size.width() > screenGeometry.right()) {
        width = size.width() - ((x + size.width()) - screenGeometry.right());
        x = (screenGeometry.right() + 1) - width;
        offset.setX(0);
    }
    else {
        width = size.width();
        offset.setX(0);
    }
    // top
    if (y < screenGeometry.top()) {
        height = size.height() + y;
        offset.setY(y);
        y = screenGeometry.top();
    }
    // bottom
    else if (y + size.height() > screenGeometry.bottom()) {
        height = size.height() - ((y + size.height()) - screenGeometry.bottom());
        y = (screenGeometry.bottom() + 1) - height;
        offset.setY(0);
    }
    else {
        height = size.height() ;
        offset.setY(0);
    }
    widget->setGeometry(x, y, width, height);
    // needed to remove resize handlers
    widget->setFixedSize(width, height);
    widget->QWidget::update();
}

QSize
PickerPrivate::mapToSize() const
{
    return(baseSize * factor);
}

void
PickerPrivate::paintPicker()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    qreal dpr = screen->devicePixelRatio();
    // size
    QSize size = mapToSize();
    // buffer
    buffer = QPixmap(size * dpr);
    buffer.fill(Qt::transparent);
    buffer.setDevicePixelRatio(dpr);
    
    QPainter p(&buffer);
    qreal diameter = std::min(size.width(), size.height()) * scale;
    qreal radius = diameter / 2.0;
    QPointF center(size.width() / 2.0, size.height() / 2.0);
    QRectF rect(center.x() - radius, center.y() - radius, diameter, diameter);
    // shadow
    {
        QColor shadow = Qt::black;
        shadow.setAlpha(80);
        QPointF offset(2, 2);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF ellipse = rect.translated(offset);
        p.setBrush(shadow);
        p.setPen(Qt::NoPen);
        p.drawEllipse(ellipse);
    }
    // ellipse
    {
        QBrush brush(Qt::white);
        p.setPen(QPen(brush, 1.0));
        p.setBrush(QBrush(color));
        p.drawEllipse(rect);
    }
    // cross
    p.translate(center.x(), center.y());
    {
        qreal length = qMax(radius * 0.4, 0.0);
        qreal origin = length * 0.2;
        QBrush brush = QBrush(Qt::white);
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
    if (event->type() == QEvent::Show) {
        mac::hideCursor();
    }
    if (event->type() == QEvent::Hide) {
        mac::showCursor();
        widget->closed();
    }
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Escape) {
            widget->hide();
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Plus) {
            factor = qMin(factor + 0.2, 1.0);
            paintPicker();
            mapToGeometry(); // needed to update mask
        }
        else if (keyEvent->key() == Qt::Key_Minus) {
            factor = qMax(factor - 0.2, 0.2);
            paintPicker();
            mapToGeometry(); // needed to update mask
        }
    }
    if (event->type() == QEvent::QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = (QMouseEvent*)event;
        if (mouseEvent->button() == Qt::LeftButton) {
            widget->triggered();
        }
        
        if (mouseEvent->button() == Qt::RightButton) {
            widget->hide();
            
        }
        return true;
    }
    return false;
}

#include "picker.moc"

Picker::Picker(QWidget* parent)
: QWidget(parent,
  Qt::Dialog |
  Qt::FramelessWindowHint |
  Qt::NoDropShadowWindowHint)
, p(new PickerPrivate())
{
    p->widget = this;
    p->init();
    p->paintPicker();
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
    painter.fillRect(rect(), QColor(0, 0, 0, 1)); // needed for mouse cursor update
    painter.drawPixmap(p->offset.x(), p->offset.y(), p->buffer);
    painter.end();
}

void
Picker::setColor(const QColor& color)
{
    if (p->color != color) {
        p->color = color;
        p->paintPicker();
    }
}

void
Picker::update(const QPoint& position)
{
    p->position = position;
    p->mapToGeometry();
}
