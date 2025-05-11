/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WARPKATEPREFERENCESDIALOG_H
#define WARPKATEPREFERENCESDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSlider>

class QAbstractButton;

/**
 * Preferences dialog for WarpKate plugin
 */
class WarpKatePreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit WarpKatePreferencesDialog(QWidget *parent = nullptr);

    /**
     * Destructor
     */
    ~WarpKatePreferencesDialog();

private Q_SLOTS:
    /**
     * Apply settings when requested
     */
    void apply();

    /**
     * Reset settings when requested
     */
    void reset();

    /**
     * Reset to default settings
     */
    void defaults();

    /**
     * Browse for Obsidian vault path
     */
    void browseObsidianVault();

    /**
     * Toggle AI assistant name field
     */
    void onCustomAssistantNameToggled(bool enabled);

    /**
     * Toggle response style controls
     */
    void onCustomResponseStyleToggled(bool enabled);

private:
    /**
     * Load settings from config
     */
    void loadSettings();

    /**
     * Save settings to config
     */
    void saveSettings();

    /**
     * Create the UI components
     */
    void setupUI();

private:
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;
    bool m_changed;

    // Obsidian Integration
    QLineEdit *m_obsidianVaultPathEdit;
    QPushButton *m_browseFolderButton;
    QCheckBox *m_autoSaveToObsidianCheck;
    QLineEdit *m_defaultFilenamePatternEdit;

    // AI Assistant Personalization
    QCheckBox *m_customAssistantNameCheck;
    QLineEdit *m_assistantNameEdit;
    QLineEdit *m_userNameEdit;
    QComboBox *m_responseStyleCombo;
    QCheckBox *m_customResponseStyleCheck;
    QSlider *m_responseDetailSlider;
    QSlider *m_responseCreativitySlider;
};

#endif // WARPKATEPREFERENCESDIALOG_H
