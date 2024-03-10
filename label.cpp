// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "label.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPointer>

class LabelPrivate : public QObject
{
    Q_OBJECT
    public:
        LabelPrivate();
        void init();
        void update();
        bool eventFilter(QObject* object, QEvent* event);
    
    public:
        bool readOnly;
        QPixmap buffer;
        QPointer<Label> widget;
};

LabelPrivate::LabelPrivate()
: readOnly(true)
{
}

void
LabelPrivate::init()
{
    widget->installEventFilter(this);
}

void
LabelPrivate::update()
{
    qreal dpr = widget->devicePixelRatio();
    buffer = QPixmap(widget->size() * dpr);
    buffer.fill(Qt::transparent);
    buffer.setDevicePixelRatio(dpr);
    // painter
    QPainter p(&buffer);
    QBrush brush = QBrush(Qt::white);
    // readonly
    if (!readOnly)
    {
        QRectF rect = widget->rect();
        p.setPen(QPen(brush, 2));
        p.drawLine(rect.bottomLeft(), rect.bottomRight());
    }
    p.end();
}


bool
LabelPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = (QMouseEvent*)event;
        if (mouseEvent->button() == Qt::LeftButton) {
            widget->triggered();
        }
    }
    return false;
}

#include "label.moc"

Label::Label(QWidget* parent)
: QLabel(parent)
, p(new LabelPrivate())
{
    p->widget = this;
    p->init();
}

Label::~Label()
{
}

void
Label::setReadOnly(bool readOnly)
{
    p->readOnly = readOnly;
    p->update();
    update();
}

void
Label::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, p->buffer);
    painter.end();
    QLabel::paintEvent(event);
}
