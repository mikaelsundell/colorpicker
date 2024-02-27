// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "dragger.h"
#include "mac.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QtGlobal>

class DraggerPrivate : public QObject
{
    Q_OBJECT
    public:
        DraggerPrivate();
        void init();
        void mapToGeometry();
        QSize mapToSize() const;
        QRect dragRect() const;
        bool eventFilter(QObject* object, QEvent* event);
        void activate();
        void deactivate();
    
    public:
        class State
        {
            public:
            QPoint position;
            QRect rect;
            QRect unitedRect;
            bool dragging;
        };
        void paintCross();
        void paintDrag();
        void paintGrid();
        QPixmap buffer;
        QColor borderColor;
        QPoint offset;
        QPoint position;
        QSize baseSize;
        QRect baseRect;
        qreal factor;
        qreal scale;
        State state;
        QPointer<Dragger> widget;
};

DraggerPrivate::DraggerPrivate()
: borderColor(Qt::white)
, offset(QPoint(0.0, 0.0))
, baseSize(256, 256)
, baseRect(0, 0, 256, 256)
, scale(1.0)
, factor(1.0)
{
}

void
DraggerPrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->resize(mapToSize());
    widget->setCursor(Qt::BlankCursor);
    widget->setContextMenuPolicy(Qt::NoContextMenu);
    widget->installEventFilter(this);
    mac::setTopLevel(widget->winId());
    deactivate(); // need to be initialized
}

void
DraggerPrivate::mapToGeometry()
{
    QScreen* screen;
    if (state.dragging) {
        screen = QGuiApplication::screenAt(state.position);
        QRect clipGeometry = screen->geometry();
        position.setX(qMax(clipGeometry.left(), qMin(position.x(), clipGeometry.right())));
        position.setY(qMax(clipGeometry.top(), qMin(position.y(), clipGeometry.bottom())));
    } else {
        screen = QGuiApplication::screenAt(position);
    }
    QSize size = mapToSize();
    int x = position.x() - size.width() / 2;
    int y = position.y() - size.height() / 2;
    int width = baseRect.width();
    int height = baseRect.height();
    
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
        x = screenGeometry.right() - width;
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
        y = screenGeometry.bottom() - height;
        offset.setY(0);
    }
    else {
        height = size.height() ;
        offset.setY(0);
    }
    baseRect = QRect(x, y, width, height);
    if (state.dragging) {
        QRect rect = baseRect.united(state.rect);
        state.unitedRect = rect;
        widget->setGeometry(rect);
        widget->setFixedSize(rect.size());
        widget->QWidget::update();
        paintGrid(); // force repaint, rectangle changed
    } else {
        widget->setGeometry(x, y, width, height);
        widget->setFixedSize(width, height);
        widget->QWidget::update(); // no repaint needed, single cross
    }
}

QSize
DraggerPrivate::mapToSize() const
{
    return baseSize * factor;
}

QRect
DraggerPrivate::dragRect() const
{
    if (state.dragging) {
        return QRect(state.position, position).normalized();
    } else {
        return QRect();
    }
}

void
DraggerPrivate::paintCross()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    qreal dpr = screen->devicePixelRatio();
    // size
    QSize size = mapToSize();
    // buffer
    buffer = QPixmap(size * dpr);
    buffer.fill(Qt::transparent);
    buffer.setDevicePixelRatio(dpr);

    // painter
    QPainter p(&buffer);
    qreal diameter = std::min(size.width(), size.height()) * scale;
    qreal radius = diameter/2.0;
    QPointF center(size.width()/2.0, size.height()/2.0);
    QBrush brush = QBrush(borderColor);
    // cross
    p.translate(center.x(), center.y());
    {
        qreal length = qMax(radius * 0.1, 0.0);
        qreal origin = length * 0.4;
        p.setPen(QPen(brush, 2));
        p.drawLine(origin, 0, length, 0);
        p.drawLine(-length, 0, -origin, 0);
        p.drawLine(0, length, 0, origin);
        p.drawLine(0, -origin, 0, -length);
    }
    p.end();
}

void
DraggerPrivate::paintDrag()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    qreal dpr = screen->devicePixelRatio();
    // size
    QSize size = widget->rect().size(); // size is now widget size, no offsets
    // buffer
    buffer = QPixmap(size * dpr);
    buffer.fill(Qt::transparent);
    buffer.setDevicePixelRatio(dpr);
    // cross
    QPainter p(&buffer);
    {
        QPoint from = widget->mapFromGlobal(state.position);
        QPoint to = widget->mapFromGlobal(position);
        // fill
        {
            p.save();
            QRect rectangle(from, to);
            QColor fillColor = Qt::gray;
            fillColor.setAlphaF(0.2);
            QBrush brush(fillColor);
            p.setBrush(brush);
            QPen pen(Qt::white, 1);
            p.setPen(pen);
            p.drawRect(rectangle);
            p.restore();
        }
        // cross from
        size = mapToSize();
        {
            p.save();
            qreal diameter = std::min(size.width(), size.height()) * scale;
            qreal radius = diameter/2.0;
            QBrush brush = QBrush(borderColor);
            p.translate(from.x(), from.y());
            {
                qreal length = qMax(radius * 0.1, 0.0);
                qreal origin = length * 0.4;
                p.setPen(QPen(brush, 2));
                p.drawLine(origin, 0, length, 0);
                p.drawLine(-length, 0, -origin, 0);
                p.drawLine(0, length, 0, origin);
                p.drawLine(0, -origin, 0, -length);
            }
            p.restore();
        }
        // cross to
        {
            p.save();
            qreal diameter = std::min(size.width(), size.height()) * scale;
            qreal radius = diameter/2.0;
            QBrush brush = QBrush(borderColor);
            p.translate(to.x(), to.y());
            {
                qreal length = qMax(radius * 0.1, 0.0);
                qreal origin = length * 0.4;
                p.setPen(QPen(brush, 2));
                p.drawLine(origin, 0, length, 0);
                p.drawLine(-length, 0, -origin, 0);
                p.drawLine(0, length, 0, origin);
                p.drawLine(0, -origin, 0, -length);
            }
            p.restore();
        }
    }
    p.end();
}

void
DraggerPrivate::paintGrid()
{
    if (state.dragging) {
        paintDrag();
    } else {
        paintCross();
    }
}

bool
DraggerPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::Hide) {
        widget->closed();
    }
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Escape) {
            widget->releaseMouse();
            widget->hide();
            deactivate();
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Plus) {
            factor = qMin(factor + 0.1, 0.8);
            paintGrid();
            mapToGeometry(); // needed to update mask
        }
        else if (keyEvent->key() == Qt::Key_Minus) {
            factor = qMax(factor - 0.1, 0.4);
            paintGrid();
            mapToGeometry(); // needed to update mask
        }
    }
    
    if (event->type() == QEvent::QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = (QMouseEvent*)event;
        if (mouseEvent->button() == Qt::LeftButton) {
            widget->grabMouse(); // needed for mouse move events
            activate();
        }
        
        if (mouseEvent->button() == Qt::RightButton) {
            widget->releaseMouse();
            widget->hide();
            deactivate();
        }
        return true;
    }
    
    if (event->type() == QEvent::QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = (QMouseEvent*)event;
        if (mouseEvent->button() == Qt::LeftButton) {
            widget->releaseMouse();
            widget->triggered();
            deactivate();
        }
        return true;
    }
    return false;
}

void
DraggerPrivate::activate()
{
    state = State {
        position,
        widget->geometry(),
        QRect(),
        true
    };
    paintGrid();
    mapToGeometry();
}

void
DraggerPrivate::deactivate()
{
    state = State {
        QPoint(),
        QRect(),
        QRect(),
        false
    };
    paintGrid();
    mapToGeometry();
}

#include "dragger.moc"

Dragger::Dragger(QWidget* parent)
: QWidget(parent,
  Qt::Dialog |
  Qt::FramelessWindowHint)
, p(new DraggerPrivate())
{
    p->widget = this;
    p->init();
    p->paintGrid();
}

Dragger::~Dragger()
{
}

void
Dragger::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 1)); // needed for mouse cursor update
    if (!p->state.dragging) {
        painter.drawPixmap(p->offset.x(), p->offset.y(), p->buffer);
    } else {
        painter.drawPixmap(0, 0, p->buffer);
    }
    painter.end();
}

QRect
Dragger::dragRect() const
{
    return p->dragRect();
}

void
Dragger::update(const QPoint& position)
{
    p->position = position;
    p->mapToGeometry();
}
