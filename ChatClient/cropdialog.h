#ifndef CROPDIALOG_H
#define CROPDIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QRect>
#include <QSize>

class CropView;

// The avatar is round, so the selection is forced square. The view shows the source scaled
// to fit; the helpers below translate between view and source coordinates and are covered
// by the client tests.
class CropDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CropDialog(const QPixmap& source, QWidget* parent = nullptr);

    QPixmap CroppedPixmap() const;

    // The largest rect with the image's aspect ratio centered in the view.
    static QRect FitImage(const QSize& image, const QSize& area);
    // A square selection of at least min_side, shifted so it stays inside bounds.
    static QRect ClampSelection(const QRect& selection, const QRect& bounds, int min_side);
    static QRect MapSelectionToSource(const QRect& selection, const QRect& image_rect,
                                      const QSize& source);

private:
    QPixmap _source;
    CropView* _view;
};

#endif // CROPDIALOG_H
