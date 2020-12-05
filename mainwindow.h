#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QBuffer>
#include <QThread>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QMap>
#include <QBitmap>
#include <QPainter>
#include <QStaticText>
#include <QCameraInfo>
#include <QFontDialog>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void start_or_stop();
    void apply();
    void revert();
    void config_changed();
    void change_font();
    void update_font_label();

signals:
    void image_processed(QPixmap img);
    void tick_framerate();

private:
    Ui::MainWindow *ui;
    QCamera *camera;
    QCameraImageCapture *image_capture;

    QThread *capture_thread;
    QThread *framerate_count;
    QCameraInfo selected_camera;

    bool thread_running = false;
    QString chars = "@#P+;-. ";
    int source_width = 320;
    int source_height = 240;

    int ascii_width = 771;
    int ascii_height = 531;

    int bitmap_width = 771;
    int bitmap_height = 531;

    int refresh_timer = 25;
    bool disable_process = false;
    QFont canvasFont;
    QFont canvasFont_tmp;

    int fps = 0;

    QList<QCameraInfo> cameras_availables;

    void init(QCameraInfo cam);
    void convert(QImage in);
    void delete_all();
};
#endif // MAINWINDOW_H
