// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorpicker.h"
#include "picker.h"
#include "dragger.h"
#include "editor.h"
#include "eventfilter.h"
#include "icctransform.h"
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

// stdc++
#include <random>

// opencv
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>

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
        enum HslChannel
        {
            HslH, HslS, HslL
        };
        enum Format
        {
            Int8bit,
            Int10bit,
            Float,
            Hex,
            Percentage
        };
        enum Display
        {
            Hsv,
            Hsl
        };
        enum Mode
        {
            None,
            Pick,
            Drag
        };
    public:
        ColorpickerPrivate();
        void init();
        void stylesheet();
        void update();
        void view();
        void widget();
        void profile();
        void blank();
        void activate();
        void deactivate();
        bool eventFilter(QObject* object, QEvent* event);
        bool blocked();
    
    public Q_SLOTS:
        void toggleDisplay();
        void togglePin(bool checked);
        void toggleActive(bool checked);
        void pick();
        void drag();
        void pickClosed();
        void dragClosed();
        void togglePick();
        void toggleDrag();
        void copyRGB();
        void copyHSV();
        void copyHSL();
        void copyHEX();
        void copyIccProfile();
        void copyColor();
        void as8bitValues();
        void as10bitValues();
        void asFloatValues();
        void asHexValues();
        void asPercentageValues();
        void asHSVValues();
        void asHSLValues();
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
                about->version->setText(MACOSX_BUNDLE_LONG_VERSION_STRING);
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
                QImage image;
                QPoint cursor;
                QPoint origin;
                int displayNumber;
                QString iccProfile;
                QString iccDisplayProfile;
        };
        class Edit
        {
            public:
                enum Type
                {
                    Rgb,
                    Hsv
                };
                RgbChannel rgbChannel;
                HsvChannel hsvChannel;
                Type type;
        };
    
        QRect grabRect(QPoint cursor);
        QImage grabBuffer(QRect rect);
        bool underMouse(QWidget* widget);
        float channelRgb(QColor color, RgbChannel channel);
        float channelHsv(QColor color, HsvChannel channel);
        float channelHsl(QColor color, HslChannel channel);
        QString formatRgb(QColor color, RgbChannel channel);
        QString formatHsv(QColor color, HsvChannel channel);
        QString formatHsl(QColor color, HslChannel channel);
        QString asFloat(float channel);
        QString asHex(int channel);
        QString asPercentage(float channel);
        QString asDegree(float channel);
        QByteArray asBase64(const QImage& image, QString format);
        cv::Mat asBGR(const cv::Mat& mat);
        cv::Mat asFloat32(const cv::Mat& mat);
        QColor asColor(const cv::Vec3f& vec);
        QList<QPair<QColor,QString>> asColors();
        int width;
        int height;
        int aperture;
        int magnify;
        int displayNumber;
        QString iccProfile;
        QString iccWindowProfile;
        QString iccDisplayProfile;
        QPoint cursor;
        bool active;
        bool mouselocation;
        Format format;
        Display display;
        Mode mode;
        State state;
        Edit edit;
        int selected;
        QSize size;
        QList<State> states;
        QPointer<Colorpicker> window;
        QList<QColor> dragcolors;
        QList<QPoint> dragpositions;
        QScopedPointer<Picker> picker;
        QScopedPointer<Dragger> dragger;
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
, display(Display::Hsv)
, mode(Mode::None)
, selected(-1)
{
}

void
ColorpickerPrivate::init()
{
    mac::setDarkAppearance();
    // ui
    ui.reset(new Ui_Colorpicker());
    ui->setupUi(window);
    // layout
    // needed to keep .ui fixed size from setupUi
    window->setFixedSize(window->size());
    // utils
    picker.reset(new Picker(window.data()));
    dragger.reset(new Dragger(window.data()));
    // editor
    editor.reset(new Editor(window.data()));
    editor->setObjectName("editor");
    // resources
    QDir resources(QApplication::applicationDirPath() + "/../ICCProfiles");
    ui->iccColorProfile->insertSeparator(ui->iccColorProfile->count());
    for(QFileInfo resourceFile : resources.entryInfoList( QStringList( "*.icc" )))
    {
        ui->iccColorProfile->addItem
            ("Convert to " + resourceFile.baseName(), QVariant::fromValue(resourceFile.filePath()));
    }
    // actions
    ui->toggleActive->setDefaultAction(ui->active);
    ui->togglePin->setDefaultAction(ui->pin);
    // event filter
    window->installEventFilter(this);
    // display filter
    displayfilter.reset(new Eventfilter);
    ui->displayBar->installEventFilter(displayfilter.data());
    // color filter
    colorsfilter.reset(new Eventfilter);
    ui->colorsBar->installEventFilter(colorsfilter.data());
    // connect
    connect(displayfilter.data(), &Eventfilter::pressed, ui->toggleDisplay, &QPushButton::click);
    connect(colorsfilter.data(), &Eventfilter::pressed, ui->toggleColors, &QPushButton::click);
    connect(ui->toggleDisplay, &QPushButton::pressed, this, &ColorpickerPrivate::toggleDisplay);
    connect(ui->toggleColors, &QPushButton::pressed, this, &ColorpickerPrivate::toggleColors);
    connect(ui->iccColorProfile, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ColorpickerPrivate::iccConvertProfileChanged);
    connect(ui->r, &Label::triggered, this, &ColorpickerPrivate::toggleR);
    connect(ui->g, &Label::triggered, this, &ColorpickerPrivate::toggleG);
    connect(ui->b, &Label::triggered, this, &ColorpickerPrivate::toggleB);
    connect(ui->h, &Label::triggered, this, &ColorpickerPrivate::toggleH);
    connect(ui->s, &Label::triggered, this, &ColorpickerPrivate::toggleS);
    connect(ui->v, &Label::triggered, this, &ColorpickerPrivate::toggleV);
    connect(ui->pick, &QAction::triggered, this, &ColorpickerPrivate::togglePick);
    connect(ui->togglePick, &QPushButton::released, this, &ColorpickerPrivate::togglePick);
    connect(ui->drag, &QAction::triggered, this, &ColorpickerPrivate::toggleDrag);
    connect(ui->toggleDrag, &QPushButton::released, this, &ColorpickerPrivate::toggleDrag);
    connect(ui->copyRGBAsText, &QAction::triggered, this, &ColorpickerPrivate::copyRGB);
    connect(ui->copyHSVAsText, &QAction::triggered, this, &ColorpickerPrivate::copyHSV);
    connect(ui->copyHSLAsText, &QAction::triggered, this, &ColorpickerPrivate::copyHSL);
    connect(ui->copyHexAsText, &QAction::triggered, this, &ColorpickerPrivate::copyHEX);
    connect(ui->copyIccAsText, &QAction::triggered, this, &ColorpickerPrivate::copyIccProfile);
    connect(ui->copyColorAsBitmap, &QAction::triggered, this, &ColorpickerPrivate::copyColor);
    connect(ui->active, &QAction::toggled, this, &ColorpickerPrivate::toggleActive);
    connect(ui->pin, &QAction::toggled, this, &ColorpickerPrivate::togglePin);
    connect(ui->as8bitValues, &QAction::triggered, this, &ColorpickerPrivate::as8bitValues);
    connect(ui->as10bitValues, &QAction::triggered, this, &ColorpickerPrivate::as10bitValues);
    connect(ui->asFloatValues, &QAction::triggered, this, &ColorpickerPrivate::asFloatValues);
    connect(ui->asHexadecimalValues, &QAction::triggered, this, &ColorpickerPrivate::asHexValues);
    connect(ui->asPercentageValues, &QAction::triggered, this, &ColorpickerPrivate::asPercentageValues);
    {
        QActionGroup* actions = new QActionGroup(this);
        actions->setExclusive(true);
        {
            actions->addAction(ui->as8bitValues);
            actions->addAction(ui->as10bitValues);
            actions->addAction(ui->asFloatValues);
            actions->addAction(ui->asHexadecimalValues);
            actions->addAction(ui->asPercentageValues);
        }
    }
    connect(ui->asHSVDisplay, &QAction::triggered, this, &ColorpickerPrivate::asHSVValues);
    connect(ui->asHSLDisplay, &QAction::triggered, this, &ColorpickerPrivate::asHSLValues);
    {
        QActionGroup* actions = new QActionGroup(this);
        actions->setExclusive(true);
        {
            actions->addAction(ui->asHSVDisplay);
            actions->addAction(ui->asHSLDisplay);
        }
        
    }
    connect(ui->magnify1x, &QAction::triggered, this, &ColorpickerPrivate::magnify1x);
    connect(ui->magnify2x, &QAction::triggered, this, &ColorpickerPrivate::magnify2x);
    connect(ui->magnify3x, &QAction::triggered, this, &ColorpickerPrivate::magnify3x);
    connect(ui->magnify4x, &QAction::triggered, this, &ColorpickerPrivate::magnify4x);
    connect(ui->magnify5x, &QAction::triggered, this, &ColorpickerPrivate::magnify5x);
    {
        QActionGroup* actions = new QActionGroup(this);
        actions->setExclusive(true);
        for(QAction* action : ui->magnify->actions())
            actions->addAction(action);
    }
    connect(ui->toggleMouseLocation, &QAction::triggered, this, &ColorpickerPrivate::toggleMouseLocation);
    connect(ui->aperture, &QSlider::valueChanged, this, &ColorpickerPrivate::apertureChanged);
    connect(ui->markerSize, &QSlider::valueChanged, this, &ColorpickerPrivate::markerSizeChanged);
    connect(ui->backgroundOpacity, &QSlider::valueChanged, this, &ColorpickerPrivate::backgroundOpacityChanged);
    connect(ui->angle, &QSlider::valueChanged, this, &ColorpickerPrivate::angleChanged);
    connect(ui->iqline, &QCheckBox::stateChanged, this, &ColorpickerPrivate::iqlineChanged);
    connect(ui->zoom, &QCheckBox::stateChanged, this, &ColorpickerPrivate::zoomChanged);
    connect(ui->saturation, &QCheckBox::stateChanged, this, &ColorpickerPrivate::saturationChanged);
    connect(ui->labels, &QCheckBox::stateChanged, this, &ColorpickerPrivate::labelsChanged);
    connect(ui->clear, &QAction::triggered, this, &ColorpickerPrivate::clear);
    connect(ui->toggleClear, &QPushButton::pressed, this, &ColorpickerPrivate::clear);
    connect(ui->pdf, &QPushButton::pressed, this, &ColorpickerPrivate::pdf);
    connect(ui->about, &QAction::triggered, this, &ColorpickerPrivate::about);
    connect(ui->openGithubReadme, &QAction::triggered, this, &ColorpickerPrivate::openGithubReadme);
    connect(ui->openGithubIssues, &QAction::triggered, this, &ColorpickerPrivate::openGithubIssues);
    connect(picker.get(), &Picker::triggered, this, &ColorpickerPrivate::pick);
    connect(picker.get(), &Picker::closed, this, &ColorpickerPrivate::pickClosed);
    connect(dragger.get(), &Dragger::triggered, this, &ColorpickerPrivate::drag);
    connect(dragger.get(), &Dragger::closed, this, &ColorpickerPrivate::dragClosed);
    connect(editor.get(), &Editor::valueChanged, this, &ColorpickerPrivate::editorChanged);
    // signals
    connect(this, &ColorpickerPrivate::readOnly, ui->r, &Label::setReadOnly);
    connect(this, &ColorpickerPrivate::readOnly, ui->g, &Label::setReadOnly);
    connect(this, &ColorpickerPrivate::readOnly, ui->b, &Label::setReadOnly);
    connect(this, &ColorpickerPrivate::readOnly, ui->h, &Label::setReadOnly);
    connect(this, &ColorpickerPrivate::readOnly, ui->s, &Label::setReadOnly);
    connect(this, &ColorpickerPrivate::readOnly, ui->v, &Label::setReadOnly);
    size = window->size();
    profile();
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
    QString qss = stylesheet.readAll();
    QRegularExpression hslRegex("hsl\\(\\s*(\\d+)\\s*,\\s*(\\d+)%\\s*,\\s*(\\d+)%\\s*\\)");
    QString transformqss = qss;
    QRegularExpressionMatchIterator i = hslRegex.globalMatch(transformqss);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            if (!match.captured(1).isEmpty() &&
                !match.captured(2).isEmpty() &&
                !match.captured(3).isEmpty())
            {
                int h = match.captured(1).toInt();
                int s = match.captured(2).toInt();
                int l = match.captured(3).toInt();
                QColor color = QColor::fromHslF(h / 360.0f, s / 100.0f, l / 100.0f);
                // icc profile
                ICCTransform* transform = ICCTransform::instance();
                QDir resources(QApplication::applicationDirPath() + "/../Resources");
                QString iccColorProfile = resources.filePath("sRGB2014.icc"); // build-in Qt profile for colors
                color = transform->transformTo(color.rgb(), iccColorProfile, iccWindowProfile);
                QString hsl = QString("hsl(%1, %2%, %3%)")
                                .arg(color.hue() == -1 ? 0 : color.hue())
                                .arg(static_cast<int>(color.hslSaturationF() * 100))
                                .arg(static_cast<int>(color.lightnessF() * 100));
                
                transformqss.replace(match.captured(0), hsl);
            }
        }
    }
    qApp->setStyleSheet(transformqss);
}

QRect
ColorpickerPrivate::grabRect(QPoint pos)
{
    int w = int(width / float(magnify));
    int h = int(height / float(magnify));
    int x = pos.x() - w / 2;
    int y = pos.y() - h / 2;
    
    if (width % magnify > 0)
        ++w;
    
    if (height % magnify > 0)
        ++h;
    
    return(QRect(x, y, w, h));
}

QImage
ColorpickerPrivate::grabBuffer(QRect rect)
{
    int x = rect.x();
    int y = rect.y();
    int w = rect.width();
    int h = rect.height();
    
    WId id = 0;
    if (mode == Mode::Pick) {
        id = picker->winId();
    }
    if (mode == Mode::Drag) {
        id = dragger->winId();
    }
    QImage buffer;
    const QBrush blackBrush = QBrush(Qt::black);
    QScreen* screen = QGuiApplication::screenAt(rect.center());
    buffer = mac::grabImage(x, y, w, h, id);
   
    QRegion geom(x, y, w, h);
    QRect screenRect;
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
    return buffer;
}

bool
ColorpickerPrivate::underMouse(QWidget* widget) {
    QPoint pos = widget->mapFromGlobal(QCursor::pos());
    return widget->rect().contains(pos);
}

void
ColorpickerPrivate::update()
{
    if (!active)
        return;
    
    QRect grab = grabRect(cursor);
    QImage buffer = grabBuffer(grab);
    QScreen* screen = QGuiApplication::screenAt(cursor);
    qreal dpr = buffer.devicePixelRatio();
    
    // paint with device pixel ratio and apply
    // transforms and fill in user space
    QColor color;
    QRect rect(
       (grab.width() - aperture) / 2,
       (grab.height() - aperture) / 2,
       aperture, aperture
    );
    int colorR=0, colorG=0, colorB=0;
    for(int cx = rect.left(); cx <= rect.right(); cx++)
    {
        for(int cy = rect.top(); cy <= rect.bottom(); cy++)
        {
            QColor pixel = buffer.pixel(cx * dpr, cy * dpr);
            colorR += pixel.red();
            colorG += pixel.green();
            colorB += pixel.blue();
        }
    }
    int size = rect.width() * rect.height();
    color = QColor(colorR / size, colorG / size, colorB / size);
    
    // icc profile
    ICCTransform* transform = ICCTransform::instance();
    QString iccCurrentProfile = iccProfile;
    if (!iccCurrentProfile.length()) {
        iccCurrentProfile = iccDisplayProfile;
    }
    if (iccCurrentProfile != iccDisplayProfile) {
        color = transform->transformTo(color.rgb(), iccDisplayProfile, iccCurrentProfile);
        buffer = transform->transformTo(buffer, iccDisplayProfile, iccCurrentProfile);
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
          iccCurrentProfile,
          iccDisplayProfile,
        };
    }
    view();
    widget();
}

void
ColorpickerPrivate::view()
{
    qreal dpr = state.image.devicePixelRatio();
    const QBrush blackBrush = QBrush(Qt::black);
    QColor color;
    QImage image;
    // icc profile
    ICCTransform* transform = ICCTransform::instance();
    {
        if (state.iccProfile != iccWindowProfile) {
            color = transform->transformTo(state.color.rgb(), state.iccProfile, iccWindowProfile);
            image = transform->transformTo(state.image, state.iccProfile, iccWindowProfile);
        }
        else {
            color = state.color;
            image = state.image;
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
        p.drawImage(0, 0, image);
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
    ui->view->setPixmap(pixmap);
}

void
ColorpickerPrivate::widget()
{
    // display
    {
        ui->display->setText(QString("Display #%1").arg(state.displayNumber));
        QFontMetrics metrics(ui->iccProfile->font());
        QString text = metrics.elidedText(QFileInfo(state.iccDisplayProfile).baseName(), Qt::ElideRight, ui->iccProfile->width());
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
        if (display == Display::Hsv) {
            ui->display1Label->setText("H");
            ui->display2Label->setText("S");
            ui->display3Label->setText("V");
            ui->h->setText(QString("%1").arg(formatHsv(state.color, HsvChannel::H)));
            ui->s->setText(QString("%1").arg(formatHsv(state.color, HsvChannel::S)));
            ui->v->setText(QString("%1").arg(formatHsv(state.color, HsvChannel::V)));
        } else {
            ui->display1Label->setText("H");
            ui->display2Label->setText("S");
            ui->display3Label->setText("L");
            ui->h->setText(QString("%1").arg(formatHsl(state.color, HslChannel::HslH)));
            ui->s->setText(QString("%1").arg(formatHsl(state.color, HslChannel::HslS)));
            ui->v->setText(QString("%1").arg(formatHsl(state.color, HslChannel::HslL)));
            
        }
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
            if (dragcolors.count() > 0) {
                QString iccCurrentProfile = iccProfile;
                if (!iccCurrentProfile.length()) {
                    iccCurrentProfile = iccDisplayProfile;
                }
                for(QColor color : dragcolors) {
                    colors.push_back(QPair<QColor,QString>(color.rgb(), QFileInfo(iccCurrentProfile).baseName()));
                }
            }
            else
            {
                colors.push_back(QPair<QColor,QString>(state.color, QFileInfo(state.iccProfile).baseName()));
            }
            // push current state, use as selected
            ui->colorWheel->setColors(colors, true);
        }
        else
        {
            // restore colors, skip selected
            ui->colorWheel->setColors(colors, false);
        }
    }
    // picker
    if (mode == Mode::Pick)
    {
        QColor color = state.color;
        
        // icc profile
        ICCTransform* transform = ICCTransform::instance();
        if (state.iccProfile != iccWindowProfile) {
            color = transform->transformTo(color.rgb(), state.iccProfile, iccDisplayProfile); // use the current display for picking
        }
        // threshold for border color contrast
        if (color.valueF() < 0.2f) {
            picker->setBorderColor(Qt::white);
        } else {
            picker->setBorderColor(Qt::black);
        }
        picker->setColor(color);
        picker->update(cursor);
    }
    // drag
    if (mode == Mode::Drag)
    {
        dragger->update(cursor);
    }
}

void
ColorpickerPrivate::profile()
{
    iccWindowProfile = mac::grabIccProfileUrl(window->winId());
    stylesheet();
}

void
ColorpickerPrivate::blank()
{
    qreal dpr = window->devicePixelRatio();
    const QBrush blackBrush = QBrush(Qt::black);
    // pixmap
    QPixmap pixmap(width * dpr, height * dpr);
    pixmap.setDevicePixelRatio(dpr);
    {
        QPainter p(&pixmap);
        p.fillRect(QRect(0, 0, width, height), blackBrush);
        p.end();
    }
    ui->view->setPixmap(pixmap);
    QColor color = Qt::black;
    // rgb
    {
        ui->r->setText(QString("%1").arg(formatRgb(color, RgbChannel::R)));
        ui->g->setText(QString("%1").arg(formatRgb(color, RgbChannel::G)));
        ui->b->setText(QString("%1").arg(formatRgb(color, RgbChannel::B)));
    }
    // hsv
    {
        ui->h->setText(QString("%1").arg(formatHsv(color, HsvChannel::H)));
        ui->s->setText(QString("%1").arg(formatHsv(color, HsvChannel::S)));
        ui->v->setText(QString("%1").arg(formatHsv(color, HsvChannel::V)));
    }
    ui->mouseLocation->setText(QString("(%1, %2)").arg(0).arg(0));
    ui->colorWheel->setColors(asColors());
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
    if (event->type() == QEvent::ScreenChangeInternal) {
        profile();
        if (active) {
            view();
            widget();
        }
    }
    if (event->type() ==
        QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Escape) {
            if (active) {
                deactivate();
            }
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
    if (active) {
        if (event->type() == QEvent::QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            if (mouseEvent->button() == Qt::RightButton) {
                deactivate();
            }
        }
    }
    else {
        if (event->type() == QEvent::QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            if (mouseEvent->button() == Qt::LeftButton) {
                
                Colorwheel* colorWheel = ui->colorWheel;
                if (underMouse(colorWheel)) {
                    selected = colorWheel->mapToSelected(
                        colorWheel->mapFrom(window, mouseEvent->pos())
                    );
                    if (selected >= 0) {
                        colorWheel->setSelected(selected);
                        state = states[selected]; // restore state
                        view();
                        widget();
                    }
                }
            }
        }
        if (event->type() == QEvent::QEvent::MouseMove) {
            QMouseEvent* mouseEvent = (QMouseEvent*)event;
            if (mouseEvent->buttons() & Qt::LeftButton) {
                Colorwheel* colorWheel = ui->colorWheel;
                if (underMouse(colorWheel)) {
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
    if (selected >= 0) {

        ui->colorWheel->setSelected(selected);
        state = states[selected]; // restore state
        view();
        widget();
    }
    else
    {
        states.clear();
        selected = -1;
        blank();
    }
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
    mode = Mode::Pick;
    selected = ui->colorWheel->selected();
    states.push_back(state);
    update();
}

void
ColorpickerPrivate::drag()
{
    mode = Mode::Drag;
    QRect rect = dragger->dragRect();
    QImage image = mac::grabImage(rect.x(), rect.y(), rect.width(), rect.height(), dragger->winId());
    image = image.convertToFormat(QImage::Format_RGB888); // opencv need 24-bit RGB only
    qreal dpr = image.devicePixelRatio();
    dragcolors.clear();
    dragpositions.clear();
    if (rect.width() > 1 && rect.height() > 1)
    {
        int width = image.width();
        int height = image.height();
        int colornum = 6;
        {
            cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
            cv::Mat matrix = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
            matrix = asFloat32(asBGR(matrix));
            
            // prepare the image data for k-means clustering
            // the image is reshaped into a 2D matrix where each row represents a pixel and each column represents a color channel.
            // this serialization flattens the image into a single row for processing with k-means.
            cv::Mat serialized = matrix.reshape(1, matrix.total());
            serialized.convertTo(serialized, CV_32F);

            // perform k-Means clustering
            // k-means is applied to find 'k' clusters within the color space of the image.
            // 'k' is predefined, aiming for a broad initial capture of the image's color diversity.
            int k = 20;
            std::vector<int> labels;
            cv::Mat centers;
            cv::kmeans(serialized, k, labels, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 10, 1.0), 3, cv::KMEANS_PP_CENTERS, centers);

            // diversity selection logic
            // calculates pairwise distances between all cluster centers to identify similar colors.
            std::vector<std::vector<double>> distances(centers.rows, std::vector<double>(centers.rows, 0));
            for (int i = 0; i < centers.rows; ++i) {
                for (int j = i + 1; j < centers.rows; ++j) {
                    distances[i][j] = distances[j][i] = cv::norm(centers.row(i) - centers.row(j));
                }
            }
            
            // select a diverse set of colors.
            // aim to select 'tool.colors' that are as distinct as possible based on their pairwise distances.
            std::set<int> selectedindices;
            while (selectedindices.size() < colornum)
            {
                double maxmindistance = 0;
                int candidateindex = -1;
                for (int i = 0; i < centers.rows; ++i)
                    {
                    if (selectedindices.find(i) != selectedindices.end()) continue;
                    double minDistance = std::numeric_limits<double>::max();
                    for (int j : selectedindices)
                    {
                        minDistance = std::min(minDistance, distances[i][j]);
                    }
                    if (minDistance > maxmindistance)
                    {
                        maxmindistance = minDistance;
                        candidateindex = i;
                    }
                }
                if (candidateindex != -1) {
                    selectedindices.insert(candidateindex);
                } else {
                    break;
                }
            }
            
            // create a matrix for the selected diverse centers.
            cv::Mat diversecenters(selectedindices.size(), centers.cols, centers.type());
            int idx = 0;
            for (int selectedIndex : selectedindices) {
                centers.row(selectedIndex).copyTo(diversecenters.row(idx++));
            }

            // reassign each pixel in the image to the color of the closest diverse center.
            // this step alters the original serialized image data to reflect the reduced color palette.
            for (int i = 0; i < labels.size(); ++i) {
                int clusterIndex = std::distance(selectedindices.begin(), selectedindices.find(labels[i]));
                for (int j = 0; j < 3; ++j) { // assuming 3 channels
                    serialized.at<float>(i * 3 + j) = diversecenters.at<float>(clusterIndex, j);
                }
            }
                
            const int seed = 101010; // ultimate question of life in binary
            std::mt19937 gen(seed);
            
            // convert to hue and get pixel (x, y) coordinates
            std::unordered_map<int, std::vector<int>> indicesmap;
            for (int i = 0; i < labels.size(); ++i) {
                indicesmap[labels[i]].push_back(i);
            }
            
            for (int i = 0; i < diversecenters.rows; ++i) {
                cv::Vec3f center = diversecenters.at<cv::Vec3f>(i);
                auto it = std::next(selectedindices.begin(), i);
                if (it != selectedindices.end()) {
                    int originallabel = *it;
                    std::vector<int> &indices = indicesmap[originallabel];

                    // randomly select an index from the vector for this label
                    std::uniform_int_distribution<> dis(0, indices.size() - 1);
                    int index = indices[dis(gen)];
                    QPoint position = QPoint(index % width, index / width) / dpr;
                    // push
                    {
                        QColor color = asColor(center);
                        // icc profile
                        ICCTransform* transform = ICCTransform::instance();
                        QString iccCurrentProfile = iccProfile;
                        if (!iccCurrentProfile.length()) {
                            iccCurrentProfile = iccDisplayProfile;
                        }
                        if (iccCurrentProfile != iccDisplayProfile) {
                            color = transform->transformTo(color.rgb(), iccDisplayProfile, iccCurrentProfile);
                        }
                        dragcolors.push_back(color);
                        dragpositions.push_back(rect.topLeft() + position);
                    }
                }
            }
        }
    }
    update();
}

void
ColorpickerPrivate::pickClosed()
{
    mode = Mode::None;
    deactivate();
}

void
ColorpickerPrivate::dragClosed()
{
    if (dragcolors.size()) {
        for (int i = 0; i < dragcolors.size(); ++i) {
            QPoint pos = dragpositions.at(i);
            QRect grab = grabRect(pos);
            QImage buffer = grabBuffer(grab);
            QScreen* screen = QGuiApplication::screenAt(pos);

            // paint with device pixel ratio and apply
            // transforms and fill in user space
            QColor color = dragcolors.at(i);
            QRect rect(
               (grab.width() - aperture) / 2,
               (grab.height() - aperture) / 2,
               aperture, aperture
            );
            // colors are already converted in drag()
            QString iccCurrentProfile = iccProfile;
            if (!iccCurrentProfile.length()) {
                iccCurrentProfile = iccDisplayProfile;
            }
            // state
            State drag = State{
              color,
              rect,
              magnify,
              buffer,
              pos,
              screen->geometry().topLeft(),
              displayNumber,
              iccCurrentProfile,
              iccDisplayProfile,
            };
            states.push_back(drag);
        }
        dragcolors.clear();
        dragpositions.clear();
        selected = states.count() - 1;
        view();
        widget();
    }
    mode = Mode::None;
    deactivate();
}

void
ColorpickerPrivate::togglePick()
{
    if (mode == Mode::Pick)
    {
        mode = Mode::None;
        deactivate();
        {
            picker->hide();
        }
    }
    else
    {
        mode = Mode::Pick;
        activate();
        {
            // threshold for border color contrast
            if (state.color.valueF() < 0.2f) {
                picker->setBorderColor(Qt::white);
            } else {
                picker->setBorderColor(Qt::black);
            }
            picker->setColor(state.color);
            picker->update(cursor);
            picker->show();
        }
    }
}

void
ColorpickerPrivate::toggleDrag()
{
    if (dragger->isVisible())
    {
        mode = Mode::None;
        deactivate();
        {
            dragger->hide();
        }
    }
    else
    {
        mode = Mode::Drag;
        activate();
        {
            dragger->setFocus();
            dragger->update(cursor);
            dragger->show();
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
        "%, " +
        QString("%1").arg(formatHsv(state.color, HsvChannel::V)) +
        "%");
}

void
ColorpickerPrivate::copyHSL()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(
        QString("%1").arg(formatHsl(state.color, HslChannel::HslH)) +
        ", " +
        QString("%1").arg(formatHsl(state.color, HslChannel::HslS)) +
        ", " +
        QString("%1").arg(formatHsl(state.color, HslChannel::HslL)));
}

void
ColorpickerPrivate::copyHEX()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(state.color.name());
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
ColorpickerPrivate::as8bitValues()
{
    format = Format::Int8bit;
    widget();
}

void
ColorpickerPrivate::as10bitValues()
{
    format = Format::Int10bit;
    widget();
}

void
ColorpickerPrivate::asFloatValues()
{
    format = Format::Float;
    widget();
}

void
ColorpickerPrivate::asHexValues()
{
    format = Format::Hex;
    widget();
}

void
ColorpickerPrivate::asHSVValues()
{
    display = Display::Hsv;
    widget();
}

void
ColorpickerPrivate::asHSLValues()
{
    display = Display::Hsl;
    widget();
}

void
ColorpickerPrivate::asPercentageValues()
{
    format = Format::Percentage;
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
    iccProfile = ui->iccColorProfile->itemData(index, Qt::UserRole).value<QString>();
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
    selected = -1;
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
        arg(datetime.toString("hh:mm:ss"));
    
    QString filename =
        QString("%1/Colorpicker %2.pdf").
        arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).
        arg(datestamp);
    
    // icc profile
    ICCTransform* transform = ICCTransform::instance();
    QDir resources(QApplication::applicationDirPath() + "/../Resources");
    QString iccPDFProfile = resources.filePath("sRGB2014.icc"); // build-in Qt profile for PDF
    
    // document
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
            QString format = "png";
            
            // icc profile
            QImage image = transform->transformTo(widget.toImage(), iccWindowProfile, iccPDFProfile);
            QTextImageFormat imageformat;
            imageformat.setWidth(ui->colorWheel->width()/2);
            imageformat.setHeight(ui->colorWheel->height()/2);
            imageformat.setName(QString("data:image/%1;base64,%2")
                .arg(format)
                .arg(asBase64(image, format)));
            
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
                
                // icc profile
                QColor color = transform->transformTo(state.color.rgb(), state.iccProfile, iccPDFProfile);
                QImage image = transform->transformTo(state.image, state.iccProfile, iccPDFProfile);
                qreal dpr = state.image.devicePixelRatio();
                const QBrush blackBrush = QBrush(Qt::black);
                // png
                QString format = "png";
                QImage png(image.width(), image.height(), QImage::Format_RGB32); // png format
                png.setDevicePixelRatio(dpr);
                {
                    QPainter p(&png);
                    p.save();
                    p.scale(state.magnify, state.magnify);
                    p.fillRect(QRect(0, 0, image.width(), image.height()), blackBrush);
                    p.drawImage(0, 0, image);
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
                QTextImageFormat imageformat;
                imageformat.setWidth(64);
                imageformat.setHeight(64);
                imageformat.setName(QString("data:image/%1;base64,%2")
                    .arg(format)
                    .arg(asBase64(png, format)));
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
                       
float
ColorpickerPrivate::channelHsl(QColor color, HslChannel channel)
{
   switch (channel)
   {
       case HslChannel::HslH:
       {
           return qMax<float>(0, color.hslHueF());
       }
       break;
       case HslChannel::HslS:
       {
           return color.hslSaturationF();
       }
       break;
       case HslChannel::HslL:
       {
           return color.lightnessF();
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
ColorpickerPrivate::formatHsl(QColor hsl, HslChannel channel)
{
   if (channel == HslChannel::HslH)
   {
       return QString("%1°").arg(asDegree(channelHsl(hsl, channel) * 360));
   }
   else
   {
       return QString("%1%").arg(asPercentage(channelHsl(hsl, channel)));
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

cv::Mat
ColorpickerPrivate::asBGR(const cv::Mat& mat)
{
    cv::Mat matBGR;
    cv::cvtColor(mat, matBGR, cv::COLOR_RGB2BGR);
    return matBGR;
}

cv::Mat
ColorpickerPrivate::asFloat32(const cv::Mat& mat)
{
    cv::Mat mat32F;
    mat.convertTo(mat32F, CV_32FC3, 1.0 / 255.0);
    return mat32F;
}

QColor
ColorpickerPrivate::asColor(const cv::Vec3f& vec)
{
    int r = std::min(std::max(0.0f, vec[2]), 1.0f) * 255;
    int g = std::min(std::max(0.0f, vec[1]), 1.0f) * 255;
    int b = std::min(std::max(0.0f, vec[0]), 1.0f) * 255;

    return QColor(r, g, b);
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
        p->iccDisplayProfile = event.iccProfile;
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
