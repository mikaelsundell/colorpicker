// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorpicker.h"
#include "lcms2.h"
#include "mac.h"

#include <QScreen>
#include <QDesktopWidget>
#include <QPainter>
#include <QDebug>
#include <QFileInfo>
#include <QAction>
#include <QMenu>
#include <QPointer>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QDir>

// generated files
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
    
    public Q_SLOTS:
        void toggleFreeze();
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
        void openGithubReadme();
        void openGithubIssues();
    
    public:
        float channelRgb(QColor color, RgbChannel channel);
        float channelHsv(QColor color, HsvChannel channel);
        QString formatRgb(QColor color, RgbChannel channel);
        QString formatHsv(QColor color, HsvChannel channel);
        QString asFloat(float channel);
        QString asHex(int channel);
        QString asPercentage(float channel);
        QString asDegree(float channel);
        QPointer<QMainWindow> window;
        QColor color;
        int width;
        int height;
        int aperture;
        int magnify;
        int displayNumber;
        QString displayProfile;
        QString displayInProfile;
        QPoint mousepos;
        QPoint cursorpos;
        bool freeze;
        bool mouselocation;
        Format format;
        QScopedPointer<Ui_Colorpicker> ui;
};

ColorpickerPrivate::ColorpickerPrivate()
: width(128)
, height(128)
, aperture(1)
, magnify(1)
, freeze(false)
, mouselocation(true)
, format(Format::Int8bit)
{
}

void
ColorpickerPrivate::init()
{
    ui.reset(new Ui_Colorpicker());
    ui->setupUi(window);
    // resources
    QDir resources(QApplication::applicationDirPath() + "/../Resources");
    ui->displayInProfile->insertSeparator(ui->displayInProfile->count());
    for(QFileInfo resourceFile : resources.entryInfoList( QStringList( "*.icc" )))
    {
        ui->displayInProfile->addItem
            ("Display in " + resourceFile.baseName(), QVariant::fromValue(resourceFile.filePath()));
    }
    connect(ui->displayInProfile, SIGNAL(currentIndexChanged(int)), this, SLOT(displayInProfileChanged(int)));
    // connect
    connect(ui->copyRGBAsText, SIGNAL(triggered()), this, SLOT(copyRGB()));
    connect(ui->copyHSVAsText, SIGNAL(triggered()), this, SLOT(copyHSV()));
    connect(ui->copyProfileAsText, SIGNAL(triggered()), this, SLOT(copyDisplayProfile()));
    connect(ui->copyColorAsBitmap, SIGNAL(triggered()), this, SLOT(copyColor()));
    connect(ui->freeze, SIGNAL(triggered()), this, SLOT(toggleFreeze()));
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
    connect(ui->openGithubReadme, SIGNAL(triggered()), this, SLOT(openGithubReadme()));
    connect(ui->openGithubIssues, SIGNAL(triggered()), this, SLOT(openGithubIssues()));
}

void
ColorpickerPrivate::update()
{
    if (!freeze)
        cursorpos = QCursor::pos();

    // match available geometry for screen,
    // mac cursor is -1 offset from Qt
    QPoint pos(
        qMax(0, cursorpos.x() - 1),
        qMax(0, cursorpos.y() - 1)
    );
    
    int w = int(width / float(magnify));
    int h = int(height / float(magnify));
    int x = pos.x() - w / 2;
    int y = pos.y() - h / 2;
    
    if (width % magnify > 0)
        ++w;
    
    if (height % magnify > 0)
        ++h;
    
    QPixmap buffer;
    const QBrush blackBrush = QBrush(Qt::black);
    if (QScreen *screen = window->screen()) {
        buffer = mac::grabDisplayPixmap(x, y, w, h);
    } else {
        buffer = QPixmap(w, h);
        buffer.fill(blackBrush.color());
    }
    QRegion geom(x, y, w, h);
    QRect screenRect;
    
    const auto screen = QGuiApplication::screenAt(pos);
    const auto screens = QGuiApplication::screens();
    for (auto screen : screens)
        screenRect |= screen->geometry();
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

    QPixmap pixmap(width, height);
    {
        QPainter p(&pixmap);
        p.save();
        p.scale(magnify, magnify);
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
                QColor pixel = image.pixel(cx, cy);
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
    ui->view->setPixmap(pixmap);
    // display in profile
    if (displayInProfile.count() > 0)
    {
        color = lcms2::convertColor(color, displayProfile, displayInProfile);
    }
    // display
    {
        ui->display->setText(QString("Display #%1").arg(displayNumber));
        QFontMetrics metrics(ui->displayProfile->font());
        QString text = metrics.elidedText(QFileInfo(displayProfile).fileName(), Qt::ElideRight, ui->displayProfile->width());
        ui->displayProfile->setText(text);
    }
    // rgb
    {
        ui->r->setText(QString("%1").arg(formatRgb(color, RgbChannel::R)));
        ui->g->setText(QString("%1").arg(formatRgb(color, RgbChannel::G)));
        ui->b->setText(QString("%1").arg(formatRgb(color, RgbChannel::B)));
    }
    // hsv
    {
        QColor hsv = color.toHsv();
        ui->h->setText(QString("%1").arg(formatHsv(color, HsvChannel::H)));
        ui->s->setText(QString("%1").arg(formatHsv(color, HsvChannel::S)));
        ui->v->setText(QString("%1").arg(formatHsv(color, HsvChannel::V)));
    }
    // mouse location
    {
        QPoint screenpos = cursorpos - screen->geometry().topLeft();
        ui->mouseLocation->setText(QString("(%1, %2)").arg(screenpos.x()).arg(screenpos.y()));
    }
    mousepos = pos;
}

void
ColorpickerPrivate::toggleFreeze()
{
    if (freeze)
        freeze = false;
    else
        freeze = true;
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
    p->update();
}
