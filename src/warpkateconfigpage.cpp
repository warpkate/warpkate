/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "warpkateconfigpage.h"
#include "ui_configwidget.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QFontDialog>
#include <QFontDatabase>
#include <QDebug>

WarpKateConfigPage::WarpKateConfigPage(QWidget *parent)
    : KTextEditor::ConfigPage(parent)
    , m_ui(new Ui::ConfigWidget)
    , m_changed(false)
{
    // Setup UI
    m_ui->setupUi(this);

    // Load settings from config
    loadSettings();

    // Connect signals to detect changes
    connect(m_ui->shellEdit, &QLineEdit::textChanged, this, [this]() { m_changed = true; });
    connect(m_ui->startupCmdEdit, &QLineEdit::textChanged, this, [this]() { m_changed = true; });
    connect(m_ui->autoshowCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    connect(m_ui->saveHistoryCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    connect(m_ui->historySizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { m_changed = true; });
    connect(m_ui->positionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { m_changed = true; });
    connect(m_ui->heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { m_changed = true; });
    
    // Appearance tab connections
    connect(m_ui->colorSchemeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { m_changed = true; });
    connect(m_ui->fontSelectButton, &QPushButton::clicked, this, &WarpKateConfigPage::onFontSelectClicked);
    connect(m_ui->transparencyCheck, &QCheckBox::toggled, this, &WarpKateConfigPage::onTransparencyToggled);
    connect(m_ui->transparencySlider, &QSlider::valueChanged, this, [this]() { m_changed = true; });
    connect(m_ui->blockStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { m_changed = true; });
    connect(m_ui->showTimestampsCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    connect(m_ui->syntaxHighlightCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    
    // AI tab connections
    connect(m_ui->enableAICheck, &QCheckBox::toggled, this, &WarpKateConfigPage::onAIToggled);
    connect(m_ui->aiModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { m_changed = true; });
    connect(m_ui->apiKeyEdit, &QLineEdit::textChanged, this, [this]() { m_changed = true; });
    connect(m_ui->contextAwarenessCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    connect(m_ui->privacyModeCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    connect(m_ui->autoSuggestCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    connect(m_ui->maxSuggestionsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { m_changed = true; });
    connect(m_ui->suggestionDelaySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { m_changed = true; });
    
    // Button box connections
    connect(m_ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
        if (m_ui->buttonBox->standardButton(button) == QDialogButtonBox::Apply) {
            apply();
        } else if (m_ui->buttonBox->standardButton(button) == QDialogButtonBox::Reset) {
            reset();
        } else if (m_ui->buttonBox->standardButton(button) == QDialogButtonBox::RestoreDefaults) {
            defaults();
        }
    });
    
    // Initial state for dependent widgets
    onTransparencyToggled(m_ui->transparencyCheck->isChecked());
    onAIToggled(m_ui->enableAICheck->isChecked());
}

WarpKateConfigPage::~WarpKateConfigPage()
{
    delete m_ui;
}

QString WarpKateConfigPage::name() const
{
    return i18n("WarpKate Terminal");
}

QString WarpKateConfigPage::fullName() const
{
    return i18n("WarpKate Terminal Configuration");
}

QIcon WarpKateConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("utilities-terminal"));
}

void WarpKateConfigPage::apply()
{
    if (m_changed) {
        saveSettings();
        m_changed = false;
        Q_EMIT changed();
    }
}

void WarpKateConfigPage::reset()
{
    loadSettings();
    m_changed = false;
}

void WarpKateConfigPage::defaults()
{
    // General tab
    m_ui->shellEdit->setText(QStringLiteral("/bin/bash"));
    m_ui->startupCmdEdit->clear();
    m_ui->autoshowCheck->setChecked(true);
    m_ui->saveHistoryCheck->setChecked(true);
    m_ui->historySizeSpinBox->setValue(1000);
    m_ui->positionCombo->setCurrentIndex(0); // Bottom
    m_ui->heightSpinBox->setValue(300);
    
    // Appearance tab
    m_ui->colorSchemeCombo->setCurrentIndex(0); // System Default
    m_terminalFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_ui->transparencyCheck->setChecked(false);
    m_ui->transparencySlider->setValue(20);
    m_ui->blockStyleCombo->setCurrentIndex(0); // Modern
    m_ui->showTimestampsCheck->setChecked(true);
    m_ui->syntaxHighlightCheck->setChecked(true);
    
    // AI tab
    m_ui->enableAICheck->setChecked(true);
    m_ui->aiModelCombo->setCurrentIndex(0); // Local (Fast)
    m_ui->apiKeyEdit->clear();
    m_ui->contextAwarenessCheck->setChecked(true);
    m_ui->privacyModeCheck->setChecked(false);
    m_ui->autoSuggestCheck->setChecked(true);
    m_ui->maxSuggestionsSpinBox->setValue(3);
    m_ui->suggestionDelaySpinBox->setValue(500);
    
    // Update dependent widgets
    onTransparencyToggled(m_ui->transparencyCheck->isChecked());
    onAIToggled(m_ui->enableAICheck->isChecked());
    
    m_changed = true;
}

void WarpKateConfigPage::onTransparencyToggled(bool enabled)
{
    m_ui->transparencyLabel->setEnabled(enabled);
    m_ui->transparencySlider->setEnabled(enabled);
    m_changed = true;
}

void WarpKateConfigPage::onFontSelectClicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_terminalFont, this, i18n("Select Terminal Font"));
    if (ok) {
        m_terminalFont = font;
        m_changed = true;
    }
}

void WarpKateConfigPage::onAIToggled(bool enabled)
{
    // Enable/disable AI-related widgets based on checkbox state
    m_ui->aiModelLabel->setEnabled(enabled);
    m_ui->aiModelCombo->setEnabled(enabled);
    m_ui->apiKeyLabel->setEnabled(enabled);
    m_ui->apiKeyEdit->setEnabled(enabled);
    m_ui->contextAwarenessCheck->setEnabled(enabled);
    m_ui->privacyModeCheck->setEnabled(enabled);
    m_ui->autoSuggestCheck->setEnabled(enabled);
    m_ui->maxSuggestionsLabel->setEnabled(enabled);
    m_ui->maxSuggestionsSpinBox->setEnabled(enabled);
    m_ui->suggestionDelayLabel->setEnabled(enabled);
    m_ui->suggestionDelaySpinBox->setEnabled(enabled);
    m_changed = true;
}

void WarpKateConfigPage::loadSettings()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    
    // General tab
    m_ui->shellEdit->setText(config.readEntry("Shell", QStringLiteral("/bin/bash")));
    m_ui->startupCmdEdit->setText(config.readEntry("StartupCommand", QString()));
    m_ui->autoshowCheck->setChecked(config.readEntry("AutoShow", true));
    m_ui->saveHistoryCheck->setChecked(config.readEntry("SaveHistory", true));
    m_ui->historySizeSpinBox->setValue(config.readEntry("HistorySize", 1000));
    m_ui->positionCombo->setCurrentIndex(config.readEntry("Position", 0));
    m_ui->heightSpinBox->setValue(config.readEntry("Height", 300));
    
    // Appearance tab
    m_ui->colorSchemeCombo->setCurrentIndex(config.readEntry("ColorScheme", 0));
    m_terminalFont = config.readEntry("Font", QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_ui->transparencyCheck->setChecked(config.readEntry("EnableTransparency", false));
    m_ui->transparencySlider->setValue(config.readEntry("TransparencyLevel", 20));
    m_ui->blockStyleCombo->setCurrentIndex(config.readEntry("BlockStyle", 0));
    m_ui->showTimestampsCheck->setChecked(config.readEntry("ShowTimestamps", true));
    m_ui->syntaxHighlightCheck->setChecked(config.readEntry("SyntaxHighlight", true));
    
    // AI tab
    m_ui->enableAICheck->setChecked(config.readEntry("EnableAI", true));
    m_ui->aiModelCombo->setCurrentIndex(config.readEntry("AIModel", 0));
    m_ui->apiKeyEdit->setText(config.readEntry("APIKey", QString()));
    m_ui->contextAwarenessCheck->setChecked(config.readEntry("ContextAwareness", true));
    m_ui->privacyModeCheck->setChecked(config.readEntry("PrivacyMode", false));
    m_ui->autoSuggestCheck->setChecked(config.readEntry("AutoSuggest", true));
    m_ui->maxSuggestionsSpinBox->setValue(config.readEntry("MaxSuggestions", 3));
    m_ui->suggestionDelaySpinBox->setValue(config.readEntry("SuggestionDelay", 500));
    
    // Update dependent widgets
    onTransparencyToggled(m_ui->transparencyCheck->isChecked());
    onAIToggled(m_ui->enableAICheck->isChecked());
    
    m_changed = false;
}

void WarpKateConfigPage::saveSettings()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    
    // General tab
    config.writeEntry("Shell", m_ui->shellEdit->text());
    config.writeEntry("StartupCommand", m_ui->startupCmdEdit->text());
    config.writeEntry("AutoShow", m_ui->autoshowCheck->isChecked());
    config.writeEntry("SaveHistory", m_ui->saveHistoryCheck->isChecked());
    config.writeEntry("HistorySize", m_ui->historySizeSpinBox->value());
    config.writeEntry("Position", m_ui->positionCombo->currentIndex());
    config.writeEntry("Height", m_ui->heightSpinBox->value());
    
    // Appearance tab
    config.writeEntry("ColorScheme", m_ui->colorSchemeCombo->currentIndex());
    config.writeEntry("Font", m_terminalFont);
    config.writeEntry("EnableTransparency", m_ui->transparencyCheck->isChecked());
    config.writeEntry("TransparencyLevel", m_ui->transparencySlider->value());
    config.writeEntry("BlockStyle", m_ui->blockStyleCombo->currentIndex());
    config.writeEntry("ShowTimestamps", m_ui->showTimestampsCheck->isChecked());
    config.writeEntry("SyntaxHighlight", m_ui->syntaxHighlightCheck->isChecked());
    
    // AI tab
    config.writeEntry("EnableAI", m_ui->enableAICheck->isChecked());
    config.writeEntry("AIModel", m_ui->aiModelCombo->currentIndex());
    config.writeEntry("APIKey", m_ui->apiKeyEdit->text());
    config.writeEntry("ContextAwareness", m_ui->contextAwarenessCheck->isChecked());
    config.writeEntry("PrivacyMode", m_ui->privacyModeCheck->isChecked());
    config.writeEntry("AutoSuggest", m_ui->autoSuggestCheck->isChecked());
    config.writeEntry("MaxSuggestions", m_ui->maxSuggestionsSpinBox->value());
    config.writeEntry("SuggestionDelay", m_ui->suggestionDelaySpinBox->value());
    
    // Sync changes to disk
    config.sync();
}
