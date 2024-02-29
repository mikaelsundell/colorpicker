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
#include <QBackingStore>

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
        QPixmap paintSweep();
        QPixmap buffer;
        QColor borderColor;
        QPoint position;
        QSize baseSize;
        QRect baseRect;
        qreal scale;
        State state;
        QPointer<Dragger> widget;
    

};

DraggerPrivate::DraggerPrivate()
: borderColor(Qt::white)
, baseSize(256, 256)
, baseRect(0, 0, 256, 256)
, scale(1.0)
{
}

void
DraggerPrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->resize(mapToSize());
    widget->setCursor(Qt::CrossCursor);
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
        position.setX(qMax(clipGeometry.left(), qMin(position.x(), clipGeometry.right() - 1)));
        position.setY(qMax(clipGeometry.top(), qMin(position.y(), clipGeometry.bottom() - 1)));
    } else {
        screen = QGuiApplication::screenAt(position);
    }
    QSize size = mapToSize();
    int x = position.x() - size.width() / 2;
    int y = position.y() - size.height() / 2;
    int width;
    int height;
    
    QRect screenGeometry = screen->geometry();
    // left
    if (x < screenGeometry.left()) {
        width = (x + size.width()) - screenGeometry.left();
        x = screenGeometry.left() - 2;
    }
    // right
    else if (x + size.width() > screenGeometry.right()) {
        width = (size.width() - ((x + size.width()) - screenGeometry.right()));
        x = (screenGeometry.right() + 1) - width;
    }
    else {
        width = size.width();
    }
    // top
    if (y < screenGeometry.top()) {
        height = size.height() + y;
        y = screenGeometry.top();
    }
    // bottom
    else if (y + size.height() > screenGeometry.bottom()) {
        height = size.height() - ((y + size.height()) - screenGeometry.bottom());
        y = (screenGeometry.bottom() + 1) - height;
    }
    else {
        height = size.height() ;
    }

    baseRect = QRect(x, y, width, height);
    if (state.dragging) {
        QRect rect = baseRect.united(state.rect);
        state.unitedRect = rect;
        widget->setGeometry(rect);
        widget->setFixedSize(rect.size());
        widget->QWidget::update();
    } else {
        widget->setGeometry(x, y, width, height);
        widget->setFixedSize(width, height);
        widget->QWidget::update(); // no repaint needed, single cross
    }
}

QSize
DraggerPrivate::mapToSize() const
{
    return baseSize;
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

QPixmap
DraggerPrivate::paintSweep()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    qreal dpr = screen->devicePixelRatio();
    // size
    QSize size = state.unitedRect.size();
    // buffer
    QPixmap pixmap = QPixmap(size * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);
    // cross
    QPainter p(&pixmap);
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
    }
    p.end();
    return pixmap;
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
    mapToGeometry();
}

#include "dragger.moc"

Dragger::Dragger(QWidget* parent)
: QWidget(parent,
  Qt::Dialog |
  Qt::FramelessWindowHint |
  Qt::NoDropShadowWindowHint)
, p(new DraggerPrivate())
{
    p->widget = this;
    p->init();
}

Dragger::~Dragger()
{
}

void
Dragger::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 1)); // needed for mouse cursor update
    if (p->state.dragging) {
        painter.drawPixmap(0, 0, p->paintSweep());
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
