// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "dragger.h"
#include "mac.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QTimer>

class DraggerPrivate : public QObject {
    Q_OBJECT

public:
    DraggerPrivate();
    void init();
    void mapToGeometry();
    QSize mapToSize() const;
    QRect dragRect() const;
    bool eventFilter(QObject* object, QEvent* event) override;
    void activate();
    void deactivate();
    void setTopLevel();

public:
    class State {
    public:
        QPoint position;
        QRect rect;
        QRect unitedRect;
        bool dragging;
    };
    QPixmap paintCross();
    QPixmap paintSweep();
    QColor color;
    QPoint position;
    QSize baseSize;
    QRect baseRect;
    qreal scale;
    State state;
    QPointer<Dragger> widget;
};

DraggerPrivate::DraggerPrivate()
    : color(Qt::black)
    , baseSize(256, 256)
    , baseRect(0, 0, 256, 256)
    , scale(0.8)
    , state { QPoint(), QRect(), QRect(), false }
{}

void
DraggerPrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->resize(mapToSize());
    widget->installEventFilter(this);
    deactivate();
}

void
DraggerPrivate::setTopLevel()
{
#ifdef Q_OS_MAC
    if (!widget || !widget->isVisible()) {
        return;
    }
    QTimer::singleShot(0, widget, [this]() {
        if (!widget || !widget->isVisible()) {
            return;
        }
        mac::setTopLevel(widget->winId());
    });
#endif
}

void
DraggerPrivate::mapToGeometry()
{
    QScreen* screen = nullptr;

    if (state.dragging) {
        screen = QGuiApplication::screenAt(state.position);
    }
    else {
        screen = QGuiApplication::screenAt(position);
    }

    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (!screen || !widget) {
        return;
    }

    if (state.dragging) {
        QRect clipGeometry = screen->geometry();

        position.setX(qMax(clipGeometry.left(), qMin(position.x(), clipGeometry.right() - 1)));
        position.setY(qMax(clipGeometry.top(), qMin(position.y(), clipGeometry.bottom() - 1)));
    }

    QSize size = mapToSize();

    int x = position.x() - size.width() / 2;
    int y = position.y() - size.height() / 2;
    int width = size.width();
    int height = size.height();

    QRect screenGeometry = screen->geometry();

    if (x < screenGeometry.left()) {
        width = (x + size.width()) - screenGeometry.left();
        x = screenGeometry.left() - 2;
    }
    else if (x + size.width() > screenGeometry.right()) {
        width = size.width() - ((x + size.width()) - screenGeometry.right());
        x = (screenGeometry.right() + 1) - width;
    }

    if (y < screenGeometry.top()) {
        height = size.height() + y;
        y = screenGeometry.top();
    }
    else if (y + size.height() > screenGeometry.bottom()) {
        height = size.height() - ((y + size.height()) - screenGeometry.bottom());
        y = (screenGeometry.bottom() + 1) - height;
    }

    width = qMax(1, width);
    height = qMax(1, height);

    baseRect = QRect(x, y, width, height);

    if (state.dragging) {
        QRect rect = baseRect.united(state.rect);

        state.unitedRect = rect;

        widget->setGeometry(rect);
        widget->setFixedSize(rect.size());
        widget->QWidget::update();
    }
    else {
        widget->setGeometry(x, y, width, height);
        widget->setFixedSize(width, height);
        widget->QWidget::update();
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
    }

    return QRect();
}

QPixmap
DraggerPrivate::paintCross()
{
    QScreen* screen = QGuiApplication::screenAt(position);

    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    qreal dpr = screen ? screen->devicePixelRatio() : widget->devicePixelRatio();

    QPoint cursor = widget->mapFromGlobal(position);
    QSize size = mapToSize();

    QPixmap pixmap = QPixmap(size * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);

    QPainter p(&pixmap);

    qreal diameter = std::min(size.width(), size.height()) * scale;
    qreal radius = diameter / 2.0;

    p.translate(cursor.x(), cursor.y());

    {
        qreal length = qMax(radius * 0.1, 0.0);
        qreal origin = length * 0.4;

        p.setPen(QPen(Qt::black, 2));
        p.translate(2, 2);
        p.drawLine(origin, 0, length, 0);
        p.drawLine(-length, 0, -origin, 0);
        p.drawLine(0, length, 0, origin);
        p.drawLine(0, -origin, 0, -length);
        p.translate(-1, -1);
    }
    {
        qreal length = qMax(radius * 0.1, 0.0);
        qreal origin = length * 0.4;

        p.setPen(QPen(Qt::white, 2));
        p.drawLine(origin, 0, length, 0);
        p.drawLine(-length, 0, -origin, 0);
        p.drawLine(0, length, 0, origin);
        p.drawLine(0, -origin, 0, -length);
    }
    p.end();
    return pixmap;
}

QPixmap
DraggerPrivate::paintSweep()
{
    QScreen* screen = QGuiApplication::screenAt(position);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    qreal dpr = screen ? screen->devicePixelRatio() : widget->devicePixelRatio();
    QSize size = state.unitedRect.size();

    QPixmap pixmap = QPixmap(size * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);

    QPainter p(&pixmap);
    QPoint from = widget->mapFromGlobal(state.position);
    QPoint to = widget->mapFromGlobal(position);
    {
        p.save();

        QRect rectangle = QRect(from, to).normalized();
        QColor shadow = Qt::black;
        shadow.setAlpha(20);

        p.fillRect(rectangle, shadow);
        p.restore();
    }
    {
        QSize crossSize = mapToSize();
        QPoint cursor = widget->mapFromGlobal(position);

        qreal diameter = std::min(crossSize.width(), crossSize.height()) * scale;
        qreal radius = diameter / 2.0;

        p.save();
        p.translate(cursor.x(), cursor.y());
        {
            qreal length = qMax(radius * 0.1, 0.0);
            qreal origin = length * 0.4;

            p.setPen(QPen(Qt::black, 2));
            p.translate(2, 2);
            p.drawLine(origin, 0, length, 0);
            p.drawLine(-length, 0, -origin, 0);
            p.drawLine(0, length, 0, origin);
            p.drawLine(0, -origin, 0, -length);
            p.translate(-1, -1);
        }
        {
            qreal length = qMax(radius * 0.1, 0.0);
            qreal origin = length * 0.4;

            p.setPen(QPen(Qt::white, 2));
            p.drawLine(origin, 0, length, 0);
            p.drawLine(-length, 0, -origin, 0);
            p.drawLine(0, length, 0, origin);
            p.drawLine(0, -origin, 0, -length);
        }
        p.restore();
    }
    p.end();

    return pixmap;
}

bool
DraggerPrivate::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object);
    if (!widget) {
        return false;
    }
    if (event->type() == QEvent::Show) {
        mac::hideCursor();
        setTopLevel();
        return false;
    }
    if (event->type() == QEvent::Hide) {
        mac::showCursor();
        widget->closed();
        return false;
    }
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Escape) {
            widget->releaseMouse();
            widget->hide();
            deactivate();
            return true;
        }
    }
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            widget->grabMouse();
            activate();
            return true;
        }
        if (mouseEvent->button() == Qt::RightButton) {
            widget->releaseMouse();
            widget->hide();
            deactivate();
            return true;
        }
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            widget->releaseMouse();
            widget->triggered();
            deactivate();
            return true;
        }
    }
    return false;
}

void
DraggerPrivate::activate()
{
    state = State { position, widget->geometry(), QRect(), true };
    mapToGeometry();
}

void
DraggerPrivate::deactivate()
{
    state = State { QPoint(), QRect(), QRect(), false };
    mapToGeometry();
}

#include "dragger.moc"

Dragger::Dragger(QWidget* parent)
    : QWidget(parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    , p(new DraggerPrivate())
{
    p->widget = this;
    p->init();
}

Dragger::~Dragger() {}

void
Dragger::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 1));
    if (p->state.dragging) {
        painter.drawPixmap(0, 0, p->paintSweep());
    }
    else {
        painter.drawPixmap(0, 0, p->paintCross());
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
