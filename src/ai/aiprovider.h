/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WARPKATE_AIPROVIDER_H
#define WARPKATE_AIPROVIDER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <functional>
#include <memory>

/**
 * Enum for AI provider types, matching the UI options in the configuration.
 */
enum class AIProviderType {
    Local,      // Local models like llama.cpp
    Remote,     // OpenAI/Anthropic
    CustomAPI   // User-defined API
};

/**
 * @brief Abstract interface for all AI service providers
 * 
 * This interface defines the contract that all AI providers must implement.
 * It allows for swapping different AI backends (OpenAI, Anthropic, local models, etc.)
 * while maintaining the same API for the rest of the application.
 */
class AIServiceProvider {
public:
    /**
     * Virtual destructor for proper cleanup in derived classes
     */
    virtual ~AIServiceProvider() = default;
    
    /**
     * Initialize the provider with its current configuration
     */
    virtual void initialize() = 0;
    
    /**
     * Check if the provider is properly initialized
     * @return True if initialized and ready to use, false otherwise
     */
    virtual bool isInitialized() const = 0;
    
    /**
     * Generate an AI response to a query
     * 
     * @param query The user's question or instruction
     * @param contextInfo Additional context information (e.g., code from the editor)
     * @param responseCallback Callback function to receive response chunks and completion status
     *        The boolean parameter indicates if this is the final chunk (true) or if more are coming (false)
     */
    virtual void generateResponse(
        const QString &query, 
        const QString &contextInfo,
        std::function<void(const QString&, bool)> responseCallback
    ) = 0;
    
    /**
     * Set the API key for the service
     * @param apiKey The API key for authentication
     */
    virtual void setApiKey(const QString &apiKey) = 0;
    
    /**
     * Set parameters for the model (temperature, max tokens, etc.)
     * @param parameters Key-value map of parameters
     */
    virtual void setModelParameters(const QVariantMap &parameters) = 0;
    
    /**
     * Get the provider's human-readable name
     * @return Name of the provider (e.g., "OpenAI", "Anthropic")
     */
    virtual QString name() const = 0;
    
    /**
     * Get a list of available models for this provider
     * @return List of model identifiers
     */
    virtual QStringList availableModels() const = 0;
};

/**
 * @brief Factory class for creating AI service providers
 * 
 * This factory instantiates the appropriate provider based on the
 * configured provider type.
 */
class AIServiceProviderFactory {
public:
    /**
     * Create a provider instance based on the specified type
     * 
     * @param type The type of provider to create
     * @return A new provider instance (caller takes ownership)
     */
    static AIServiceProvider* createProvider(AIProviderType type);
};

#endif // WARPKATE_AIPROVIDER_H

