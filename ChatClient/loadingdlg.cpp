#include "loadingdlg.h"
#include "ui_loadingdlg.h"
#include <QMovie>

LoadingDlg::LoadingDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoadingDlg)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog |Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground); // make the background transparent

    // get the screen size
    setFixedSize(parent->size()); // set the dialog to fullscreen size

    QMovie* movie = new QMovie(":/res/loading.gif"); // load the animation resource file
    ui->loading_lb->setMovie(movie);
    movie->start();
}

LoadingDlg::~LoadingDlg()
{
    delete ui;
}
