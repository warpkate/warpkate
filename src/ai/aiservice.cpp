/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "aiservice.h"

#include <QDebug>
#include <QStandardPaths>

// Constructor
AIService::AIService(QObject *parent)
    : QObject(parent)
    , m_providerType(AIProviderType::Remote)  // Default to Remote (OpenAI)
    , m_model(QStringLiteral("gpt-3.5-turbo"))      // Default model
    , m_initialized(false)
{
    // Initialize default parameters
    m_parameters[QStringLiteral("temperature")] = DEFAULT_TEMPERATURE;
    m_parameters[QStringLiteral("maxTokens")] = DEFAULT_MAX_TOKENS;
}

// Destructor
AIService::~AIService()
{
    // Provider is managed by unique_ptr, so it will be automatically destroyed
}

// Initialize with configuration
bool AIService::initialize(const KConfigGroup &config)
{
    // Load provider type
    int providerTypeInt = config.readEntry(QStringLiteral("AIModel"), 1); // Default to Remote (index 1)
    m_providerType = static_cast<AIProviderType>(providerTypeInt);
    
    // Load model
    m_model = config.readEntry(QStringLiteral("Model"), QStringLiteral("gpt-3.5-turbo"));
    
    // Load parameters
    m_parameters[QStringLiteral("temperature")] = config.readEntry(QStringLiteral("Temperature"), DEFAULT_TEMPERATURE);
    m_parameters[QStringLiteral("maxTokens")] = config.readEntry(QStringLiteral("MaxTokens"), DEFAULT_MAX_TOKENS);
    
    // Load API key (in a real implementation, this would use a secure storage mechanism)
    if (!loadApiKey()) {
        qWarning() << "Failed to load API key for provider:" << static_cast<int>(m_providerType);
        m_apiKey = config.readEntry(QStringLiteral("APIKey"), QString());
    }
    
    // Set up the provider
    setupProvider();
    
    return m_initialized;
}

// Set up the provider based on current settings
void AIService::setupProvider()
{
    // Create the appropriate provider
    m_provider.reset(AIServiceProviderFactory::createProvider(m_providerType));
    
    if (m_provider) {
        // Configure the provider
        m_provider->setApiKey(m_apiKey);
        m_provider->setModelParameters(m_parameters);
        
        // Initialize the provider
        m_provider->initialize();
        
        // Check if provider is initialized
        m_initialized = m_provider->isInitialized();
        
        if (!m_initialized) {
            qWarning() << "Failed to initialize AI provider:" << m_provider->name();
        } else {
            qDebug() << "Successfully initialized AI provider:" << m_provider->name();
        }
    } else {
        m_initialized = false;
        qWarning() << "Failed to create AI provider for type:" << static_cast<int>(m_providerType);
    }
}

// Load API key
bool AIService::loadApiKey()
{
    // TODO: Implement secure API key loading with KWallet
    // For now, return false to indicate we couldn't load from secure storage
    // This will cause the code to fall back to the config entry
    return false;
}

// Generate response
void AIService::generateResponse(
    const QString &query, 
    const QString &contextInfo,
    std::function<void(const QString&, bool)> responseCallback
)
{
    if (!isReady()) {
        responseCallback(QStringLiteral("AI service is not properly initialized. Please check your configuration."), true);
        return;
    }
    
    // Delegate to the provider
    m_provider->generateResponse(query, contextInfo, responseCallback);
}

// Check if ready
bool AIService::isReady() const
{
    return m_initialized && m_provider != nullptr;
}

// Set provider type
void AIService::setProviderType(AIProviderType type)
{
    if (m_providerType != type) {
        m_providerType = type;
        setupProvider();
    }
}

// Set API key
void AIService::setApiKey(const QString &apiKey)
{
    if (m_apiKey != apiKey) {
        m_apiKey = apiKey;
        
        // Update the provider with the new key
        if (m_provider) {
            m_provider->setApiKey(m_apiKey);
        }
        
        // TODO: Store API key securely
    }
}

// Set model
void AIService::setModel(const QString &model)
{
    if (m_model != model) {
        m_model = model;
        
        // Update parameters with new model
        m_parameters[QStringLiteral("model")] = m_model;
        
        // Update the provider
        if (m_provider) {
            m_provider->setModelParameters(m_parameters);
        }
    }
}

// Set parameters
void AIService::setParameters(const QVariantMap &parameters)
{
    // Merge the new parameters with existing ones
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        m_parameters[it.key()] = it.value();
    }
    
    // Update the provider
    if (m_provider) {
        m_provider->setModelParameters(m_parameters);
    }
}

// Get provider type
AIProviderType AIService::providerType() const
{
    return m_providerType;
}

// Get model name
QString AIService::modelName() const
{
    return m_model;
}

// Get available models
QStringList AIService::availableModels() const
{
    if (m_provider) {
        return m_provider->availableModels();
    }
    return QStringList();
}

// Get provider name
QString AIService::providerName() const
{
    if (m_provider) {
        return m_provider->name();
    }
    return QStringLiteral("Unknown");
}

// Test connection
void AIService::testConnection(std::function<void(bool, const QString&)> resultCallback)
{
    if (!isReady()) {
        resultCallback(false, QStringLiteral("AI service is not properly initialized. Please check your configuration."));
        return;
    }
    
    // Simple test: try to generate a minimal response
    m_provider->generateResponse(
        QStringLiteral("Test connection"),
        QStringLiteral(""),
        [resultCallback](const QString& response, bool isFinal) {
            if (isFinal) {
                resultCallback(true, QStringLiteral("Connection successful. Response: ") + response);
            }
        }
    );
}

// Save configuration
void AIService::saveConfiguration(KConfigGroup &config) const
{
    config.writeEntry(QStringLiteral("AIModel"), static_cast<int>(m_providerType));
    config.writeEntry(QStringLiteral("Model"), m_model);
    config.writeEntry(QStringLiteral("Temperature"), m_parameters.value(QStringLiteral("temperature"), DEFAULT_TEMPERATURE).toDouble());
    config.writeEntry(QStringLiteral("MaxTokens"), m_parameters.value(QStringLiteral("maxTokens"), DEFAULT_MAX_TOKENS).toInt());
    
    // API key is only saved temporarily for this initial implementation
    // In a real implementation, we'd store it securely and only keep a reference
    config.writeEntry(QStringLiteral("APIKey"), m_apiKey);
    
    config.sync();
}

#include "moc_aiservice.cpp"

