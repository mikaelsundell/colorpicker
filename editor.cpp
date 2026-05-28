// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "editor.h"
#include "mac.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QStyle>
#include <QTimer>

// generated files
#include "ui_editor.h"

class EditorPrivate : public QObject {
    Q_OBJECT

public:
    EditorPrivate();

    void init();
    void update();
    void setTopLevelWhenVisible();

    bool eventFilter(QObject* object, QEvent* event) override;

public:
    QPixmap buffer;
    QPointer<Editor> widget;
    QScopedPointer<Ui_Editor> ui;
};

EditorPrivate::EditorPrivate() {}

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
    connect(ui->slider, &QSlider::valueChanged, widget, &Editor::valueChanged);
}

void
EditorPrivate::setTopLevelWhenVisible()
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
EditorPrivate::update()
{
    if (!widget || widget->size().isEmpty()) {
        return;
    }

    qreal dpr = widget->devicePixelRatio();
    buffer = QPixmap(widget->size() * dpr);
    buffer.fill(Qt::transparent);
    buffer.setDevicePixelRatio(dpr);

    // painter
    QPainter p(&buffer);
    qreal scale = 0.95;
    qreal w = widget->width() * scale;
    qreal h = widget->height() * scale * 0.75;

    QPointF center(widget->width() / 2.0, widget->height() / 2.0);
    QRectF rect(center.x() - w / 2, center.y() - h / 2, w, h);
    p.fillRect(widget->rect(), Qt::transparent);

    QBrush brush = QBrush(widget->palette().base());
    // background
    {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(brush, 1.0));
        p.setBrush(brush);

        qreal radius = 4.0;
        QPainterPath background;
        background.addRoundedRect(rect, radius, radius, Qt::AbsoluteSize);

        // arrow
        QPainterPath path;
        {
            qreal width = w * 0.1;
            qreal height = (widget->height() - h) / 2.0;
            QRectF rect(center.x() - width / 2.0, 0, width, height * 1.05);

            path.moveTo(rect.bottomLeft());
            path.lineTo(rect.center().x(), rect.top());
            path.lineTo(rect.bottomRight());
            path.moveTo(rect.bottomLeft());
        }
        p.drawPath(background.united(path));
    }
    p.end();
}

bool
EditorPrivate::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object);

    if (!widget) {
        return false;
    }
    if (event->type() == QEvent::Show) {
        setTopLevelWhenVisible();
        return false;
    }
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Escape) {
            widget->hide();
            return true;
        }
    }
    if (event->type() == QEvent::StyleChange) {
        widget->style()->polish(widget);  // update stylesheet
        update();
        widget->QDialog::update();
        return false;
    }
    if (event->type() == QEvent::WindowDeactivate) {
        if (widget->isVisible()) {
            widget->hide();
        }
        return false;
    }
    return false;
}

#include "editor.moc"

Editor::Editor(QWidget* parent)
    : QDialog(parent, Qt::Window | Qt::FramelessWindowHint)
    , p(new EditorPrivate())
{
    p->widget = this;
    p->init();
    p->update();
}

Editor::~Editor() {}

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
    p->ui->slider->setMinimum(minimum);
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
Editor::paintEvent(QPaintEvent* event)
{
    const qreal dpr = devicePixelRatio();
    const bool needsUpdate = p->buffer.isNull() || p->buffer.size() != size() * dpr
                             || !qFuzzyCompare(p->buffer.devicePixelRatio() + 1.0, dpr + 1.0);

    if (needsUpdate) {
        p->update();
    }
    QPainter painter(this);
    if (!p->buffer.isNull()) {
        painter.drawPixmap(0, 0, p->buffer);
    }
    painter.end();
    QDialog::paintEvent(event);
}
