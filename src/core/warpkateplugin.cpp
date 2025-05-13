#include <QFile>
#include <QDir>
#include <QStandardPaths>
/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "warpkateplugin.h"
#include "warpkateview.h"
#include "warpkateconfigpage.h"

#include <KPluginFactory>
#include <KLocalizedString>
#include <KXMLGUIFactory>
#include <KActionCollection>
#include <KConfigGroup>

#include <QAction>
#include <QApplication>
#include <QDebug>

K_PLUGIN_FACTORY_WITH_JSON(WarpKatePluginFactory, "warpkateplugin.json", registerPlugin<WarpKatePlugin>();)

WarpKatePlugin::WarpKatePlugin(QObject *parent, const QVariantList &args)
    : KTextEditor::Plugin(parent)
{
    Q_UNUSED(args);
    qDebug() << "WarpKate Plugin: Initializing...";
}

WarpKatePlugin::~WarpKatePlugin()
{
    qDebug() << "WarpKate Plugin: Shutting down...";
}

QObject *WarpKatePlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    // Create a new view for this main window
    WarpKateView *view = new WarpKateView(this, mainWindow);
    m_views[mainWindow] = view;
    
    // Return the view
    return view;
}

int WarpKatePlugin::configPages() const
{
    // Only one config page for now
    return 1;
}

KTextEditor::ConfigPage *WarpKatePlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }
    
    // Return a new instance of our config page
    return new WarpKateConfigPage(parent);
}

#include "warpkateplugin.moc"

#include "moc_warpkateplugin.cpp"

