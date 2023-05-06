// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "editor.h"
#include "mac.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>

// generated files
#include "ui_editor.h"

class EditorPrivate : public QObject
{
    Q_OBJECT
    public:
        EditorPrivate();
        void init();
        void update();
        bool eventFilter(QObject* object, QEvent* event);
    
    public:
        QPixmap buffer;
        QPointer<Editor> widget;
        QScopedPointer<Ui_Editor> ui;
};

EditorPrivate::EditorPrivate()
{
}

void
EditorPrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    // ui
    ui.reset(new Ui_Editor());
    ui->setupUi(widget);
    // widget
    widget->installEventFilter(this);
    // connect
    connect(ui->slider, SIGNAL(valueChanged(int)), widget, SIGNAL(valueChanged(int)));
    mac::setupOverlay(widget->winId());
}

void
EditorPrivate::update()
{
    qreal dpr = widget->devicePixelRatio();
    buffer = QPixmap(widget->size() * dpr);
    buffer.fill(Qt::transparent);
    buffer.setDevicePixelRatio(dpr);
    // painter
    QPainter p(&buffer);
    qreal scale = 0.95;
    qreal w = widget->width() * scale;
    qreal h = widget->height() * scale * 0.75;
    QPointF center(widget->width()/2.0, widget->height()/2.0);
    QRectF rect(center.x() - w/2, center.y() - h/2, w, h);
    p.fillRect(widget->rect(), Qt::transparent);
    QBrush brush = QBrush(widget->palette().base());
    // background
    {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(brush, 1.0));
        p.setBrush(brush);
        
        qreal radius = 4.0;
        QPainterPath background;
        {
            background.addRoundedRect(
                rect,
                radius,
                radius,
                Qt::AbsoluteSize
            );
            // arrow
            QPainterPath path;
            {
                qreal width = w * 0.1;
                qreal height = (widget->height() - h)/2.0;
                QRectF rect(center.x() - width/2.0, 0, width, height * 1.05);
                path.moveTo(rect.bottomLeft());
                path.lineTo(rect.center().x(), rect.top());
                path.lineTo(rect.bottomRight());
                path.moveTo(rect.bottomLeft());
            }
            p.drawPath(background.united(path));
        }
    }
    p.end();
}

bool
EditorPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate) {
        widget->hide();
    }
}

#include "editor.moc"

Editor::Editor(QWidget* parent)
: QDialog(parent,
  Qt::Window |
  Qt::FramelessWindowHint)
, p(new EditorPrivate())
{
    p->widget = this;
    p->init();
    p->update();
}

Editor::~Editor()
{
}

int
Editor::maximum() const
{
    return p->ui->slider->maximum();
}

int
Editor::minimum() const
{
    return p->ui->slider->minimum();
}

void
Editor::setMaximum(int maximum)
{
    p->ui->slider->setMaximum(maximum);
}

void
Editor::setMinimum(int minimum)
{
    p->ui->slider->setMaximum(minimum);
}

int
Editor::value() const
{
    return p->ui->slider->value();
}

void
Editor::setValue(int value)
{
    p->ui->slider->setValue(value);
}

void
Editor::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, p->buffer);
    painter.end();
    
    QWidget::paintEvent(event);
}
