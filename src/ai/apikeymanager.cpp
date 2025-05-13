/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "apikeymanager.h"

#include <QDebug>
#include <QSettings>
#include <QCryptographicHash>

// Singleton instance
APIKeyManager* APIKeyManager::s_instance = nullptr;

// Get singleton instance
APIKeyManager& APIKeyManager::instance()
{
    if (!s_instance) {
        s_instance = new APIKeyManager();
    }
    return *s_instance;
}

// Constructor
APIKeyManager::APIKeyManager(QObject *parent)
    : QObject(parent)
{
    qWarning() << "WARNING: Using temporary insecure API key storage. Do not use sensitive API keys.";
}

// Destructor
APIKeyManager::~APIKeyManager()
{
}

// Store API key (temporary implementation using QSettings)
bool APIKeyManager::storeApiKey(const QString &serviceName, const QString &apiKey)
{
    if (serviceName.isEmpty() || apiKey.isEmpty()) {
        return false;
    }
    
    QSettings settings(QStringLiteral("WarpKate"), QStringLiteral("APIKeys"));
    
    // Simple obfuscation (not real security)
    QString obfuscatedKey = obfuscate(apiKey);
    settings.setValue(serviceName, obfuscatedKey);
    
    return true;
}

// Retrieve API key
QString APIKeyManager::retrieveApiKey(const QString &serviceName)
{
    if (serviceName.isEmpty()) {
        return QString();
    }
    
    QSettings settings(QStringLiteral("WarpKate"), QStringLiteral("APIKeys"));
    
    if (!settings.contains(serviceName)) {
        return QString();
    }
    
    QString obfuscatedKey = settings.value(serviceName).toString();
    return deobfuscate(obfuscatedKey);
}

// Check if key exists
bool APIKeyManager::hasApiKey(const QString &serviceName)
{
    if (serviceName.isEmpty()) {
        return false;
    }
    
    QSettings settings(QStringLiteral("WarpKate"), QStringLiteral("APIKeys"));
    return settings.contains(serviceName);
}

// Remove key
bool APIKeyManager::removeApiKey(const QString &serviceName)
{
    if (serviceName.isEmpty()) {
        return false;
    }
    
    QSettings settings(QStringLiteral("WarpKate"), QStringLiteral("APIKeys"));
    
    if (!settings.contains(serviceName)) {
        return false;
    }
    
    settings.remove(serviceName);
    return true;
}

// Very simple obfuscation (NOT secure)
QString APIKeyManager::obfuscate(const QString &apiKey)
{
    QByteArray keyBytes = apiKey.toUtf8();
    return QString::fromLatin1(keyBytes.toBase64());
}

// Deobfuscate (reverse of obfuscate)
QString APIKeyManager::deobfuscate(const QString &obfuscatedKey)
{
    QByteArray keyBytes = QByteArray::fromBase64(obfuscatedKey.toLatin1());
    return QString::fromUtf8(keyBytes);
}

#include "moc_apikeymanager.cpp"
