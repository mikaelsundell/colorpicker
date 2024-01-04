// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "grid.h"
#include "mac.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QtGlobal>

class GridPrivate : public QObject
{
    Q_OBJECT
    public:
        GridPrivate();
        void init();
        void mapToGeometry();
        int mapToScale(int value) const;
        QSize mapToSize() const;
        bool eventFilter(QObject* object, QEvent* event);
    
    public:
        void paintGrid();
        QPixmap buffer;
        QColor borderColor;
        QPoint offset;
        QPoint origin;
        QPoint position;
        QSize baseSize;
        qreal factor;
        qreal scale;
        bool drag;
        QPointer<Grid> widget;
};

GridPrivate::GridPrivate()
: borderColor(Qt::black)
, offset(QPoint(0.0, 0.0))
, origin(QPoint(0.0, 0.0))
, baseSize(256, 256)
, scale(0.4)
, factor(0.5)
, drag(false)
{
}

void
GridPrivate::init()
{
    //widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->resize(mapToSize());
    widget->setCursor(Qt::BlankCursor);
    widget->installEventFilter(this);
    mac::setupOverlay(widget->winId());
}

void
GridPrivate::mapToGeometry()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    QSize size = mapToSize();
    
    int x = position.x() - size.width() / 2;
    int y = position.y() - size.height() / 2;
    int width = widget->width();
    int height = widget->height();
    
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
    widget->setGeometry(x, y, width, height);
    // needed to remove resize handlers
    widget->setFixedSize(width, height);
    widget->QWidget::update();
}

int
GridPrivate::mapToScale(int value) const
{
    return static_cast<int>(mapToSize().height() * scale);
}

QSize
GridPrivate::mapToSize() const
{
    return baseSize * factor;
}

void
GridPrivate::paintGrid()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    qreal dpr = screen->devicePixelRatio();
    // size
    QSize size = mapToSize();
    // buffer
    buffer = QPixmap(size * dpr);
    // TODO: buffer.fill(Qt::transparent);
    
    if (drag)
        buffer.fill(Qt::green);
    else
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

bool
GridPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Escape)
        {
            widget->hide();
            widget->closed();
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
            
            qDebug() << "Starting to drag ...";
            qDebug() << " position: " << position;
            
            origin = position;
            drag = true;
            //widget->triggered();
        }
        
        if (mouseEvent->button() == Qt::RightButton) {
            widget->hide();
            widget->closed();
            drag = false;
        }
    }
    
    if (event->type() == QEvent::QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouseEvent = (QMouseEvent*)event;
        if (mouseEvent->button() == Qt::LeftButton) {
            
            qDebug() << "Drag finished ...";
            
            drag = false;
            
            //widget->triggered();
        }
        
        if (mouseEvent->button() == Qt::RightButton) {
            widget->hide();
            widget->closed();
            drag = false;
        }
    }
}

#include "grid.moc"

Grid::Grid()
: QWidget(nullptr,
  Qt::Window |
  Qt::FramelessWindowHint)
, p(new GridPrivate())
{
    p->widget = this;
    p->init();
    p->paintGrid();
}

Grid::~Grid()
{
}

QColor
Grid::borderColor()
{
    return p->borderColor;
}

void
Grid::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::transparent);
    painter.translate(p->offset.x(), p->offset.y());
    painter.drawPixmap(0, 0, p->buffer);
    painter.end();
}

void
Grid::setBorderColor(const QColor& color)
{
    p->borderColor = color;
    p->paintGrid();
}

void
Grid::update(const QPoint& position)
{
    p->position = position;
    p->mapToGeometry();
}
