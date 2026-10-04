#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACOMBOBOXVIEW_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACOMBOBOXVIEW_H_

#include <QListView>

class ElaComboBoxView : public QListView
{
    Q_OBJECT
public:
    explicit ElaComboBoxView(QWidget* parent = nullptr);
    ~ElaComboBoxView();
Q_SIGNALS:
    Q_SIGNAL void itemPressed(const QModelIndex& index);

protected:
    void mousePressEvent(QMouseEvent* event) override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_DEVELOPERCOMPONENTS_ELACOMBOBOXVIEW_H_

