/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WARPKATE_APIKEYMANAGER_H
#define WARPKATE_APIKEYMANAGER_H

#include <QString>
#include <QObject>

namespace KWallet {
    class Wallet;
}

/**
 * @brief Secure manager for API keys
 * 
 * This class provides a secure way to store and retrieve API keys
 * using KDE's KWallet. It ensures that sensitive credentials are
 * not stored in plain text in configuration files.
 * 
 * The class follows the singleton pattern and should be accessed
 * through the instance() method.
 */
class APIKeyManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Get the singleton instance of the API key manager
     * @return Reference to the API key manager instance
     */
    static APIKeyManager& instance();
    
    /**
     * Check if KWallet is available for secure storage
     * @return True if KWallet is available, false otherwise
     */
    bool isWalletAvailable() const;
    
    /**
     * Store an API key in the wallet
     * 
     * @param serviceName Name of the service (e.g., "OpenAI", "Anthropic")
     * @param apiKey The API key to store
     * @return True if the key was successfully stored
     */
    bool storeApiKey(const QString &serviceName, const QString &apiKey);
    
    /**
     * Retrieve an API key from the wallet
     * 
     * @param serviceName Name of the service
     * @return The API key if found, or an empty string if not found
     */
    QString retrieveApiKey(const QString &serviceName);
    
    /**
     * Check if an API key exists in the wallet
     * 
     * @param serviceName Name of the service
     * @return True if the key exists, false otherwise
     */
    bool hasApiKey(const QString &serviceName);
    
    /**
     * Remove an API key from the wallet
     * 
     * @param serviceName Name of the service
     * @return True if the key was successfully removed
     */
    bool removeApiKey(const QString &serviceName);

signals:
    /**
     * Emitted when a wallet error occurs
     * @param message Error message
     */
    void walletError(const QString &message);

private:
    /**
     * Private constructor (singleton pattern)
     * @param parent QObject parent
     */
    explicit APIKeyManager(QObject *parent = nullptr);
    
    /**
     * Destructor
     */
    ~APIKeyManager();
    
    /**
     * Open the KWallet
     * @return Pointer to opened wallet, or nullptr if failed
     */
    KWallet::Wallet* openWallet();
    
    /**
     * Close the wallet if it's open
     */
    void closeWallet();
    
    // Disable copy construction and assignment
    APIKeyManager(const APIKeyManager&) = delete;
    APIKeyManager& operator=(const APIKeyManager&) = delete;

    // Wallet instance
    KWallet::Wallet* m_wallet;
    
    // Wallet folder for WarpKate
    QString m_walletFolder;
    
    // Has attempt to open wallet been made
    bool m_walletOpenAttempted;
};

#endif // WARPKATE_APIKEYMANAGER_H

