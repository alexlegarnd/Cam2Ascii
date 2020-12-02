#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cameras_availables = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras_availables)
    {
        ui->devicesBox->addItem(cameraInfo.description());
    }
    ui->devicesBox->setEditText(QCameraInfo::defaultCamera().description());
    revert();
    init(QCameraInfo::defaultCamera());
    QObject::connect(ui->startAndStopButton, &QPushButton::clicked, this, &MainWindow::start_or_stop);
    QObject::connect(ui->devicesBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::config_changed);
    QObject::connect(ui->sourceWidth, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::config_changed);
    QObject::connect(ui->sourceHeight, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::config_changed);
    QObject::connect(ui->asciiWidth, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::config_changed);
    QObject::connect(ui->asciiHeight, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::config_changed);
    QObject::connect(ui->charEdit, &QLineEdit::textEdited, this, &MainWindow::config_changed);
    QObject::connect(ui->disableProcess, &QCheckBox::clicked, this, &MainWindow::config_changed);
    QObject::connect(ui->applyButton, &QPushButton::clicked, this, &MainWindow::apply);
    QObject::connect(ui->revertButton, &QPushButton::clicked, this, &MainWindow::revert);
    QObject::connect(this, &MainWindow::image_processed, [&, this] (QPixmap img) {
        img = img.scaled(ascii_width, ascii_height);
        ui->screen->setPixmap(img);
        fps += 1;
    });
    QObject::connect(this, &MainWindow::tick_framerate, [&, this] () {
        ui->framerate->display(fps);
        fps = 0;
    });
}

void MainWindow::init(QCameraInfo cam)
{
    camera = new QCamera(cam);
    image_capture = new QCameraImageCapture(camera);
    image_capture->setCaptureDestination(QCameraImageCapture::CaptureDestination::CaptureToBuffer);
    camera->setCaptureMode(QCamera::CaptureStillImage);
    camera->start();
    QObject::connect(image_capture, &QCameraImageCapture::imageCaptured, [&, this] (int id, QImage img) {
        img = img.scaled(source_width, source_height);
        if (!disable_process) {
            convert(img);
        } else {
            ui->screen->setPixmap(QPixmap::fromImage(img));
            fps += 1;
        }
    });
}

MainWindow::~MainWindow()
{
    delete_all();
    delete ui;
}

void MainWindow::delete_all() {
    if (thread_running) {
        start_or_stop();
    }
    camera->unlock();
    camera->stop();
    delete camera;
    delete image_capture;
}

void MainWindow::convert(QImage in) {
    int length = chars.length();
    QString res = "";
    for (int y = 0; y < in.height(); y+=4) {
        for (int x = 1; x < in.width(); x+=2) {
            QColor pix1(in.pixel(x, y));
            int avg1 = (pix1.red() + pix1.green() + pix1.blue()) / 3;
            QColor pix2(in.pixel(x-1, y));
            int avg2 = (pix2.red() + pix2.green() + pix2.blue()) / 3;
            double avg = floor(avg1 + avg2 / 2);
            int char_index = floor(((256.0 - avg) / 256.0) * (length - 1));
            if (char_index >= length) char_index = length - 1;
            if (char_index < 0) char_index = 0;
            res += chars[char_index];
        }
        res += '\n';
    }
    QBitmap bitmap(1920,1080);
    bitmap.fill(Qt::white);
    QPainter painter(&bitmap);
    QFont serifFont("Consolas", 12);
    painter.setFont(serifFont);
    painter.setPen(Qt::black);
    const QRect rectangle = QRect(0, 0, 1920, 1080);
    QRect boundingRect;
    painter.drawText(rectangle, 0, res, &boundingRect);
    painter.save();
    QPixmap pix(bitmap);
    emit image_processed(pix);
}

void MainWindow::start_or_stop() {
    if (thread_running) {
        thread_running = false;
        capture_thread->wait();
        framerate_count->wait();
        delete capture_thread;
        delete framerate_count;
        ui->startAndStopButton->setText("Start");
    } else {
        ui->startAndStopButton->setText("Stop");
        thread_running = true;
        capture_thread = QThread::create([&, this]() {
            while(thread_running) {
                image_capture->capture();
                QThread::msleep(refresh_timer);
            }
        });
        framerate_count = QThread::create([&, this]() {
            while(thread_running) {
                QThread::sleep(1);
                emit tick_framerate();
            }
        });
        capture_thread->start();
        framerate_count->start();
    }

}

void MainWindow::apply() {
    ui->applyButton->setEnabled(false);
    ui->revertButton->setEnabled(false);
    delete_all();
    QCameraInfo cam = cameras_availables[ui->devicesBox->currentIndex()];
    init(cam);
    source_width = ui->sourceWidth->value();
    source_height = ui->sourceHeight->value();
    ascii_width = ui->asciiWidth->value();
    ascii_height = ui->asciiHeight->value();
    chars = ui->charEdit->text();
    disable_process = ui->disableProcess->checkState();
    refresh_timer = ui->refreshTimer->value();
}

void MainWindow::revert() {
    ui->sourceWidth->setValue(source_width);
    ui->sourceHeight->setValue(source_height);
    ui->asciiWidth->setValue(ascii_width);
    ui->asciiHeight->setValue(ascii_height);
    ui->charEdit->setText(chars);
    ui->refreshTimer->setValue(refresh_timer);
    ui->applyButton->setEnabled(false);
    ui->revertButton->setEnabled(false);
}

void MainWindow::config_changed() {
    ui->applyButton->setEnabled(true);
    ui->revertButton->setEnabled(true);
}
