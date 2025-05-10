/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WARPKATECONFIGPAGE_H
#define WARPKATECONFIGPAGE_H

#include <KTextEditor/ConfigPage>
#include <KLocalizedString>

#include <QFont>
#include <QWidget>

class QDialogButtonBox;
class QFontDialog;

namespace Ui {
class ConfigWidget;
}

/**
 * Configuration page for WarpKate plugin
 */
class WarpKateConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit WarpKateConfigPage(QWidget *parent = nullptr);

    /**
     * Destructor
     */
    ~WarpKateConfigPage() override;

    /**
     * Returns display name for the config page
     */
    QString name() const override;

    /**
     * Returns full name for the config page
     */
    QString fullName() const override;

    /**
     * Returns the icon for the config page
     */
    QIcon icon() const override;

public Q_SLOTS:
    /**
     * Apply settings when requested
     */
    void apply() override;

    /**
     * Reset settings when requested
     */
    void reset() override;

    /**
     * Reset to default settings
     */
    void defaults() override;

private Q_SLOTS:
    /**
     * Handler for when the transparency checkbox is toggled
     */
    void onTransparencyToggled(bool enabled);

    /**
     * Handler for font selection button
     */
    void onFontSelectClicked();

    /**
     * Handler for when the enable AI checkbox is toggled
     */
    void onAIToggled(bool enabled);

private:
    /**
     * Load settings from config
     */
    void loadSettings();

    /**
     * Save settings to config
     */
    void saveSettings();

private:
    Ui::ConfigWidget *m_ui;
    QFont m_terminalFont;
    bool m_changed;
};

#endif // WARPKATECONFIGPAGE_H

