// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorpicker.h"
#include "picker.h"
#include "editor.h"
#include "eventfilter.h"
#include "lcms2.h"
#include "mac.h"

#include <QAction>
#include <QActionGroup>
#include <QBuffer>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPrinter>
#include <QScreen>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTextTable>
#include <QUrl>
#include <QWindow>
#include <QDebug>

// generated files
#include "ui_about.h"
#include "ui_colorpicker.h"

class ColorpickerPrivate : public QObject
{
    Q_OBJECT
    public:
        enum RgbChannel
        {
            R, G, B
        };
        enum HsvChannel
        {
            H, S, V
        };
        enum Format
        {
            Int8bit,
            Int10bit,
            Float,
            Hex,
            Percentage
        };
    public:
        ColorpickerPrivate();
        void init();
        void stylesheet();
        void update();
        void view();
        void widget();
        void activate();
        void deactivate();
        bool eventFilter(QObject* object, QEvent* event);
        bool blocked();
    
    public Q_SLOTS:
        void toggleDisplay();
        void togglePin(bool checked);
        void toggleActive(bool checked);
        void pick();
        void closed();
        void togglePick();
        void copyRGB();
        void copyHSV();
        void copyIccProfile();
        void copyColor();
        void as8bit();
        void as10bit();
        void asFloat();
        void asHex();
        void asPercentage();
        void magnify1x();
        void magnify2x();
        void magnify3x();
        void magnify4x();
        void magnify5x();
        void toggleMouseLocation();
        void iccConvertProfileChanged(int index);
        void toggleColors();
        void toggleRGB();
        void toggleR();
        void toggleG();
        void toggleB();
        void toggleHSV();
        void toggleH();
        void toggleS();
        void toggleV();
        void apertureChanged(int value);
        void markerSizeChanged(int value);
        void backgroundOpacityChanged(int value);
        void angleChanged(int value);
        void iqlineChanged(int state);
        void zoomChanged(int state);
        void saturationChanged(int state);
        void labelsChanged(int state);
        void editorChanged(int value);
        void pdf();
        void clear();
        void about();
        void openGithubReadme();
        void openGithubIssues();
    
    Q_SIGNALS:
        void readOnly(bool readOnly);

    public:
        class About : public QDialog
        {
            public: About(QWidget *parent = nullptr)
            : QDialog(parent)
            {
                QScopedPointer<Ui_About> about;
                about.reset(new Ui_About());
                about->setupUi(this);
                about->copyright->setText(MACOSX_BUNDLE_COPYRIGHT);
                QString url = GITHUBURL;
                about->github->setText(QString("Github project: <a href='%1'>%1</a>").arg(url));
                about->github->setTextFormat(Qt::RichText);
                about->github->setTextInteractionFlags(Qt::TextBrowserInteraction);
                about->github->setOpenExternalLinks(true);
                QFile file(":/files/resources/Copyright.txt");
                file.open(QIODevice::ReadOnly | QIODevice::Text);
                QTextStream in(&file);
                QString text = in.readAll();
                file.close();
                about->licenses->setText(text);
            }
        };
        class State
        {
            public:
                QColor color;
                QRect rect;
                int magnify;
                QPixmap buffer;
                QPoint cursor;
                QPoint origin;
                int displayNumber;
                QString iccProfile;
                QString iccDisplayProfile;
                QPixmap iccPixmap;
        };
        class Edit
        {
            public:
                enum Type
                {
                    Rgb,
                    Hsv
                };
                ColorpickerPrivate::RgbChannel rgbChannel;
                ColorpickerPrivate::HsvChannel hsvChannel;
                Type type;
        };
        float channelRgb(QColor color, RgbChannel channel);
        float channelHsv(QColor color, HsvChannel channel);
        QString formatRgb(QColor color, RgbChannel channel);
        QString formatHsv(QColor color, HsvChannel channel);
        QString asFloat(float channel);
        QString asHex(int channel);
        QString asPercentage(float channel);
        QString asDegree(float channel);
        QByteArray asBase64(const QImage& image, QString format);
        QList<QPair<QColor,QString>> asColors();
        int width;
        int height;
        int aperture;
        int magnify;
        int displayNumber;
        QString iccProfile;
        QString iccConvertProfile;
        QString iccDisplayProfile;
        QPoint cursor;
        bool active;
        bool mouselocation;
        Format format;
        State state;
        Edit edit;
        int selected;
        QSize size;
        QList<State> states;
        QPointer<Colorpicker> window;
        QScopedPointer<Picker> picker;
        QScopedPointer<Editor> editor;
        QScopedPointer<Eventfilter> displayfilter;
        QScopedPointer<Eventfilter> colorsfilter;
        QScopedPointer<Ui_Colorpicker> ui;
};

ColorpickerPrivate::ColorpickerPrivate()
: width(128)
, height(128)
, aperture(50)
, magnify(1)
, active(true)
, mouselocation(true)
, format(Format::Int8bit)
, selected(-1)
{
}

void
ColorpickerPrivate::init()
{
    mac::setupMac();
    // ui
    ui.reset(new Ui_Colorpicker());
    ui->setupUi(window);
    // utils
    picker.reset(new Picker());
    editor.reset(new Editor());
    // resources
    QDir resources(QApplication::applicationDirPath() + "/../Resources");
    ui->iccConvertProfile->insertSeparator(ui->iccConvertProfile->count());
    for(QFileInfo resourceFile : resources.entryInfoList( QStringList( "*.icc" )))
    {
        ui->iccConvertProfile->addItem
            ("Convert to " + resourceFile.baseName(), QVariant::fromValue(resourceFile.filePath()));
    }
    // actions
    ui->toggleActive->setDefaultAction(ui->active);
    ui->togglePin->setDefaultAction(ui->pin);
    // stylesheet
    stylesheet();
    // event filter
    window->installEventFilter(this);
    // display filter
    displayfilter.reset(new Eventfilter);
    ui->displayBar->installEventFilter(displayfilter.data());
    // color filter
    colorsfilter.reset(new Eventfilter);
    ui->colorsBar->installEventFilter(colorsfilter.data());
    // connect
    connect(displayfilter.data(), SIGNAL(pressed()), ui->toggleDisplay, SLOT(click()));
    connect(colorsfilter.data(), SIGNAL(pressed()), ui->toggleColors, SLOT(click()));
    connect(ui->toggleDisplay, SIGNAL(pressed()), this, SLOT(toggleDisplay()));
    connect(ui->toggleColors, SIGNAL(pressed()), this, SLOT(toggleColors()));
    connect(ui->iccConvertProfile, SIGNAL(currentIndexChanged(int)), this, SLOT(iccConvertProfileChanged(int)));
    connect(ui->r, SIGNAL(triggered()), this, SLOT(toggleR()));
    connect(ui->g, SIGNAL(triggered()), this, SLOT(toggleG()));
    connect(ui->b, SIGNAL(triggered()), this, SLOT(toggleB()));
    connect(ui->h, SIGNAL(triggered()), this, SLOT(toggleH()));
    connect(ui->s, SIGNAL(triggered()), this, SLOT(toggleS()));
    connect(ui->v, SIGNAL(triggered()), this, SLOT(toggleV()));
    connect(ui->pick, SIGNAL(triggered()), this, SLOT(togglePick()));
    connect(ui->togglePick, SIGNAL(pressed()), this, SLOT(togglePick()));
    connect(ui->copyRGBAsText, SIGNAL(triggered()), this, SLOT(copyRGB()));
    connect(ui->copyHSVAsText, SIGNAL(triggered()), this, SLOT(copyHSV()));
    connect(ui->copyIccAsText, SIGNAL(triggered()), this, SLOT(copyIccProfile()));
    connect(ui->copyColorAsBitmap, SIGNAL(triggered()), this, SLOT(copyColor()));
    connect(ui->active, SIGNAL(toggled(bool)), this, SLOT(toggleActive(bool)));
    connect(ui->pin, SIGNAL(toggled(bool)), this, SLOT(togglePin(bool)));
    connect(ui->as8bitValues, SIGNAL(triggered()), this, SLOT(as8bit()));
    connect(ui->as10bitValues, SIGNAL(triggered()), this, SLOT(as10bit()));
    connect(ui->asFloatValues, SIGNAL(triggered()), this, SLOT(asFloat()));
    connect(ui->asHexadecimalValues, SIGNAL(triggered()), this, SLOT(asHex()));
    connect(ui->asPercentageValues, SIGNAL(triggered()), this, SLOT(asPercentage()));
    {
        QActionGroup* actions = new QActionGroup(this);
        actions->setExclusive(true);
        for(QAction* action : ui->displayValues->actions())
            actions->addAction(action);
    }
    connect(ui->magnify1x, SIGNAL(triggered()), this, SLOT(magnify1x()));
    connect(ui->magnify2x, SIGNAL(triggered()), this, SLOT(magnify2x()));
    connect(ui->magnify3x, SIGNAL(triggered()), this, SLOT(magnify3x()));
    connect(ui->magnify4x, SIGNAL(triggered()), this, SLOT(magnify4x()));
    connect(ui->magnify5x, SIGNAL(triggered()), this, SLOT(magnify5x()));
    {
        QActionGroup* actions = new QActionGroup(this);
        actions->setExclusive(true);
        for(QAction* action : ui->magnify->actions())
            actions->addAction(action);
    }
    connect(ui->toggleMouseLocation, SIGNAL(triggered()), this, SLOT(toggleMouseLocation()));
    connect(ui->aperture, SIGNAL(valueChanged(int)), this, SLOT(apertureChanged(int)));
    connect(ui->markerSize, SIGNAL(valueChanged(int)), this, SLOT(markerSizeChanged(int)));
    connect(ui->backgroundOpacity, SIGNAL(valueChanged(int)), this, SLOT(backgroundOpacityChanged(int)));
    connect(ui->angle, SIGNAL(valueChanged(int)), this, SLOT(angleChanged(int)));
    connect(ui->iqline, SIGNAL(stateChanged(int)), this, SLOT(iqlineChanged(int)));
    connect(ui->zoom, SIGNAL(stateChanged(int)), this, SLOT(zoomChanged(int)));
    connect(ui->saturation, SIGNAL(stateChanged(int)), this, SLOT(saturationChanged(int)));
    connect(ui->labels, SIGNAL(stateChanged(int)), this, SLOT(labelsChanged(int)));
    connect(ui->clear, SIGNAL(pressed()), this, SLOT(clear()));
    connect(ui->pdf, SIGNAL(pressed()), this, SLOT(pdf()));
    connect(ui->about, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->openGithubReadme, SIGNAL(triggered()), this, SLOT(openGithubReadme()));
    connect(ui->openGithubIssues, SIGNAL(triggered()), this, SLOT(openGithubIssues()));
    connect(picker.get(), SIGNAL(triggered()), this, SLOT(pick()));
    connect(picker.get(), SIGNAL(closed()), this, SLOT(closed()));
    connect(editor.get(), SIGNAL(valueChanged(int)), this, SLOT(editorChanged(int)));
    // signals
    connect(this, SIGNAL(readOnly(bool)), ui->r, SLOT(setReadOnly(bool)));
    connect(this, SIGNAL(readOnly(bool)), ui->g, SLOT(setReadOnly(bool)));
    connect(this, SIGNAL(readOnly(bool)), ui->b, SLOT(setReadOnly(bool)));
    connect(this, SIGNAL(readOnly(bool)), ui->h, SLOT(setReadOnly(bool)));
    connect(this, SIGNAL(readOnly(bool)), ui->s, SLOT(setReadOnly(bool)));
    connect(this, SIGNAL(readOnly(bool)), ui->v, SLOT(setReadOnly(bool)));
    size = window->size();
    // debug
    #ifdef QT_DEBUG
        QMenu* menu = ui->menubar->addMenu("Debug");
        {
            QAction* action = new QAction("Reload stylesheet...", this);
            action->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
            menu->addAction(action);
            connect(action, &QAction::triggered, [&]() {
                this->stylesheet();
            });
        }
    #endif
}

void
ColorpickerPrivate::stylesheet()
{
    QDir resources(QApplication::applicationDirPath());
    QFile stylesheet(resources.absolutePath() + "/../Resources/App.css");
    stylesheet.open(QFile::ReadOnly);
    qApp->setStyleSheet(stylesheet.readAll());
}

void
ColorpickerPrivate::update()
{
    if (!active)
        return;
    
    int w = int(width / float(magnify));
    int h = int(height / float(magnify));
    int x = cursor.x() - w / 2;
    int y = cursor.y() - h / 2;
    
    if (width % magnify > 0)
        ++w;
    
    if (height % magnify > 0)
        ++h;
    
    QPixmap buffer;
    qreal dpr = 1.0;
    const QBrush blackBrush = QBrush(Qt::black);
    if (QScreen *screen = window->screen()) {
        buffer = mac::grabDisplayPixmap(x, y, w, h, picker->winId());
        dpr = buffer.devicePixelRatio();
    } else {
        buffer = QPixmap(w, h);
        buffer.fill(blackBrush.color());
    }
    QRegion geom(x, y, w, h);
    QRect screenRect;
    
    QScreen* screen = QGuiApplication::screenAt(cursor);
    const auto screens = QGuiApplication::screens();
    for (auto screen : screens)
    {
        screenRect |= screen->geometry();
    }
    geom -= screenRect;
    
    const auto rectsInRegion = geom.rectCount();
    if (rectsInRegion > 0) {
        QPainter p(&buffer);
        p.translate(-x, -y);
        p.setPen(Qt::NoPen);
        p.setBrush(blackBrush);
        p.drawRects(geom.begin(), rectsInRegion);
        p.end();
    }

    // paint with device pixel ration and apply
    // transforms and fills in user space
    QColor color;
    QRect rect(
       (w - aperture) / 2,
       (h - aperture) / 2,
       aperture, aperture
    );
    int colorR=0, colorG=0, colorB=0;
    QImage image = buffer.toImage();
    for(int cx = rect.left(); cx <= rect.right(); cx++)
    {
        for(int cy = rect.top(); cy <= rect.bottom(); cy++)
        {
            QColor pixel = image.pixel(cx * dpr, cy * dpr);
            colorR += pixel.red();
            colorG += pixel.green();
            colorB += pixel.blue();
        }
    }
    int size = rect.width() * rect.height();
    color = QColor(colorR / size, colorG / size, colorB / size);
    // icc profile
    if (iccDisplayProfile.isEmpty())
    {
        iccDisplayProfile = mac::grabIccProfile(window->winId());
    }
    if (iccConvertProfile.length())
    {
        color = QColor::fromRgb(lcms2::convertColor(color.rgb(), iccProfile, iccConvertProfile));
        iccProfile = iccConvertProfile;
    }
    // state
    {
        state = State{
          color,
          rect,
          magnify,
          buffer,
          cursor,
          screen->geometry().topLeft(),
          displayNumber,
          iccProfile,
          iccDisplayProfile,
        };
    }
    view();
    widget();
}

void
ColorpickerPrivate::view()
{
    qreal dpr = state.buffer.devicePixelRatio();
    const QBrush blackBrush = QBrush(Qt::black);
    QColor color;
    QPixmap buffer;
    // icc profile
    {
        QString iccProfile;
        if (iccProfile != iccDisplayProfile)
        {
            color = lcms2::convertColor(state.color.rgb(), state.iccProfile, iccDisplayProfile);
            buffer = lcms2::convertPixmap(state.buffer, state.iccProfile, iccDisplayProfile);
        }
        else
        {
            color = state.color;
            buffer = state.buffer;
        }
    }
    // pixmap
    QPixmap pixmap(width * dpr, height * dpr);
    pixmap.setDevicePixelRatio(dpr);
    {
        QPainter p(&pixmap);
        p.save();
        p.scale(state.magnify, state.magnify);
        p.fillRect(QRect(0, 0, width, height), blackBrush);
        p.drawPixmap(0, 0, buffer);
        p.setPen(QPen(Qt::NoPen));
        p.fillRect(state.rect, QBrush(color));
        QTransform transform = p.transform();
        p.restore();
        QRect frame = QRect(transform.mapRect(state.rect));
        p.setPen(QPen(Qt::gray));
        p.setBrush(QBrush(Qt::NoBrush));
        p.drawRect(frame);
        p.end();
    }
    state.iccPixmap = QPixmap(pixmap);
    ui->view->setPixmap(pixmap);
}

void
ColorpickerPrivate::widget()
{
    // display
    {
        ui->display->setText(QString("Display #%1").arg(state.displayNumber));
        QFontMetrics metrics(ui->iccProfile->font());
        QString text = metrics.elidedText(QFileInfo(state.iccProfile).baseName(), Qt::ElideRight, ui->iccProfile->width());
        ui->iccProfile->setText(text);
    }
    // rgb
    {
        ui->r->setText(QString("%1").arg(formatRgb(state.color, RgbChannel::R)));
        ui->g->setText(QString("%1").arg(formatRgb(state.color, RgbChannel::G)));
        ui->b->setText(QString("%1").arg(formatRgb(state.color, RgbChannel::B)));
    }
    // hsv
    {
        ui->h->setText(QString("%1").arg(formatHsv(state.color, HsvChannel::H)));
        ui->s->setText(QString("%1").arg(formatHsv(state.color, HsvChannel::S)));
        ui->v->setText(QString("%1").arg(formatHsv(state.color, HsvChannel::V)));
    }
    // mouse location
    {
        QPoint screenpos = state.cursor - state.origin;
        ui->mouseLocation->setText(QString("(%1, %2)").arg(screenpos.x()).arg(screenpos.y()));
    }
    // colorwheel
    {
        QList<QPair<QColor,QString>> colors = asColors();
        if (active)
        {
            // push back current state as active color
            colors.push_back(QPair<QColor,QString>(state.color, QFileInfo(state.iccProfile).baseName()));
            ui->colorWheel->setColors(colors, true);
        }
        else
        {
            ui->colorWheel->setColors(colors, false);
        }
    }
    // picker
    if (picker->isVisible())
    {
        // threshold for border color contrast
        if (state.color.valueF() < 0.2f) {
            picker->setBorderColor(Qt::white);
        } else {
            picker->setBorderColor(Qt::black);
        }
        picker->setColor(state.color);
        picker->update(cursor);
    }
}

void
ColorpickerPrivate::activate()
{
    ui->active->setChecked(true);
}

void
ColorpickerPrivate::deactivate()
{
    ui->active->setChecked(false);
}

bool
ColorpickerPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::ScreenChangeInternal)
    {
        iccDisplayProfile = mac::grabIccProfile(window->winId());
        view();
        widget();
    }
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Escape) {
            deactivate();
        }
        else if (keyEvent->key() == Qt::Key_Plus) {
            QSlider* slider = ui->aperture;
            {
                slider->setSliderPosition(slider->value() + slider->singleStep());
            }
        }
        else if (keyEvent->key() == Qt::Key_Minus) {
            QSlider* slider = ui->aperture;
            {
                slider->setSliderPosition(slider->value() - slider->singleStep());
            }
        }
    }
    if (active)
    {
        if (event->type() == QEvent::QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            if (mouseEvent->button() == Qt::RightButton) {
                deactivate();
            }
        }
    }
    else
    {
        if (event->type() == QEvent::QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            if (mouseEvent->button() == Qt::LeftButton) {
                
                Colorwheel* colorWheel = ui->colorWheel;
                if (colorWheel->underMouse())
                {
                    selected = colorWheel->mapToSelected(
                        colorWheel->mapFrom(window, mouseEvent->pos())
                    );
                    if (selected >= 0) {
                        
                        colorWheel->setSelected(selected);
                        state = states[selected];
                        view();
                        widget();
                    }
                }
            }
        }
        if (event->type() == QEvent::QEvent::MouseMove)
        {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            if (mouseEvent->buttons() & Qt::LeftButton)
            {
                Colorwheel* colorWheel = ui->colorWheel;
                if (colorWheel->underMouse())
                {
                    if (selected >= 0) {
                        
                        QColor color = colorWheel->mapToColor(
                            state.color, colorWheel->mapFrom(window, mouseEvent->pos())
                        );
                        state.color = color;
                        states[selected] = state;
                        view();
                        widget();
                    }
                }
            }
        }
    }
    if (event->type() == QEvent::Close)
    {
        lcms2::clear();
    }
    return false;
}

bool
ColorpickerPrivate::blocked()
{
    QWidget* widget = QApplication::activeModalWidget();
    if (widget) {
        return widget == window.data() || widget->isAncestorOf(widget) || widget->window() == widget->window();
    }
    return false;
}

void
ColorpickerPrivate::toggleActive(bool checked)
{
    ui->colorWheel->setColors(asColors());
    if (checked) {
        emit readOnly(true);
    } else {
        if (selected >= 0) {
            emit readOnly(false);
        } else {
            emit readOnly(true);
        }
    }
    active = checked;

}

void
ColorpickerPrivate::toggleDisplay()
{
    int height = ui->displayWidget->height();
    if (ui->toggleDisplay->isChecked()) {
        ui->toggleDisplay->setIcon(QIcon(":/icons/resources/Collapse.png"));
        ui->displayWidget->show();
        window->setFixedSize(window->width(), size.height() + height);

    } else {
        ui->toggleDisplay->setIcon(QIcon(":/icons/resources/Expand.png"));
        ui->displayWidget->hide();
        window->setFixedSize(window->width(), size.height() - height);
    }
    size = window->size();
}

void
ColorpickerPrivate::togglePin(bool checked)
{
    if (checked) {
        window->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    } else {
        window->setWindowFlag(Qt::WindowStaysOnTopHint, false);
    }
    window->show();
}

void
ColorpickerPrivate::pick()
{
    selected = ui->colorWheel->selected();
    states.push_back(state);
}

void
ColorpickerPrivate::closed()
{
    deactivate();
}

void
ColorpickerPrivate::togglePick()
{
    if (picker->isVisible())
    {
        deactivate();
        {
            picker->hide();
        }
    }
    else
    {
        activate();
        {
            picker->setColor(state.color);
            picker->update(cursor);
            picker->show();
        }
    }
}

void
ColorpickerPrivate::copyRGB()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(
        QString("%1").arg(formatRgb(state.color, RgbChannel::R)) +
        ", " +
        QString("%1").arg(formatRgb(state.color, RgbChannel::G)) +
        ", " +
        QString("%1").arg(formatRgb(state.color, RgbChannel::B)));
}

void
ColorpickerPrivate::copyHSV()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(
        QString("%1").arg(formatHsv(state.color, HsvChannel::H)) +
        ", " +
        QString("%1").arg(formatHsv(state.color, HsvChannel::S)) +
        ", " +
        QString("%1").arg(formatHsv(state.color, HsvChannel::V)));
}

void
ColorpickerPrivate::copyIccProfile()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(iccProfile);
}

void
ColorpickerPrivate::copyColor()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QImage image(QSize(width, height), QImage::Format_RGB888);
    image.fill(state.color); // color without display profile
    clipboard->setImage(image);
}

void
ColorpickerPrivate::as8bit()
{
    format = ColorpickerPrivate::Int8bit;
    widget();
}

void
ColorpickerPrivate::as10bit()
{
    format = ColorpickerPrivate::Int10bit;
    widget();
}

void
ColorpickerPrivate::asFloat()
{
    format = ColorpickerPrivate::Float;
    widget();
}

void
ColorpickerPrivate::asHex()
{
    format = ColorpickerPrivate::Hex;
    widget();
}

void
ColorpickerPrivate::asPercentage()
{
    format = ColorpickerPrivate::Percentage;
    widget();
}

void
ColorpickerPrivate::magnify1x()
{
    magnify = 1.0;
    update();
}

void
ColorpickerPrivate::magnify2x()
{
    magnify = 2.0;
    update();
}

void
ColorpickerPrivate::magnify3x()
{
    magnify = 3.0;
    update();
}

void
ColorpickerPrivate::magnify4x()
{
    magnify = 4.0;
    update();
}

void
ColorpickerPrivate::magnify5x()
{
    magnify = 5.0;
    update();
}

void
ColorpickerPrivate::toggleMouseLocation()
{
    if (ui->mouseLocation->isVisible())
        ui->mouseLocation->setVisible(false);
    else
        ui->mouseLocation->setVisible(true);
}

void
ColorpickerPrivate::iccConvertProfileChanged(int index)
{
    iccConvertProfile = ui->iccConvertProfile->itemData(index, Qt::UserRole).value<QString>();
    update();
}

void
ColorpickerPrivate::toggleColors()
{
    int height = ui->colorsWidget->height();
    if (ui->toggleColors->isChecked()) {
        ui->toggleColors->setIcon(QIcon(":/icons/resources/Collapse.png"));
        ui->colorsWidget->show();
        window->setFixedSize(window->width(), size.height() + height);
    } else {
        
        ui->toggleColors->setIcon(QIcon(":/icons/resources/Expand.png"));
        ui->colorsWidget->hide();
        window->setFixedSize(window->width(), size.height() - height);
    }
    size = window->size();
}

void
ColorpickerPrivate::toggleRGB()
{
    if (!editor->isVisible())
    {
        editor->setMaximum(std::numeric_limits<uint8_t>::max());
        editor->setValue(channelRgb(state.color, edit.rgbChannel)*editor->maximum());
        editor->move(cursor.x() - editor->width()/2, cursor.y() + 4);
        editor->show();
    }
    else
    {
        editor->hide();
    }
}

void
ColorpickerPrivate::toggleR()
{
    if (!active && selected >= 0)
    {
        edit = Edit {
            .rgbChannel = RgbChannel::R,
            .type = Edit::Rgb
        };
        toggleRGB();
    }
}

void
ColorpickerPrivate::toggleG()
{
    if (!active && selected >= 0)
    {
        edit = Edit {
            .rgbChannel = RgbChannel::G,
            .type = Edit::Rgb
        };
        toggleRGB();
    }
}

void
ColorpickerPrivate::toggleB()
{
    if (!active && selected >= 0)
    {
        edit = Edit {
            .rgbChannel = RgbChannel::B,
            .type = Edit::Rgb
        };
        toggleRGB();
    }
}

void
ColorpickerPrivate::toggleHSV()
{
    if (!editor->isVisible())
    {
        if (edit.hsvChannel == HsvChannel::H)
        {
            editor->setMaximum(360);
        }
        else
        {
            editor->setMaximum(100);
        }
        editor->setValue(channelHsv(state.color, edit.hsvChannel)*editor->maximum());
        editor->move(cursor.x() - editor->width()/2, cursor.y() + 4);
        editor->show();
    }
    else
    {
        editor->hide();
    }
}

void
ColorpickerPrivate::toggleH()
{
    if (!active && selected >= 0)
    {
        edit = Edit {
            .hsvChannel = HsvChannel::H,
            .type = Edit::Hsv
        };
        toggleHSV();
    }
}


void
ColorpickerPrivate::toggleS()
{
    if (!active && selected >= 0)
    {
        edit = Edit {
            .hsvChannel = HsvChannel::S,
            .type = Edit::Hsv
        };
        toggleHSV();
    }
}


void
ColorpickerPrivate::toggleV()
{
    if (!active && selected >= 0)
    {
        edit = Edit {
            .hsvChannel = HsvChannel::V,
            .type = Edit::Hsv
        };
        toggleHSV();
    }
}

void
ColorpickerPrivate::apertureChanged(int value)
{
    activate();
    {
        aperture = value;
        update();
    }
}

void
ColorpickerPrivate::markerSizeChanged(int value)
{
    ui->colorWheel->setMarkerSize((qreal)value / ui->markerSize->maximum());
    update();
}

void
ColorpickerPrivate::backgroundOpacityChanged(int value)
{
    ui->colorWheel->setBackgroundOpacity((qreal)value / ui->backgroundOpacity->maximum());
    update();
}

void
ColorpickerPrivate::angleChanged(int value)
{
    ui->colorWheel->setAngle((qreal)value / ui->angle->maximum());
    update();
}

void
ColorpickerPrivate::iqlineChanged(int state)
{
    if (state == Qt::Checked)
        ui->colorWheel->setIQLineVisible(true);
    else
        ui->colorWheel->setIQLineVisible(false);
}

void
ColorpickerPrivate::zoomChanged(int state)
{
    if (state == Qt::Checked)
        ui->colorWheel->setZoomFactor(2);
    else
        ui->colorWheel->setZoomFactor(1);
}

void
ColorpickerPrivate::saturationChanged(int state)
{
    if (state == Qt::Checked)
        ui->colorWheel->setSaturationVisible(true);
    else
        ui->colorWheel->setSaturationVisible(false);
}

void
ColorpickerPrivate::labelsChanged(int state)
{
    if (state == Qt::Checked)
        ui->colorWheel->setLabelsVisible(true);
    else
        ui->colorWheel->setLabelsVisible(false);
}

void
ColorpickerPrivate::editorChanged(int value)
{
    if (!editor->isVisible())
        return;
    
    // color
    qreal channel = (qreal)value/editor->maximum();
    if (edit.type == Edit::Rgb)
    {
        if (edit.rgbChannel == RgbChannel::R) {
            state.color.setRedF(channel);
        }
        if (edit.rgbChannel == RgbChannel::G) {
            state.color.setGreenF(channel);
        }
        if (edit.rgbChannel == RgbChannel::B) {
            state.color.setBlueF(channel);
        }
    }
    else
    {
        if (edit.hsvChannel == HsvChannel::H) {
            state.color.setHsvF(
                channel,
                state.color.saturationF(),
                state.color.valueF());
        }
        if (edit.hsvChannel == HsvChannel::S) {
            state.color.setHsvF(
                state.color.hueF(),
                channel,
                state.color.valueF());
        }
        if (edit.hsvChannel == HsvChannel::V) {
            state.color.setHsvF(
                state.color.hueF(),
                state.color.saturationF(),
                channel);
        }
    }
    states[selected] = state;
    view();
    widget();
}

void
ColorpickerPrivate::clear()
{
    states.clear();
    ui->colorWheel->setColors(asColors());
    activate();
    update();
}

void
ColorpickerPrivate::pdf()
{
    QDateTime datetime = QDateTime::currentDateTime();
    QString datestamp =
        QString("%2 at %3").
        arg(datetime.toString("yyyy-MM-dd")).
        arg(datetime.toString("hh.mm.ss"));
    
    QString filename =
        QString("%1/Colorpicker %2.pdf").
        arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).
        arg(datestamp);
    
    QTextDocument* doc = new QTextDocument;
    doc->setDocumentMargin(10);
    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::Start);
    
    // colorpicker
    {
        QDir resources(QApplication::applicationDirPath() + "/../Resources");
        QString image = resources.absolutePath() + "/AppIcon.png";
        QTextImageFormat imageformat;
        imageformat.setName(image);
        imageformat.setWidth(64);
        
        cursor.insertImage(imageformat);
        cursor.insertHtml("<br>");
        cursor.insertHtml(QString("<h4>Colorpicker %1</h4>").arg(datestamp));
        cursor.insertHtml("<br>");
    }
    // table
    {
        QTextTable* table = cursor.insertTable(2, 1);
        QTextCharFormat headerformat;
        headerformat.setForeground(window->palette().text());
        headerformat.setBackground(window->palette().base());
        
        // format
        {
            qreal padding = 5;
            QVector<QTextLength> columns = {
                QTextLength(QTextLength::FixedLength, 454+padding*2)
            };
            QTextTableFormat format;
            format.setBorder(1.0);
            format.setBorderCollapse(true);
            format.setCellPadding(padding);
            format.setColumnWidthConstraints(columns);
            table->setFormat(format);
        }
        // colorpicker
        {
            QTextTableCell cell = table->cellAt(0, 0);
            QTextCursor cellcursor = cell.firstCursorPosition();
            cell.setFormat(headerformat);
            cellcursor.insertHtml(QString("<h5 style='color:rgb(255, 255, 255)'>Colors</h5>"));
        }
        // image
        {
            QTextTableCell cell = table->cellAt(1, 0);
            QTextCursor cellcursor = cell.firstCursorPosition();
            QPixmap widget(ui->colorWheel->size()*ui->colorWheel->devicePixelRatio());
            widget.setDevicePixelRatio(ui->colorWheel->devicePixelRatio());
            ui->colorWheel->render(&widget);
            QImage image = widget.toImage();
            QString format = "png";
            
            QTextImageFormat imageformat;
            imageformat.setWidth(ui->colorWheel->width()/2);
            imageformat.setHeight(ui->colorWheel->height()/2);
            imageformat.setName(QString("data:image/%1;base64,%2").
                arg(QString("%1.%2").arg(rand()).arg(format)).
                arg(asBase64(image, format).data()));
            
            cellcursor.insertImage(imageformat);
        }
    }
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml("<br>");
    // table
    {
        QTextTable* table = cursor.insertTable(static_cast<int>(states.count()) + 1, 5);
        QTextCharFormat headerformat;
        headerformat.setForeground(window->palette().text());
        headerformat.setBackground(window->palette().base());
        
        // format
        {
            qreal padding = 5;
            QVector<QTextLength> columns = {
                QTextLength(QTextLength::FixedLength, 40),
                QTextLength(QTextLength::FixedLength, 64+padding*2),
                QTextLength(QTextLength::FixedLength, 60),
                QTextLength(QTextLength::FixedLength, 60),
                QTextLength(QTextLength::FixedLength, 230)
            };
            QTextTableFormat format;
            format.setBorder(1.0);
            format.setBorderCollapse(true);
            format.setCellPadding(padding);
            format.setColumnWidthConstraints(columns);
            table->setFormat(format);
        }
        // index
        {
            QTextTableCell cell = table->cellAt(0, 0);
            QTextCursor cellcursor = cell.firstCursorPosition();
            cell.setFormat(headerformat);
            cellcursor.insertHtml("<h5 style='color:rgb(255, 255, 255)'>Index</h5>");
        }
        // image
        {
            QTextTableCell cell = table->cellAt(0, 1);
            QTextCursor cellcursor = cell.firstCursorPosition();
            cell.setFormat(headerformat);
            cellcursor.insertHtml("<h5 style='color:rgb(255, 255, 255)'>Image</h5>");
        }
        // hsv
        {
            QTextTableCell cell = table->cellAt(0, 2);
            QTextCursor cellcursor = cell.firstCursorPosition();
            cell.setFormat(headerformat);
            cellcursor.insertHtml("<h5 style='color:rgb(255, 255, 255)'>HSV</h5>");
        }
        // rgb
        {
            QTextTableCell cell = table->cellAt(0, 3);
            QTextCursor cellcursor = cell.firstCursorPosition();
            cell.setFormat(headerformat);
            cellcursor.insertHtml("<h5 style='color:rgb(255, 255, 255)'>RGB</h5>");
        }
        // display
        {
            QTextTableCell cell = table->cellAt(0, 4);
            QTextCursor cellcursor = cell.firstCursorPosition();
            cell.setFormat(headerformat);
            cellcursor.insertHtml("<h5 style='color:rgb(255, 255, 255)'>Display</h5>");
        }
        // states
        for(int i=0; i<states.count(); i++) {
            State state = states[i];
            // index
            {
                QTextTableCell cell = table->cellAt(i+1, 0);
                QTextCursor cellcursor = cell.firstCursorPosition();
                cellcursor.insertHtml(QString("<small>%1</small>").arg(i+1));
            }
            // image
            {
                QTextTableCell cell = table->cellAt(i+1, 1);
                QTextCursor cellcursor = cell.firstCursorPosition();
                QImage image = state.iccPixmap.toImage();
                QString format = "png";
                
                QTextImageFormat imageformat;
                imageformat.setWidth(64);
                imageformat.setHeight(64);
                imageformat.setName(QString("data:image/%1;base64,%2").
                    arg(QString("%1.%2").arg(rand()).arg(format)).
                    arg(asBase64(image, format).data()));
            
                cellcursor.insertImage(imageformat);
            }
            // hsv
            {
                QTextTableCell cell = table->cellAt(i+1, 2);
                QTextCursor cellcursor = cell.firstCursorPosition();
                cellcursor.insertHtml(QString("<small><b>H:</b> %1%2</small>").arg(formatHsv(state.color, HsvChannel::H)).arg("<br>"));
                cellcursor.insertHtml(QString("<small><b>S:</b> %1%2</small>").arg(formatHsv(state.color, HsvChannel::S)).arg("<br>"));
                cellcursor.insertHtml(QString("<small><b>V:</b> %1%2</small>").arg(formatHsv(state.color, HsvChannel::V)).arg("<br>"));
            }
            // rgb
            {
                QTextTableCell cell = table->cellAt(i+1, 3);
                QTextCursor cellcursor = cell.firstCursorPosition();
                cellcursor.insertHtml(QString("<small><b>R:</b> %1%2</small>").arg(formatRgb(state.color, RgbChannel::R)).arg("<br>"));
                cellcursor.insertHtml(QString("<small><b>G:</b> %1%2</small>").arg(formatRgb(state.color, RgbChannel::G)).arg("<br>"));
                cellcursor.insertHtml(QString("<small><b>B:</b> %1%2</small>").arg(formatRgb(state.color, RgbChannel::B)).arg("<br>"));
            }
            // display
            {
                QTextTableCell cell = table->cellAt(i+1, 4);
                QTextCursor cellcursor = cell.firstCursorPosition();
                cellcursor.insertHtml(QString("<small><b>Display:</b> #%1%2</small>").arg(state.displayNumber).arg("<br>"));
                cellcursor.insertHtml(QString("<small><b>Color profile:</b> %1%2</small>").arg(QFileInfo(state.iccProfile).fileName()).arg("<br>"));
                cellcursor.insertHtml(QString("<small><b>Display profile:</b> %1%2</small>").arg(QFileInfo(state.iccDisplayProfile).fileName()).arg("<br>"));
            }
        }
    }
    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock();
    // print
    {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        printer.setPageSize(QPageSize::A4);
        printer.setColorMode(QPrinter::Color);
        printer.setResolution(300);
        doc->print(&printer);
        QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
    }
}

void
ColorpickerPrivate::about()
{
    QPointer<About> about = new About(window.data());
    about->exec();
}

void
ColorpickerPrivate::openGithubReadme()
{
    QDesktopServices::openUrl(QUrl("https://github.com/mikaelsundell/colorpicker/blob/master/README.md"));
}

void
ColorpickerPrivate::openGithubIssues()
{
    QDesktopServices::openUrl(QUrl("https://github.com/mikaelsundell/colorpicker/issues"));
}

float
ColorpickerPrivate::channelRgb(QColor color, RgbChannel channel)
{
    switch (channel)
    {
        case RgbChannel::R:
        {
            return color.redF();
        }
        break;
        case RgbChannel::G:
        {
            return color.greenF();
        }
        break;
        case RgbChannel::B:
        {
            return color.blueF();
        }
        break;
    }
}

float
ColorpickerPrivate::channelHsv(QColor color, HsvChannel channel)
{
    switch (channel)
    {
        case HsvChannel::H:
        {
            return qMax<float>(0, color.hueF());
        }
        break;
        case HsvChannel::S:
        {
            return color.saturationF();
        }
        break;
        case HsvChannel::V:
        {
            return color.valueF();
        }
        break;
    }
}


QString
ColorpickerPrivate::formatRgb(QColor color, RgbChannel channel)
{
    switch (format)
    {
        case Format::Int8bit:
        {
            float scale = std::numeric_limits<uint8_t>::max();
            return QString::number(int(channelRgb(color, channel) * scale));
        }
        break;
        case Format::Int10bit:
        {
            float scale = pow(2, 10) - 1; // 10-bit max value
            return QString::number(int(channelRgb(color, channel) * scale));
        }
        break;
        case Format::Float:
        {
            return QString("%1").arg(asFloat(channelRgb(color, channel)));
        }
        break;
        case Format::Hex:
        {
            float scale = std::numeric_limits<uint8_t>::max();
            return QString("0x%1").arg(asHex(channelRgb(color, channel) * scale));
        }
       break;
        case Format::Percentage:
        {
            return QString("%1%").arg(asPercentage(channelRgb(color, channel)));
       }
       break;
    }
}

QString
ColorpickerPrivate::formatHsv(QColor hsv, HsvChannel channel)
{
    if (channel == HsvChannel::H)
    {
        return QString("%1°").arg(asDegree(channelHsv(hsv, channel) * 360));
    }
    else
    {
        return QString("%1%").arg(asPercentage(channelHsv(hsv, channel)));
    }
}

QString
ColorpickerPrivate::asFloat(float channel)
{
    return QString("%1").arg(channel, 0, 'f', 2);
}

QString
ColorpickerPrivate::asHex(int color)
{
    return QString("%1").arg(color, 2, 16, QLatin1Char('0')).toUpper();
}

QString
ColorpickerPrivate::asPercentage(float channel)
{
    return QString("%1").arg(channel * 100, 0, 'f', 0);
}

QString
ColorpickerPrivate::asDegree(float channel)
{
    return QString("%1").arg(channel, 0, 'f', 0);
}

QByteArray
ColorpickerPrivate::asBase64(const QImage& image, QString format)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, format.toLocal8Bit().data());
    buffer.close();
    QByteArray base64 = bytes.toBase64();
    QByteArray base64l;
    for (int i=0; i<base64.size(); i++) {
        base64l.append(base64[i]);
        if (i%80 == 0) {
            base64l.append("\n");
        }
    }
    return base64l;
}

QList<QPair<QColor, QString>>
ColorpickerPrivate::asColors()
{
    QList<QPair<QColor, QString>> colors;
    for(State state : states)
    {
        colors.push_back(
            QPair<QColor,QString>(state.color, QFileInfo(state.iccProfile).baseName())
        );
    }
    return colors;
}

#include "colorpicker.moc"

Colorpicker::Colorpicker()
: QMainWindow(nullptr,
  Qt::WindowTitleHint |
  Qt::CustomizeWindowHint |
  Qt::WindowCloseButtonHint |
  Qt::WindowMinimizeButtonHint |
  Qt::WindowStaysOnTopHint)
, p(new ColorpickerPrivate())
{
    p->window = this;
    p->init();
    registerEvents();
}

Colorpicker::~Colorpicker()
{
}

bool
Colorpicker::active() const
{
    return p->active;
}

void
Colorpicker::pickEvent(PickEvent event)
{
    if (!p->blocked()) {
        p->displayNumber = event.displayNumber;
        p->iccProfile = event.iccProfile;
        p->cursor = event.cursor;
        p->update();
    }
}

void
Colorpicker::moveEvent(MoveEvent event)
{
    if (!p->blocked()) {
        p->cursor = event.cursor;
        p->update();
    }
}
