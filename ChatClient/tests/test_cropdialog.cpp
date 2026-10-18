#include <QtTest>

#include "cropdialog.h"

// The crop view keeps a square selection in view coordinates and the dialog maps it back to
// source pixels. A wrong scale here silently cuts the avatar from the wrong place, so the
// pure helpers are covered without driving the mouse.
class CropDialogHelpersTest : public QObject
{
    Q_OBJECT

private slots:
    void fitImageCentersAndPreservesAspect()
    {
        QCOMPARE(CropDialog::FitImage(QSize(1000, 500), QSize(400, 400)), QRect(0, 100, 400, 200));
        QCOMPARE(CropDialog::FitImage(QSize(500, 1000), QSize(400, 400)), QRect(100, 0, 200, 400));
        QCOMPARE(CropDialog::FitImage(QSize(400, 400), QSize(400, 400)), QRect(0, 0, 400, 400));
        QVERIFY(CropDialog::FitImage(QSize(), QSize(400, 400)).isEmpty());
        QVERIFY(CropDialog::FitImage(QSize(10, 10), QSize()).isEmpty());
    }

    void clampSelectionIsSquareAndStaysInside()
    {
        const QRect bounds(0, 0, 400, 300);
        QCOMPARE(CropDialog::ClampSelection(QRect(390, 290, 50, 20), bounds, 32),
                 QRect(350, 250, 50, 50));
        QCOMPARE(CropDialog::ClampSelection(QRect(10, 10, 5, 5), bounds, 32),
                 QRect(10, 10, 32, 32));
        QCOMPARE(CropDialog::ClampSelection(QRect(10, 10, 10, 10),
                                            QRect(0, 0, 20, 20), 48),
                 QRect(0, 0, 20, 20));
        QVERIFY(CropDialog::ClampSelection(QRect(0, 0, 10, 10), QRect(), 32).isEmpty());
    }

    void mapSelectionBackToSourcePixels()
    {
        // a 1000x500 image shown as 400x200, centered vertically in a 400x400 view
        const QRect image_rect(0, 100, 400, 200);
        QCOMPARE(CropDialog::MapSelectionToSource(QRect(40, 140, 200, 100), image_rect,
                                                  QSize(1000, 500)),
                 QRect(100, 100, 500, 250));
    }

    void mapSelectionIsClippedToTheSource()
    {
        const QRect image_rect(0, 100, 400, 200);
        QCOMPARE(CropDialog::MapSelectionToSource(QRect(-20, 80, 50, 50), image_rect,
                                                  QSize(1000, 500)),
                 QRect(0, 0, 75, 75));
    }

    void initialSelectionCropsASquare()
    {
        QPixmap source(1000, 1000);
        source.fill(Qt::red);
        CropDialog dialog(source);

        const QPixmap cropped = dialog.CroppedPixmap();
        QVERIFY(!cropped.isNull());
        QCOMPARE(cropped.width(), cropped.height());
    }
};

QObject* createCropDialogHelpersTest()
{
    return new CropDialogHelpersTest;
}

#include "test_cropdialog.moc"
