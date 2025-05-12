/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "apikeymanager.h"

#include <KWallet/KWallet>
#include <QApplication>
#include <QDebug>

// KWallet folder name for WarpKate API keys
static const QString WALLET_FOLDER = "WarpKate";

// Static instance for singleton pattern
APIKeyManager& APIKeyManager::instance()
{
    static APIKeyManager instance;
    return instance;
}

APIKeyManager::APIKeyManager(QObject *parent)
    : QObject(parent)
    , m_wallet(nullptr)
    , m_walletFolder(WALLET_FOLDER)
    , m_walletOpenAttempted(false)
{
    // Constructor is private - instance() is the only way to get an instance
}

APIKeyManager::~APIKeyManager()
{
    closeWallet();
}

bool APIKeyManager::isWalletAvailable() const
{
    return KWallet::Wallet::isEnabled();
}

KWallet::Wallet* APIKeyManager::openWallet()
{
    // If we already have an open wallet, return it
    if (m_wallet && m_wallet->isOpen()) {
        return m_wallet;
    }
    
    // If we've already tried to open the wallet and failed, don't try again
    if (m_walletOpenAttempted && !m_wallet) {
        return nullptr;
    }
    
    m_walletOpenAttempted = true;
    
    // Check if KWallet is available
    if (!isWalletAvailable()) {
        qWarning() << "KWallet is not available for secure API key storage";
        emit walletError("KWallet is not available. API keys cannot be stored securely.");
        return nullptr;
    }
    
    // Open the KDE wallet
    m_wallet = KWallet::Wallet::openWallet(
        KWallet::Wallet::LocalWallet(),  // Use the default local wallet
        QApplication::activeWindow() ? QApplication::activeWindow()->winId() : 0,
        KWallet::Wallet::Asynchronous  // Don't block the UI
    );
    
    if (!m_wallet) {
        qWarning() << "Failed to open KWallet";
        emit walletError("Failed to open KDE wallet. API keys cannot be stored securely.");
        return nullptr;
    }
    
    // Create our folder if it doesn't exist
    if (!m_wallet->hasFolder(m_walletFolder)) {
        if (!m_wallet->createFolder(m_walletFolder)) {
            qWarning() << "Failed to create KWallet folder:" << m_walletFolder;
            emit walletError("Failed to create WarpKate folder in KDE wallet.");
            closeWallet();
            return nullptr;
        }
    }
    
    // Open our folder
    if (!m_wallet->setFolder(m_walletFolder)) {
        qWarning() << "Failed to open KWallet folder:" << m_walletFolder;
        emit walletError("Failed to open WarpKate folder in KDE wallet.");
        closeWallet();
        return nullptr;
    }
    
    return m_wallet;
}

void APIKeyManager::closeWallet()
{
    if (m_wallet) {
        delete m_wallet;
        m_wallet = nullptr;
    }
}

bool APIKeyManager::storeApiKey(const QString &serviceName, const QString &apiKey)
{
    KWallet::Wallet* wallet = openWallet();
    if (!wallet) {
        return false;
    }
    
    // Store the API key
    int result = wallet->writePassword(serviceName, apiKey);
    if (result != 0) {
        qWarning() << "Failed to store API key for service:" << serviceName;
        emit walletError(QString("Failed to store API key for %1.").arg(serviceName));
        return false;
    }
    
    qDebug() << "Successfully stored API key for service:" << serviceName;
    return true;
}

QString APIKeyManager::retrieveApiKey(const QString &serviceName)
{
    KWallet::Wallet* wallet = openWallet();
    if (!wallet) {
        return QString();
    }
    
    // Retrieve the API key
    QString apiKey;
    int result = wallet->readPassword(serviceName, apiKey);
    if (result != 0) {
        qWarning() << "Failed to retrieve API key for service:" << serviceName;
        return QString();
    }
    
    return apiKey;
}

bool APIKeyManager::hasApiKey(const QString &serviceName)
{
    KWallet::Wallet* wallet = openWallet();
    if (!wallet) {
        return false;
    }
    
    // Check if the key exists
    return wallet->hasEntry(serviceName);
}

bool APIKeyManager::removeApiKey(const QString &serviceName)
{
    KWallet::Wallet* wallet = openWallet();
    if (!wallet) {
        return false;
    }
    
    // Remove the API key
    int result = wallet->removeEntry(serviceName);
    if (result != 0) {
        qWarning() << "Failed to remove API key for service:" << serviceName;
        emit walletError(QString("Failed to remove API key for %1.").arg(serviceName));
        return false;
    }
    
    qDebug() << "Successfully removed API key for service:" << serviceName;
    return true;
}

#include "moc_apikeymanager.cpp"

