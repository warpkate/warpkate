/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WARPKATEPLUGIN_H
#define WARPKATEPLUGIN_H

#include <KTextEditor/Plugin>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QVariantList>

#include "warpkate_export.h"

class WarpKateView;

/**
 * Plugin for Kate to integrate Warp Terminal features
 * This plugin provides a Warp-like terminal experience within Kate
 */
class WARPKATE_EXPORT WarpKatePlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit WarpKatePlugin(QObject *parent = nullptr, const QVariantList &args = QVariantList());

    /**
     * Destructor
     */
    ~WarpKatePlugin() override;
    
    /**
     * Create a new view of this plugin for the given main window
     * @param mainWindow main window for which to create the plugin view
     * @return new plugin view for the given main window
     */
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    /**
     * Number of config pages for this plugin
     * @return number of config pages, 1 in our case
     */
    int configPages() const override;
    
    /**
     * Create a new plugin view for the given main window
     * @param mainWindow the main window to create the view for
     * @return the new created view
     */
    KTextEditor::ConfigPage *configPage(int number, QWidget *parent) override;
    
private:
    /**
     * All plugin views for this plugin per main window
     */
    QHash<KTextEditor::MainWindow *, WarpKateView *> m_views;
};

#endif // WARPKATEPLUGIN_H
