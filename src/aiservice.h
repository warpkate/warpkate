/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WARPKATE_AISERVICE_H
#define WARPKATE_AISERVICE_H

#include "aiprovider.h"

#include <KConfigGroup>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <functional>
#include <memory>

/**
 * @brief Central service for managing AI interactions
 * 
 * This class coordinates AI functionality for WarpKate,
 * handling provider selection, configuration, and response generation.
 * It serves as the interface between the UI components and the
 * underlying AI provider implementations.
 */
class AIService : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parent QObject parent
     */
    explicit AIService(QObject *parent = nullptr);
    
    /**
     * Destructor
     */
    ~AIService() override;
    
    /**
     * Initialize the AI service with configuration
     * @param config KDE configuration group containing AI settings
     * @return True if initialization was successful
     */
    bool initialize(const KConfigGroup &config);
    
    /**
     * Generate a response to a user query
     * 
     * @param query The user's question or command
     * @param contextInfo Additional context information (e.g., document content)
     * @param responseCallback Callback function receiving response chunks and completion status
     */
    void generateResponse(
        const QString &query, 
        const QString &contextInfo,
        std::function<void(const QString&, bool)> responseCallback
    );
    
    /**
     * Check if the service is initialized and ready to use
     * @return True if ready, false otherwise
     */
    bool isReady() const;
    
    /**
     * Set the provider type (Local, Remote, CustomAPI)
     * @param type Provider type to use
     */
    void setProviderType(AIProviderType type);
    
    /**
     * Set the API key for the current provider
     * @param apiKey Provider-specific API key
     */
    void setApiKey(const QString &apiKey);
    
    /**
     * Set the model name/ID for the current provider
     * @param model Provider-specific model identifier
     */
    void setModel(const QString &model);
    
    /**
     * Set additional parameters for the AI model
     * @param parameters Key-value map of model parameters (e.g., temperature, max tokens)
     */
    void setParameters(const QVariantMap &parameters);
    
    /**
     * Get the current provider type
     * @return Current provider type
     */
    AIProviderType providerType() const;
    
    /**
     * Get the current model name/ID
     * @return Current model identifier
     */
    QString modelName() const;
    
    /**
     * Get the list of available models for the current provider
     * @return List of model identifiers
     */
    QStringList availableModels() const;
    
    /**
     * Get the current provider name
     * @return Human-readable provider name
     */
    QString providerName() const;
    
    /**
     * Test the current provider connection
     * @param resultCallback Callback function receiving test result (success/failure message)
     */
    void testConnection(std::function<void(bool, const QString&)> resultCallback);
    
    /**
     * Save current configuration to the KDE config system
     * @param config KDE configuration group to save to
     */
    void saveConfiguration(KConfigGroup &config) const;

private:
    /**
     * Create and configure a provider based on current settings
     */
    void setupProvider();
    
    /**
     * Load API key for the current provider
     * @return True if API key was successfully loaded
     */
    bool loadApiKey();
    
    // Provider instance (owned by this service)
    std::unique_ptr<AIServiceProvider> m_provider;
    
    // Configuration
    AIProviderType m_providerType;
    QString m_apiKey;
    QString m_model;
    QVariantMap m_parameters;
    bool m_initialized;
    
    // Default parameter values
    static constexpr double DEFAULT_TEMPERATURE = 0.7;
    static constexpr int DEFAULT_MAX_TOKENS = 1000;
};

#endif // WARPKATE_AISERVICE_H

