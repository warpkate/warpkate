/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "warpkatepreferencesdialog.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

WarpKatePreferencesDialog::WarpKatePreferencesDialog(QWidget *parent)
    : QDialog(parent)
    , m_changed(false)
{
    setWindowTitle(i18n("WarpKate Preferences"));
    setMinimumSize(550, 450);
    
    setupUI();
    loadSettings();
    
    // Connect button box signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    connect(m_buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
        if (m_buttonBox->standardButton(button) == QDialogButtonBox::Apply) {
            apply();
        } else if (m_buttonBox->standardButton(button) == QDialogButtonBox::Reset) {
            reset();
        } else if (m_buttonBox->standardButton(button) == QDialogButtonBox::RestoreDefaults) {
            defaults();
        }
    });
    
    // Accept also saves settings
    connect(this, &QDialog::accepted, this, &WarpKatePreferencesDialog::apply);
}

WarpKatePreferencesDialog::~WarpKatePreferencesDialog()
{
}

void WarpKatePreferencesDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    // Create Obsidian Integration tab
    QWidget *obsidianTab = new QWidget();
    QVBoxLayout *obsidianLayout = new QVBoxLayout(obsidianTab);
    
    QGroupBox *obsidianGroupBox = new QGroupBox(i18n("Obsidian Integration"));
    QFormLayout *obsidianForm = new QFormLayout(obsidianGroupBox);
    
    QLabel *vaultPathLabel = new QLabel(i18n("Obsidian Vault Path:"));
    QHBoxLayout *pathLayout = new QHBoxLayout();
    m_obsidianVaultPathEdit = new QLineEdit();
    m_browseFolderButton = new QPushButton(i18n("Browse..."));
    pathLayout->addWidget(m_obsidianVaultPathEdit);
    pathLayout->addWidget(m_browseFolderButton);
    
    obsidianForm->addRow(vaultPathLabel, pathLayout);
    
    m_autoSaveToObsidianCheck = new QCheckBox(i18n("Automatically suggest saving to Obsidian"));
    obsidianForm->addRow(QString(), m_autoSaveToObsidianCheck);
    
    QLabel *defaultFilenameLabel = new QLabel(i18n("Default Filename Pattern:"));
    m_defaultFilenamePatternEdit = new QLineEdit();
    m_defaultFilenamePatternEdit->setPlaceholderText(QStringLiteral("WarpKate-Chat-{date}"));
    obsidianForm->addRow(defaultFilenameLabel, m_defaultFilenamePatternEdit);
    
    obsidianLayout->addWidget(obsidianGroupBox);
    obsidianLayout->addStretch();
    
    // Create AI Assistant Personalization tab
    QWidget *assistantTab = new QWidget();
    QVBoxLayout *assistantLayout = new QVBoxLayout(assistantTab);
    
    QGroupBox *personalityGroupBox = new QGroupBox(i18n("AI Assistant Personality"));
    QFormLayout *personalityForm = new QFormLayout(personalityGroupBox);
    
    m_customAssistantNameCheck = new QCheckBox(i18n("Use custom assistant name"));
    personalityForm->addRow(QString(), m_customAssistantNameCheck);
    
    QLabel *assistantNameLabel = new QLabel(i18n("Assistant Name:"));
    m_assistantNameEdit = new QLineEdit();
    m_assistantNameEdit->setPlaceholderText(QStringLiteral("WarpKate"));
    personalityForm->addRow(assistantNameLabel, m_assistantNameEdit);
    
    QLabel *userNameLabel = new QLabel(i18n("Your Name:"));
    m_userNameEdit = new QLineEdit();
    personalityForm->addRow(userNameLabel, m_userNameEdit);
    
    assistantLayout->addWidget(personalityGroupBox);
    
    QGroupBox *responseGroupBox = new QGroupBox(i18n("Response Style"));
    QFormLayout *responseForm = new QFormLayout(responseGroupBox);
    
    QLabel *responseStyleLabel = new QLabel(i18n("Default Style:"));
    m_responseStyleCombo = new QComboBox();
    m_responseStyleCombo->addItem(i18n("Balanced"), QStringLiteral("balanced"));
    m_responseStyleCombo->addItem(i18n("Concise"), QStringLiteral("concise"));
    m_responseStyleCombo->addItem(i18n("Detailed"), QStringLiteral("detailed"));
    m_responseStyleCombo->addItem(i18n("Technical"), QStringLiteral("technical"));
    m_responseStyleCombo->addItem(i18n("Friendly"), QStringLiteral("friendly"));
    m_responseStyleCombo->addItem(i18n("Custom"), QStringLiteral("custom"));
    responseForm->addRow(responseStyleLabel, m_responseStyleCombo);
    
    m_customResponseStyleCheck = new QCheckBox(i18n("Customize response style"));
    responseForm->addRow(QString(), m_customResponseStyleCheck);
    
    QLabel *detailLevelLabel = new QLabel(i18n("Detail Level:"));
    m_responseDetailSlider = new QSlider(Qt::Horizontal);
    m_responseDetailSlider->setMinimum(1);
    m_responseDetailSlider->setMaximum(5);
    m_responseDetailSlider->setValue(3);
    m_responseDetailSlider->setTickPosition(QSlider::TicksBelow);
    m_responseDetailSlider->setTickInterval(1);
    responseForm->addRow(detailLevelLabel, m_responseDetailSlider);
    
    QLabel *creativityLabel = new QLabel(i18n("Creativity:"));
    m_responseCreativitySlider = new QSlider(Qt::Horizontal);
    m_responseCreativitySlider->setMinimum(1);
    m_responseCreativitySlider->setMaximum(5);
    m_responseCreativitySlider->setValue(3);
    m_responseCreativitySlider->setTickPosition(QSlider::TicksBelow);
    m_responseCreativitySlider->setTickInterval(1);
    responseForm->addRow(creativityLabel, m_responseCreativitySlider);
    
    assistantLayout->addWidget(responseGroupBox);
    assistantLayout->addStretch();
    
    // Add tabs to tab widget
    m_tabWidget->addTab(obsidianTab, i18n("Obsidian Integration"));
    m_tabWidget->addTab(assistantTab, i18n("AI Assistant"));
    
    // Create button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Reset);
    
    // Add widgets to main layout
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(m_buttonBox);
    
    // Connect signals
    connect(m_browseFolderButton, &QPushButton::clicked, this, &WarpKatePreferencesDialog::browseObsidianVault);
    connect(m_customAssistantNameCheck, &QCheckBox::toggled, this, &WarpKatePreferencesDialog::onCustomAssistantNameToggled);
    connect(m_customResponseStyleCheck, &QCheckBox::toggled, this, &WarpKatePreferencesDialog::onCustomResponseStyleToggled);
    
    // Connect signals to detect changes
    connect(m_obsidianVaultPathEdit, &QLineEdit::textChanged, this, [this]() { m_changed = true; });
    connect(m_autoSaveToObsidianCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    connect(m_defaultFilenamePatternEdit, &QLineEdit::textChanged, this, [this]() { m_changed = true; });
    connect(m_customAssistantNameCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    connect(m_assistantNameEdit, &QLineEdit::textChanged, this, [this]() { m_changed = true; });
    connect(m_userNameEdit, &QLineEdit::textChanged, this, [this]() { m_changed = true; });
    connect(m_responseStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { m_changed = true; });
    connect(m_customResponseStyleCheck, &QCheckBox::toggled, this, [this]() { m_changed = true; });
    connect(m_responseDetailSlider, &QSlider::valueChanged, this, [this]() { m_changed = true; });
    connect(m_responseCreativitySlider, &QSlider::valueChanged, this, [this]() { m_changed = true; });
}

void WarpKatePreferencesDialog::browseObsidianVault()
{
    QString startDir = m_obsidianVaultPathEdit->text();
    if (startDir.isEmpty()) {
        startDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    
    QString dir = QFileDialog::getExistingDirectory(this, i18n("Select Obsidian Vault Directory"),
                                                  startDir,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty()) {
        m_obsidianVaultPathEdit->setText(dir);
        m_changed = true;
    }
}

void WarpKatePreferencesDialog::onCustomAssistantNameToggled(bool enabled)
{
    m_assistantNameEdit->setEnabled(enabled);
}

void WarpKatePreferencesDialog::onCustomResponseStyleToggled(bool enabled)
{
    m_responseDetailSlider->setEnabled(enabled);
    m_responseCreativitySlider->setEnabled(enabled);
}

void WarpKatePreferencesDialog::apply()
{
    if (m_changed) {
        saveSettings();
        m_changed = false;
    }
}

void WarpKatePreferencesDialog::reset()
{
    loadSettings();
    m_changed = false;
}

void WarpKatePreferencesDialog::defaults()
{
    // Obsidian Integration
    m_obsidianVaultPathEdit->clear();
    m_autoSaveToObsidianCheck->setChecked(true);
    m_defaultFilenamePatternEdit->setText(QStringLiteral("WarpKate-Chat-{date}"));
    
    // AI Assistant Personalization
    m_customAssistantNameCheck->setChecked(false);
    m_assistantNameEdit->setText(QStringLiteral("WarpKate"));
    m_userNameEdit->clear();
    m_responseStyleCombo->setCurrentIndex(0); // Balanced
    m_customResponseStyleCheck->setChecked(false);
    m_responseDetailSlider->setValue(3);
    m_responseCreativitySlider->setValue(3);
    
    // Update dependent UI elements
    onCustomAssistantNameToggled(m_customAssistantNameCheck->isChecked());
    onCustomResponseStyleToggled(m_customResponseStyleCheck->isChecked());
    
    m_changed = true;
}

void WarpKatePreferencesDialog::loadSettings()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    
    // Obsidian Integration
    m_obsidianVaultPathEdit->setText(config.readEntry("ObsidianVaultPath", QString()));
    m_autoSaveToObsidianCheck->setChecked(config.readEntry("AutoSaveToObsidian", true));
    m_defaultFilenamePatternEdit->setText(config.readEntry("DefaultFilenamePattern", QStringLiteral("WarpKate-Chat-{date}")));
    
    // AI Assistant Personalization
    m_customAssistantNameCheck->setChecked(config.readEntry("UseCustomAssistantName", false));
    m_assistantNameEdit->setText(config.readEntry("AssistantName", QStringLiteral("WarpKate")));
    m_userNameEdit->setText(config.readEntry("UserName", QString()));
    m_responseStyleCombo->setCurrentIndex(config.readEntry("ResponseStyle", 0)); // Default to Balanced
    m_customResponseStyleCheck->setChecked(config.readEntry("UseCustomResponseStyle", false));
    m_responseDetailSlider->setValue(config.readEntry("ResponseDetailLevel", 3));
    m_responseCreativitySlider->setValue(config.readEntry("ResponseCreativity", 3));
    
    // Update dependent UI elements
    onCustomAssistantNameToggled(m_customAssistantNameCheck->isChecked());
    onCustomResponseStyleToggled(m_customResponseStyleCheck->isChecked());
    
    m_changed = false;
}

void WarpKatePreferencesDialog::saveSettings()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    
    // Obsidian Integration
    config.writeEntry("ObsidianVaultPath", m_obsidianVaultPathEdit->text());
    config.writeEntry("AutoSaveToObsidian", m_autoSaveToObsidianCheck->isChecked());
    config.writeEntry("DefaultFilenamePattern", m_defaultFilenamePatternEdit->text());
    
    // AI Assistant Personalization
    config.writeEntry("UseCustomAssistantName", m_customAssistantNameCheck->isChecked());
    config.writeEntry("AssistantName", m_assistantNameEdit->text());
    config.writeEntry("UserName", m_userNameEdit->text());
    config.writeEntry("ResponseStyle", m_responseStyleCombo->currentIndex());
    config.writeEntry("UseCustomResponseStyle", m_customResponseStyleCheck->isChecked());
    config.writeEntry("ResponseDetailLevel", m_responseDetailSlider->value());
    config.writeEntry("ResponseCreativity", m_responseCreativitySlider->value());
    
    // Sync changes to disk
    config.sync();
}
