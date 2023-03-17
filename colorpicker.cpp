// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorpicker.h"
#include "picker.h"
#include "lcms2.h"
#include "mac.h"

#include <QAction>
#include <QBuffer>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPrinter>
#include <QScreen>
#include <QTextDocument>
#include <QTextTable>
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
        void widget();
        void activate();
        void deactivate();
        bool eventFilter(QObject* object, QEvent* event);
    
    public Q_SLOTS:
        void toggleActive(bool checked);
        void pick();
        void closed();
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
        void pdf();
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
            QString displayInProfile;
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
        bool active;
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
, active(true)
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
    // actions
    ui->toggleActive->setDefaultAction(ui->active);
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
    connect(ui->active, SIGNAL(toggled(bool)), this, SLOT(toggleActive(bool)));
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
    connect(ui->pdf, SIGNAL(pressed()), this, SLOT(pdf()));
    connect(ui->about, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->openGithubReadme, SIGNAL(triggered()), this, SLOT(openGithubReadme()));
    connect(ui->openGithubIssues, SIGNAL(triggered()), this, SLOT(openGithubIssues()));
    connect(picker.get(), SIGNAL(triggered()), this, SLOT(pick()));
    connect(picker.get(), SIGNAL(closed()), this, SLOT(closed()));
    // pixmaps
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);
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
        color = QColor::fromRgb(lcms2::convertColor(color.rgb(), displayProfile, displayInProfile));
    }
    // state
    {
        state = State{
          color,
          pixmap,
          cursor,
          screen,
          displayNumber,
          displayProfile,
          displayInProfile
        };
    }
    widget();
}

void
ColorpickerPrivate::widget()
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
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Escape)
        {
            deactivate();
        }
        else if (keyEvent->key() == Qt::Key_Plus)
        {
            QSlider* slider = ui->aperture;
            {
                activate();
                slider->setSliderPosition(slider->value() + slider->singleStep());
            }
        }
        else if (keyEvent->key() == Qt::Key_Minus)
        {
            QSlider* slider = ui->aperture;
            {
                activate();
                slider->setSliderPosition(slider->value() - slider->singleStep());
            }
        }
    }
    if (!active)
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
                    widget();
                }
                else
                {
                    ui->widget->setSelected(-1);
                }
            }
        }
    }
    return false;
}

void
ColorpickerPrivate::toggleActive(bool checked)
{
    ui->widget->setColors(colors);
    active = checked;
}

void
ColorpickerPrivate::pick()
{
    colors.push_back(color);
    {
        update();
        states.push_back(state);
    }
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
    ui->widget->setColors(colors);
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
            QPixmap widget(ui->widget->size()*ui->widget->devicePixelRatio());
            widget.setDevicePixelRatio(ui->widget->devicePixelRatio());
            ui->widget->render(&widget);
            QImage image = widget.toImage();
            QString format = "png";
            
            QTextImageFormat imageformat;
            imageformat.setWidth(ui->widget->width()/2);
            imageformat.setHeight(ui->widget->height()/2);
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
        QTextTable* table = cursor.insertTable(states.count()+1, 5);
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
                QImage image = state.pixmap.toImage();
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
                cellcursor.insertHtml(QString("<small><b>In profile:</b> %1%2</small>").arg(QFileInfo(state.displayProfile).fileName()).arg("<br>"));
                cellcursor.insertHtml(QString("<small><b>Out profile:</b> %1%2</small>").arg(QFileInfo(state.displayInProfile).fileName()).arg("<br>"));
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
        printer.setPaperSize(QPrinter::A4);
        printer.setColorMode(QPrinter::Color);
        printer.setResolution(300);
        doc->print(&printer);
        QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
    }
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

bool
Colorpicker::active() const
{
    return p->active;
}

void
Colorpicker::displayEvent(DisplayEvent event)
{
    p->displayNumber = event.displayNumber;
    p->displayProfile = event.displayProfile;
    p->cursor = event.cursor;
    p->update();
}

void
Colorpicker::mouseEvent(MouseEvent event)
{
    p->cursor = event.cursor;
    p->update();
}

