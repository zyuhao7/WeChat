#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAIMAGECARD_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAIMAGECARD_H_

#include <QWidget>

#include "ElaPropertyMacro.h"
#include "ElaWidgetToolsExport.h"

class ElaImageCardPrivate;
class ELA_EXPORT ElaImageCard : public QWidget
{
    Q_OBJECT
    Q_Q_CREATE(ElaImageCard)
    Q_PROPERTY_REF_CREATE_Q_H(QImage, CardImage);
    Q_PROPERTY_CREATE_Q_H(int, BorderRadius)
    Q_PROPERTY_CREATE_Q_H(bool, IsPreserveAspectCrop)
public:
    explicit ElaImageCard(QWidget* parent = nullptr);
    ~ElaImageCard() override;

protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAIMAGECARD_H_
