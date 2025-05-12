/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WARPKATE_OPENAI_PROVIDER_H
#define WARPKATE_OPENAI_PROVIDER_H

#include "aiprovider.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief OpenAI API service provider implementation
 * 
 * This class implements the AIServiceProvider interface for the OpenAI API,
 * supporting models like GPT-3.5-Turbo and GPT-4. It handles API communication,
 * request formatting, and response parsing.
 */
class OpenAIProvider : public AIServiceProvider
{
public:
    /**
     * Constructor
     */
    OpenAIProvider();
    
    /**
     * Destructor
     */
    ~OpenAIProvider() override;
    
    /**
     * Initialize the provider
     */
    void initialize() override;
    
    /**
     * Check if initialized
     * @return True if properly initialized with API key
     */
    bool isInitialized() const override;
    
    /**
     * Generate a response via the OpenAI API
     * 
     * @param query User's query
     * @param contextInfo Additional context
     * @param responseCallback Callback for receiving response chunks
     */
    void generateResponse(
        const QString &query, 
        const QString &contextInfo,
        std::function<void(const QString&, bool)> responseCallback
    ) override;
    
    /**
     * Set the API key for OpenAI
     * @param apiKey OpenAI API key
     */
    void setApiKey(const QString &apiKey) override;
    
    /**
     * Set parameters for the model
     * @param parameters Parameter map including temperature, max_tokens, etc.
     */
    void setModelParameters(const QVariantMap &parameters) override;
    
    /**
     * Get provider name
     * @return "OpenAI"
     */
    QString name() const override { return QStringLiteral("OpenAI"); }
    
    /**
     * Get available models
     * @return List of available OpenAI model IDs
     */
    QStringList availableModels() const override;

private:
    /**
     * Handle network reply from API request
     * @param reply Network reply object
     * @param callback Response callback
     */
    void handleNetworkReply(QNetworkReply *reply, std::function<void(const QString&, bool)> callback);
    
    /**
     * Create request payload in OpenAI format
     * @param query User query
     * @param contextInfo Context information
     * @return JSON object with formatted request
     */
    QJsonObject createRequestPayload(const QString &query, const QString &contextInfo);
    
    /**
     * Extract content from OpenAI API response
     * @param jsonResponse JSON response from API
     * @return Extracted content text
     */
    QString extractContentFromResponse(const QJsonObject &jsonResponse);
    
    /**
     * Format an error message
     * @param errorData Error information from API
     * @return Formatted error message
     */
    QString formatErrorMessage(const QJsonObject &errorData);
    
    /**
     * Format system message with context
     * @param contextInfo Context information to include
     * @return Formatted system message
     */
    QString formatSystemMessage(const QString &contextInfo);
    
    // API endpoint
    QString m_apiEndpoint;
    
    // Network access manager for API communication
    QNetworkAccessManager m_networkManager;
    
    // API key
    QString m_apiKey;
    
    // Model ID
    QString m_model;
    
    // Parameters
    double m_temperature;
    int m_maxTokens;
    
    // Provider state
    bool m_initialized;
    
    // Default model ID
    static const QString DEFAULT_MODEL;
    
    // List of supported models
    static const QStringList SUPPORTED_MODELS;
};

#endif // WARPKATE_OPENAI_PROVIDER_H

