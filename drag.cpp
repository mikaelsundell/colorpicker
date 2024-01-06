// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "drag.h"
#include "mac.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QtGlobal>

class DragPrivate : public QObject
{
    Q_OBJECT
    public:
        DragPrivate();
        void init();
        void mapToGeometry();
        QSize mapToSize() const;
        bool eventFilter(QObject* object, QEvent* event);
        void activate();
        void deactivate();
    
    public:
        class State
        {
            public:
                QPoint position;
                QRect rect;
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
        QPointer<Drag> widget;
};

DragPrivate::DragPrivate()
: borderColor(Qt::black)
, offset(QPoint(0.0, 0.0))
, baseSize(256, 256)
, baseRect(0, 0, 256, 256)
, scale(1.0)
, factor(1.0)
{
}

void
DragPrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->resize(mapToSize());
    widget->setCursor(Qt::BlankCursor);
    widget->installEventFilter(this);
    mac::setupOverlay(widget->winId());
}

void
DragPrivate::mapToGeometry()
{
    QScreen* screen = QGuiApplication::screenAt(position);
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
        widget->setGeometry(rect);
        widget->setFixedSize(rect.size());
        paintGrid();
    } else {
        widget->setGeometry(x, y, width, height);
        widget->setFixedSize(width, height);
        widget->QWidget::update();
    }
}


QSize
DragPrivate::mapToSize() const
{
    return baseSize * factor;
}

void
DragPrivate::paintCross()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    qreal dpr = screen->devicePixelRatio();
    // size
    QSize size = mapToSize();
    // buffer
    buffer = QPixmap(size * dpr);
    //buffer.fill(Qt::transparent);
    buffer.fill(Qt::gray);
    
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

void
DragPrivate::paintDrag()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    qreal dpr = screen->devicePixelRatio();
    
    // size
    QSize size = widget->rect().size(); // size is now widget size, no offsets
    
    // buffer
    buffer = QPixmap(size * dpr);
    
    //buffer.fill(Qt::transparent);
    buffer.fill(Qt::darkBlue);
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
            QBrush brush = QBrush(borderColor);
            
            p.setBrush(QBrush(Qt::darkRed));
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
                qreal length = qMax(radius * 0.2, 0.0);
                qreal origin = length * 0.2;
                p.setPen(QPen(brush, 1));
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
                qreal length = qMax(radius * 0.2, 0.0);
                qreal origin = length * 0.2;
                p.setPen(QPen(brush, 1));
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
DragPrivate::paintGrid()
{
    if (state.dragging) {
        paintDrag();
    } else {
        paintCross();
    }
}

bool
DragPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Escape)
        {
            widget->hide();
            widget->closed();
            deactivate();
        
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Plus)
        {
            factor = qMin(factor + 0.1, 0.8);
            paintGrid();
            mapToGeometry(); // needed to update mask
        }
        else if (keyEvent->key() == Qt::Key_Minus)
        {
            factor = qMax(factor - 0.1, 0.4);
            paintGrid();
            mapToGeometry(); // needed to update mask
        }
    }
    
    if (event->type() == QEvent::QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = (QMouseEvent*)event;
        if (mouseEvent->button() == Qt::LeftButton) {
            
            qDebug() << "Start drag @ position: " << position;
            
            activate();
        }
        
        if (mouseEvent->button() == Qt::RightButton) {
            widget->hide();
            widget->closed();
            deactivate();
        }
    }
    
    if (event->type() == QEvent::QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouseEvent = (QMouseEvent*)event;
        if (mouseEvent->button() == Qt::LeftButton) {
            
            qDebug() << "Release drag @ position: " << position;
            
            deactivate();
            
        }
        
        if (mouseEvent->button() == Qt::RightButton) {
            widget->hide();
            widget->closed();
            deactivate();
        }
    }
    return false;
}

void
DragPrivate::activate()
{
    state = State {
        position,
        widget->geometry(),
        true
    };
    paintGrid();
    mapToGeometry();
}

void
DragPrivate::deactivate()
{
    state = State {
        QPoint(),
        QRect(),
        false
    };
    paintGrid();
    mapToGeometry();
}

#include "drag.moc"

Drag::Drag()
: QWidget(nullptr,
  Qt::Window |
  Qt::FramelessWindowHint)
, p(new DragPrivate())
{
    p->widget = this;
    p->init();
    p->paintGrid();
}

Drag::~Drag()
{
}

QColor
Drag::borderColor()
{
    return p->borderColor;
}

void
Drag::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::transparent);
    if (!p->state.dragging) {
        painter.drawPixmap(p->offset.x(), p->offset.y(), p->buffer);
    } else {
        painter.drawPixmap(0, 0, p->buffer);
    }
    painter.end();
}

void
Drag::setBorderColor(const QColor& color)
{
    if (p->borderColor != color) {
        //p->borderColor = color;
        //p->paintGrid();
    }
}

void
Drag::update(const QPoint& position)
{
    p->position = position;
    p->mapToGeometry();
}
