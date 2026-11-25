#ifndef STYLEGALLERYWIDGET_H
#define STYLEGALLERYWIDGET_H

#include <QWidget>
#include <QList>

class QButtonGroup;
class QToolButton;

class StyleGalleryWidget : public QWidget
{
    Q_OBJECT

public:
    enum class StylePreset {
        Normal,
        NoSpacing,
        Heading1,
        Heading2,
        Heading3,
        Heading4
    };
    Q_ENUM(StylePreset)

    explicit StyleGalleryWidget(QWidget *parent = nullptr);
    void retranslateUi();

signals:
    void stylePresetSelected(StyleGalleryWidget::StylePreset preset);

private:
    void buildCards();
    QToolButton *createStyleCard(StylePreset preset, const QString &styleSheetSnippet);

    QButtonGroup *m_buttonGroup;
    QList<QToolButton *> m_cards;
};

#endif // STYLEGALLERYWIDGET_H
