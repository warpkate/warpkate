/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WARPKATE_APIKEYMANAGER_H
#define WARPKATE_APIKEYMANAGER_H

#include <QObject>
#include <QString>

/**
 * @brief Manager for API keys (temporary implementation)
 * 
 * This class provides a way to store and retrieve API keys.
 * TEMPORARY IMPLEMENTATION: Currently uses QSettings with simple
 * obfuscation rather than KWallet for secure storage.
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
     * Store an API key
     * 
     * @param serviceName Name of the service (e.g., "OpenAI", "Anthropic")
     * @param apiKey The API key to store
     * @return True if the key was successfully stored
     */
    bool storeApiKey(const QString &serviceName, const QString &apiKey);
    
    /**
     * Retrieve an API key
     * 
     * @param serviceName Name of the service
     * @return The API key if found, or an empty string if not found
     */
    QString retrieveApiKey(const QString &serviceName);
    
    /**
     * Check if an API key exists
     * 
     * @param serviceName Name of the service
     * @return True if the key exists, false otherwise
     */
    bool hasApiKey(const QString &serviceName);
    
    /**
     * Remove an API key
     * 
     * @param serviceName Name of the service
     * @return True if the key was successfully removed
     */
    bool removeApiKey(const QString &serviceName);

    // Removed signals section temporarily

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
     * Simple obfuscation function (NOT secure)
     */
    QString obfuscate(const QString &apiKey);
    
    /**
     * Deobfuscate function
     */
    QString deobfuscate(const QString &obfuscatedKey);
    
    // Disable copy construction and assignment
    APIKeyManager(const APIKeyManager&) = delete;
    APIKeyManager& operator=(const APIKeyManager&) = delete;

    // Singleton instance
    static APIKeyManager* s_instance;
};

#endif // WARPKATE_APIKEYMANAGER_H
