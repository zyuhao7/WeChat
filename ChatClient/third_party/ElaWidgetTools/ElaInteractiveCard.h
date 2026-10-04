#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELAINTERACTIVECARD_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELAINTERACTIVECARD_H_

#include <QPixmap>
#include <QPushButton>

#include "ElaWidgetToolsDef.h"
#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"

class ElaInteractiveCardPrivate;
class ELA_EXPORT ElaInteractiveCard : public QPushButton
{
    Q_OBJECT
    Q_Q_CREATE(ElaInteractiveCard)
    Q_PROPERTY_CREATE_Q_H(int, BorderRadius)
    Q_PROPERTY_REF_CREATE_Q_H(QString, Title);
    Q_PROPERTY_REF_CREATE_Q_H(QString, SubTitle);
    Q_PROPERTY_CREATE_Q_H(int, TitlePixelSize);
    Q_PROPERTY_CREATE_Q_H(int, SubTitlePixelSize);
    Q_PROPERTY_CREATE_Q_H(int, TitleSpacing);
    Q_PROPERTY_REF_CREATE_Q_H(QPixmap, CardPixmap);
    Q_PROPERTY_CREATE_Q_H(QSize, CardPixmapSize);
    Q_PROPERTY_CREATE_Q_H(int, CardPixmapBorderRadius)
    Q_PROPERTY_CREATE_Q_H(ElaCardPixType::PixMode, CardPixMode);

public:
    explicit ElaInteractiveCard(QWidget* parent = nullptr);
    ~ElaInteractiveCard();
    void setCardPixmapSize(int width, int height);

protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELAINTERACTIVECARD_H_
