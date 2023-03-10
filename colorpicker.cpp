// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorpicker.h"
#include "picker.h"
#include "lcms2.h"
#include "mac.h"

#include <QAction>
#include <QClipboard>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QScreen>
#include <QUrl>

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
        void update();
        void view();
        bool eventFilter(QObject* object, QEvent* event);
    
    public Q_SLOTS:
        void toggleFreeze(bool checked);
        void pick();
        void togglePick();
        void copyRGB();
        void copyHSV();
        void copyDisplayProfile();
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
        void displayInProfileChanged(int index);
        void apertureChanged(int value);
        void markerSizeChanged(int value);
        void backgroundOpacityChanged(int value);
        void angleChanged(int value);
        void iqlineChanged(int state);
        void saturationChanged(int state);
        void clear();
        void about();
        void openGithubReadme();
        void openGithubIssues();
    
    public:
        class About : public QDialog
        {
            public: About()
            {
                QScopedPointer<Ui_About> about;
                about.reset(new Ui_About());
                about->setupUi(this);
            }
        };
        class State
        {
            public:
            QColor color;
            QPixmap pixmap;
            QPoint cursor;
            QScreen* screen;
            int displayNumber;
            QString displayProfile;
        };
        float channelRgb(QColor color, RgbChannel channel);
        float channelHsv(QColor color, HsvChannel channel);
        QString formatRgb(QColor color, RgbChannel channel);
        QString formatHsv(QColor color, HsvChannel channel);
        QString asFloat(float channel);
        QString asHex(int channel);
        QString asPercentage(float channel);
        QString asDegree(float channel);
        QColor color;
        QList<QColor> colors;
        int width;
        int height;
        int aperture;
        int magnify;
        int displayNumber;
        QString displayProfile;
        QString displayInProfile;
        QPoint cursor;
        bool freeze;
        bool mouselocation;
        Format format;
        State state;
        QList<State> states;
        QPointer<Colorpicker> window;
        QScopedPointer<Picker> picker;
        QScopedPointer<Ui_Colorpicker> ui;
};

ColorpickerPrivate::ColorpickerPrivate()
: width(128)
, height(128)
, aperture(50)
, magnify(1)
, freeze(false)
, mouselocation(true)
, format(Format::Int8bit)
{
}

void
ColorpickerPrivate::init()
{
    mac::setupMac();
    // ui
    ui.reset(new Ui_Colorpicker());
    ui->setupUi(window);
    // picker
    picker.reset(new Picker());
    // resources
    QDir resources(QApplication::applicationDirPath() + "/../Resources");
    ui->displayInProfile->insertSeparator(ui->displayInProfile->count());
    for(QFileInfo resourceFile : resources.entryInfoList( QStringList( "*.icc" )))
    {
        ui->displayInProfile->addItem
            ("Display in " + resourceFile.baseName(), QVariant::fromValue(resourceFile.filePath()));
    }
    // stylesheet
    QFile stylesheet(resources.absolutePath() + "/App.css");
    stylesheet.open(QFile::ReadOnly);
    qApp->setStyleSheet(stylesheet.readAll());
    // event filter
    window->installEventFilter(this);
    // connect
    connect(ui->displayInProfile, SIGNAL(currentIndexChanged(int)), this, SLOT(displayInProfileChanged(int)));
    connect(ui->pick, SIGNAL(triggered()), this, SLOT(togglePick()));
    connect(ui->togglePick, SIGNAL(pressed()), this, SLOT(togglePick()));
    connect(ui->copyRGBAsText, SIGNAL(triggered()), this, SLOT(copyRGB()));
    connect(ui->copyHSVAsText, SIGNAL(triggered()), this, SLOT(copyHSV()));
    connect(ui->copyProfileAsText, SIGNAL(triggered()), this, SLOT(copyDisplayProfile()));
    connect(ui->copyColorAsBitmap, SIGNAL(triggered()), this, SLOT(copyColor()));
    connect(ui->freeze, SIGNAL(toggled(bool)), this, SLOT(toggleFreeze(bool)));
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
    connect(ui->saturation, SIGNAL(stateChanged(int)), this, SLOT(saturationChanged(int)));
    connect(ui->clear, SIGNAL(pressed()), this, SLOT(clear()));
    connect(ui->about, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->openGithubReadme, SIGNAL(triggered()), this, SLOT(openGithubReadme()));
    connect(ui->openGithubIssues, SIGNAL(triggered()), this, SLOT(openGithubIssues()));
    connect(picker.get(), SIGNAL(triggered()), this, SLOT(pick()));
    // pixmaps
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);
}

void
ColorpickerPrivate::update()
{
    if (freeze)
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
    QPixmap pixmap(width * dpr, height * dpr);
    pixmap.setDevicePixelRatio(dpr);
    {
        QPainter p(&pixmap);
        p.save();
        p.scale(magnify, magnify);
        p.fillRect(QRect(0, 0, width, height), blackBrush);
        p.drawPixmap(0, 0, buffer);
        p.setPen(QPen(Qt::NoPen));
        QRect colorRect(
           (w - aperture) / 2,
           (h - aperture) / 2,
           aperture, aperture
        );
        int colorR=0, colorG=0, colorB=0;
        QImage image = buffer.toImage();
        for(int cx = colorRect.left(); cx <= colorRect.right(); cx++)
        {
            for(int cy = colorRect.top(); cy <= colorRect.bottom(); cy++)
            {
                QColor pixel = image.pixel(cx * dpr, cy * dpr);
                colorR += pixel.red();
                colorG += pixel.green();
                colorB += pixel.blue();
            }
        }
        int size = colorRect.width() * colorRect.height();
        color = QColor(colorR / size, colorG / size, colorB / size);
        p.fillRect(colorRect, QBrush(color));
        QTransform transform = p.transform();
        p.restore();
        QRect frameRect(transform.mapRect(colorRect));
        frameRect.adjust(-1, -1, 0, 0);
        p.setPen(QPen(Qt::gray));
        p.setBrush(QBrush(Qt::NoBrush));
        p.drawRect(frameRect);
        p.end();
    }
    // display in profile
    if (displayInProfile.count() > 0)
    {
        color = QColor::fromRgb(lcms2::convertColor(color.rgb(), state.displayProfile, displayInProfile));
    }
    // state
    {
        state = State{
          color,
          pixmap,
          cursor,
          screen,
          displayNumber,
          displayProfile
        };
    }
    view();
}

void
ColorpickerPrivate::view()
{
    ui->view->setPixmap(state.pixmap);
    // display
    {
        ui->display->setText(QString("Display #%1").arg(state.displayNumber));
        QFontMetrics metrics(ui->displayProfile->font());
        QString text = metrics.elidedText(QFileInfo(state.displayProfile).fileName(), Qt::ElideRight, ui->displayProfile->width());
        ui->displayProfile->setText(text);
    }
    // rgb
    {
        ui->r->setText(QString("%1").arg(formatRgb(state.color, RgbChannel::R)));
        ui->g->setText(QString("%1").arg(formatRgb(state.color, RgbChannel::G)));
        ui->b->setText(QString("%1").arg(formatRgb(state.color, RgbChannel::B)));
    }
    // hsv
    {
        QColor hsv = color.toHsv();
        ui->h->setText(QString("%1").arg(formatHsv(state.color, HsvChannel::H)));
        ui->s->setText(QString("%1").arg(formatHsv(state.color, HsvChannel::S)));
        ui->v->setText(QString("%1").arg(formatHsv(state.color, HsvChannel::V)));
    }
    // mouse location
    {
        QPoint screenpos = state.cursor - state.screen->geometry().topLeft();
        ui->mouseLocation->setText(QString("(%1, %2)").arg(screenpos.x()).arg(screenpos.y()));
    }
    // colorwheel
    {
        QList<QColor> active = QList<QColor>(colors);
        active.push_back(state.color);
        ui->widget->setColors(active);
    }
    // picker
    if (picker->isVisible())
    {
        picker->setColor(state.color);
        picker->move(cursor.x() - picker->width()/2, cursor.y() - picker->height()/2);
    }
}

bool
ColorpickerPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (freeze)
    {
        if (event->type() == QEvent::QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            if (mouseEvent->button() == Qt::LeftButton) {
                int selected = ui->widget->colorAt(
                    ui->widget->mapFrom(window, mouseEvent->pos()));
                
                if (selected >= 0) {
                    ui->widget->setSelected(selected);
                    state = states[selected];
                    view();
                }
            }
        }
    }
    return false;
}

void
ColorpickerPrivate::toggleFreeze(bool checked)
{
    if (checked)
        ui->widget->setColors(colors);
    
    freeze = checked;
}

void
ColorpickerPrivate::pick()
{
    colors.push_back(color);
    // state
    {
        update();
        states.push_back(state);
    }
}

void
ColorpickerPrivate::togglePick()
{
    if (picker->isVisible())
    {
        picker->hide();
    }
    else
    {
        if (ui->freeze->isChecked())
            ui->freeze->setChecked(false);
        
        // picker
        {
            picker->setColor(color);
            picker->move(cursor.x() - picker->width()/2, cursor.y() - picker->height()/2);
            picker->show();
        }
    }
}

void
ColorpickerPrivate::copyRGB()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(
        QString("%1").arg(formatRgb(color, RgbChannel::R)) +
        ", " +
        QString("%1").arg(formatRgb(color, RgbChannel::G)) +
        ", " +
        QString("%1").arg(formatRgb(color, RgbChannel::B)));
}

void
ColorpickerPrivate::copyHSV()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(
        QString("%1").arg(formatHsv(color, HsvChannel::H)) +
        ", " +
        QString("%1").arg(formatHsv(color, HsvChannel::S)) +
        ", " +
        QString("%1").arg(formatHsv(color, HsvChannel::V)));
}

void
ColorpickerPrivate::copyDisplayProfile()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(displayProfile);
}

void
ColorpickerPrivate::copyColor()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QImage image(QSize(width, height), QImage::Format_RGB888);
    image.fill(color); // color without display profile
    clipboard->setImage(image);
}

void
ColorpickerPrivate::as8bit()
{
    format = ColorpickerPrivate::Int8bit;
    update();
}

void
ColorpickerPrivate::as10bit()
{
    format = ColorpickerPrivate::Int10bit;
    update();
}

void
ColorpickerPrivate::asFloat()
{
    format = ColorpickerPrivate::Float;
    update();
}

void
ColorpickerPrivate::asHex()
{
    format = ColorpickerPrivate::Hex;
    update();
}

void
ColorpickerPrivate::asPercentage()
{
    format = ColorpickerPrivate::Percentage;
    update();
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
ColorpickerPrivate::displayInProfileChanged(int index)
{
    displayInProfile = ui->displayInProfile->itemData(index, Qt::UserRole).value<QString>();
    update();
}

void
ColorpickerPrivate::apertureChanged(int value)
{
    aperture = value;
    update();
}

void
ColorpickerPrivate::markerSizeChanged(int value)
{
    ui->widget->setMarkerSize((qreal)value / ui->markerSize->maximum());
    update();
}

void
ColorpickerPrivate::backgroundOpacityChanged(int value)
{
    ui->widget->setBackgroundOpacity((qreal)value / ui->backgroundOpacity->maximum());
    update();
}

void
ColorpickerPrivate::angleChanged(int value)
{
    ui->widget->setAngle((qreal)value / ui->angle->maximum());
    update();
}

void
ColorpickerPrivate::iqlineChanged(int state)
{
    if (state == Qt::Checked)
        ui->widget->setIQLineVisible(true);
    else
        ui->widget->setIQLineVisible(false);
}

void
ColorpickerPrivate::saturationChanged(int state)
{
    if (state == Qt::Checked)
        ui->widget->setSaturationVisible(true);
    else
        ui->widget->setSaturationVisible(false);
}

void
ColorpickerPrivate::clear()
{
    colors.clear();
    states.clear();
    if (ui->freeze->isChecked())
        ui->freeze->setChecked(false);

    update();
}

void
ColorpickerPrivate::about()
{
    QPointer<About> about = new About();
    about->show();
}

void
ColorpickerPrivate::openGithubReadme()
{
    QDesktopServices::openUrl(QUrl("https://github.com/mikaelsundell/colorpicker/blob/master/readme.md"));
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
        return QString("%1??").arg(asDegree(channelHsv(hsv, channel) * 360));
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

#include "colorpicker.moc"

Colorpicker::Colorpicker()
: QMainWindow(nullptr,
  Qt::WindowTitleHint |
  Qt::CustomizeWindowHint |
  Qt::WindowCloseButtonHint |
  Qt::WindowMinimizeButtonHint)
, p(new ColorpickerPrivate())
{
    p->window = this;
    p->init();
    registerEvents();
}

Colorpicker::~Colorpicker()
{
}

void
Colorpicker::update()
{
    p->update();
}

void
Colorpicker::updateEvents(DisplayEvent event)
{
    p->displayNumber = event.displayNumber;
    p->displayProfile = event.displayProfile;
    p->cursor = event.cursor;
    p->update();
}
