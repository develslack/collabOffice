#include "StyleGalleryWidget.h"

#include <QHBoxLayout>
#include <QButtonGroup>
#include <QToolButton>

StyleGalleryWidget::StyleGalleryWidget(QWidget *parent)
    : QWidget(parent)
    , m_buttonGroup(new QButtonGroup(this))
{
    m_buttonGroup->setExclusive(true);
    buildCards();
    retranslateUi();

    connect(m_buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this](int id) {
        emit stylePresetSelected(static_cast<StylePreset>(id));
    });
}

void StyleGalleryWidget::buildCards()
{
    QHBoxLayout *row = new QHBoxLayout(this);
    row->setContentsMargins(2, 2, 2, 2);
    row->setSpacing(4);

    m_cards.append(createStyleCard(StylePreset::Normal,
                                   QStringLiteral("font-size: 11px; color: #1E293B;")));
    m_cards.append(createStyleCard(StylePreset::NoSpacing,
                                   QStringLiteral("font-size: 11px; color: #1E293B;")));
    m_cards.append(createStyleCard(StylePreset::Heading1,
                                   QStringLiteral("font-size: 14px; font-weight: bold; color: #1D4ED8;")));
    m_cards.append(createStyleCard(StylePreset::Heading2,
                                   QStringLiteral("font-size: 12px; font-weight: bold; color: #2563EB;")));
    m_cards.append(createStyleCard(StylePreset::Heading3,
                                   QStringLiteral("font-size: 11px; font-weight: bold; color: #3B82F6;")));
    m_cards.append(createStyleCard(StylePreset::Heading4,
                                   QStringLiteral("font-size: 11px; font-style: italic; color: #2563EB;")));

    for (QToolButton *btn : qAsConst(m_cards)) {
        row->addWidget(btn);
    }

    if (!m_cards.isEmpty()) {
        m_cards.first()->setChecked(true);
    }
}

QToolButton *StyleGalleryWidget::createStyleCard(StylePreset preset, const QString &styleSheetSnippet)
{
    QToolButton *btn = new QToolButton(this);
    btn->setCheckable(true);
    btn->setFixedSize(78, 46);
    btn->setStyleSheet(QStringLiteral(
        "QToolButton { background-color: #FFFFFF; border: 1px solid #94A3B8; border-radius: 3px; padding: 2px; %1 }"
        "QToolButton:hover { border: 2px solid #3B82F6; }"
        "QToolButton:checked { border: 2px solid #2563EB; background-color: #EFF6FF; }")
                           .arg(styleSheetSnippet));

    m_buttonGroup->addButton(btn, static_cast<int>(preset));
    return btn;
}

void StyleGalleryWidget::retranslateUi()
{
    if (m_cards.size() < 6) {
        return;
    }
    m_cards[0]->setText(tr("Normal"));
    m_cards[1]->setText(tr("No Spacing"));
    m_cards[2]->setText(tr("Heading 1"));
    m_cards[3]->setText(tr("Heading 2"));
    m_cards[4]->setText(tr("Heading 3"));
    m_cards[5]->setText(tr("Heading 4"));
}
