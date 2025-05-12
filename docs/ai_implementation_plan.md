# WarpKate AI Integration Implementation Plan

## Overview

This document outlines a plan to replace the simulated AI responses in WarpKate with real AI functionality by integrating with external AI services like OpenAI's GPT models, Anthropic's Claude, and local models. The implementation will respect the existing UI options (Local/Remote/Custom API) and enhance the plugin's capabilities.

## 1. Architecture Approach

### 1.1 AI Service Provider Interface

We'll implement an abstract interface that defines the contract for all AI service providers, allowing us to:
- Support multiple AI services (OpenAI, Anthropic, local models)
- Switch between providers at runtime based on configuration
- Add new providers in the future without changing core code

```cpp
// Abstract interface for all AI providers
class AIServiceProvider {
public:
    virtual ~AIServiceProvider() = default;
    
    // Core methods
    virtual void initialize() = 0;
    virtual bool isInitialized() const = 0;
    virtual void generateResponse(const QString &query, 
                                  const QString &contextInfo,
                                  std::function<void(const QString&, bool)> responseCallback) = 0;
    
    // Configuration
    virtual void setApiKey(const QString &apiKey) = 0;
    virtual void setModelParameters(const QVariantMap &parameters) = 0;
    virtual QString name() const = 0;
    virtual QStringList availableModels() const = 0;
};
```

### 1.2 Provider Factory

A factory class will instantiate the appropriate provider based on configuration:

```cpp
class AIServiceProviderFactory {
public:
    static AIServiceProvider* createProvider(AIProviderType type);
    // Enum for provider types matching UI options
    enum AIProviderType {
        Local,      // Local models like llama.cpp
        Remote,     // OpenAI/Anthropic
        CustomAPI   // User-defined API
    };
};
```

### 1.3 AI Service Integration Class

A central class to manage AI interactions:

```cpp
class AIService {
public:
    AIService();
    ~AIService();
    
    // Initialize with configuration
    void initialize(const KConfigGroup &config);
    
    // Generate response with callback
    void generateResponse(const QString &query, 
                          const QString &contextInfo,
                          std::function<void(const QString&, bool)> responseCallback);
    
    // Configuration handling
    void setProviderType(AIProviderType type);
    void setApiKey(const QString &apiKey);
    void setModel(const QString &model);
    void setParameters(const QVariantMap &parameters);
    
private:
    std::unique_ptr<AIServiceProvider> m_provider;
    AIProviderType m_providerType;
    QString m_apiKey;
    QString m_model;
    QVariantMap m_parameters;
    bool m_initialized;
};
```

## 2. Files to Modify/Create

### 2.1 New Files

1. **aiservice.h/cpp**:
   - Core AI service management class
   - Handles provider instantiation and configuration

2. **aiprovider.h**:
   - Abstract interface for AI providers
   - Provider factory implementation

3. **openai_provider.h/cpp**:
   - OpenAI API implementation
   - GPT-3.5/4 model support

4. **anthropic_provider.h/cpp**:
   - Claude API implementation

5. **local_provider.h/cpp**:
   - Integration with local models (e.g., llama.cpp)

6. **custom_provider.h/cpp**:
   - Support for custom/alternative APIs

7. **ai_types.h**:
   - Common AI-related type definitions
   - Response data structures

### 2.2 Files to Modify

1. **CMakeLists.txt**:
   - Add new source files
   - Add Qt Network dependency
   - Add optional libllama dependency

2. **warpkateview.h/cpp**:
   - Replace simulated AI with real implementation
   - Update generateAIResponse method

3. **warpkateconfigpage.h/cpp**:
   - Expand UI to support advanced AI settings
   - Handle provider-specific settings

4. **ui/configwidget.ui**:
   - Add additional model selection options
   - Add temperature/max token controls

## 3. New Classes and Methods

### 3.1 AI Provider Implementations

#### OpenAI Provider
```cpp
class OpenAIProvider : public AIServiceProvider {
public:
    OpenAIProvider();
    
    void initialize() override;
    bool isInitialized() const override;
    void generateResponse(const QString &query, 
                          const QString &contextInfo,
                          std::function<void(const QString&, bool)> responseCallback) override;
    
    void setApiKey(const QString &apiKey) override;
    void setModelParameters(const QVariantMap &parameters) override;
    QString name() const override { return "OpenAI"; }
    QStringList availableModels() const override;
    
private:
    void handleNetworkReply(QNetworkReply *reply, std::function<void(const QString&, bool)> callback);
    QJsonObject createRequestPayload(const QString &query, const QString &contextInfo);
    
    QNetworkAccessManager m_networkManager;
    QString m_apiKey;
    QString m_model;
    double m_temperature;
    int m_maxTokens;
    bool m_initialized;
};
```

#### Local Provider (for local Llama models)
```cpp
class LocalProvider : public AIServiceProvider {
public:
    LocalProvider();
    
    void initialize() override;
    bool isInitialized() const override;
    void generateResponse(const QString &query, 
                          const QString &contextInfo,
                          std::function<void(const QString&, bool)> responseCallback) override;
    
    void setApiKey(const QString &apiKey) override; // Not used but required
    void setModelParameters(const QVariantMap &parameters) override;
    QString name() const override { return "Local Model"; }
    QStringList availableModels() const override;
    
private:
    // For local model implementation
    void loadModel(const QString &modelPath);
    QString m_modelPath;
    bool m_initialized;
    // Local model context/instance would be stored here
};
```

### 3.2 Response and Context Handling

```cpp
struct AIResponse {
    QString text;
    QString modelUsed;
    int tokensUsed;
    double processingTime;
};

class ContextBuilder {
public:
    static QString buildContext(KTextEditor::View *view, const QString &query);
    static QString extractRelevantCode(KTextEditor::Document *doc);
};
```

### 3.3 Enhanced WarpKateView Methods

```cpp
// In warpkateview.h
private:
    AIService m_aiService;
    void setupAIService();
    void handleAIResponse(const QString &response, bool isComplete);
    void appendToConversation(const QString &text, bool isAiResponse);
```

## 4. API Key Security

### 4.1 Secure Storage Approach

1. **KWallet Integration**:
   - Store API keys in KDE's secure wallet
   - Implement fallback for when KWallet is unavailable

2. **Memory Protection**:
   - Minimize API key exposure in memory
   - Clear key from memory when not in use

3. **Configuration Handling**:
   - Store only encrypted/hashed references in config files
   - Never write raw API keys to disk

### 4.2 Implementation Details

```cpp
class APIKeyManager {
public:
    static APIKeyManager& instance();
    
    // Store key in wallet
    bool storeApiKey(const QString &serviceName, const QString &apiKey);
    
    // Retrieve key from wallet
    QString retrieveApiKey(const QString &serviceName);
    
    // Check if key exists
    bool hasApiKey(const QString &serviceName);
    
    // Remove key
    bool removeApiKey(const QString &serviceName);
    
private:
    APIKeyManager();
    ~APIKeyManager();
    
    KWallet::Wallet* openWallet();
    QString m_walletFolder;
    KWallet::Wallet* m_wallet;
};
```

### 4.3 UI Integration for Key Management

- Add "Test Connection" button in API Key section
- Add visual indicator of key validity
- Implement "Forget Key" functionality

## 5. UI Integration

### 5.1 Mapping UI Options to Implementations

Map the existing UI options to concrete implementations:

1. **Local (Fast)**:
   - Use local models through llama.cpp binding
   - Folder selection for model location
   - Model selection dropdown populated from available models

2. **Remote (Advanced)**:
   - OpenAI/Anthropic integration
   - Service selection dropdown (OpenAI, Anthropic)
   - Model selection based on service
   - Advanced parameters (temperature, max tokens)

3. **Custom API**:
   - Configurable endpoint URL
   - Request/response format templates
   - Authentication method selection

### 5.2 Extended Configuration UI

Extend the current configuration UI:

```xml
<!-- Added to configwidget.ui -->
<widget class="QComboBox" name="serviceProviderCombo">
  <item><property name="text"><string>OpenAI</string></property></item>
  <item><property name="text"><string>Anthropic</string></property></item>
</widget>

<widget class="QComboBox" name="modelSelectionCombo"/>

<widget class="QSlider" name="temperatureSlider">
  <property name="minimum"><number>0</number></property>
  <property name="maximum"><number>100</number></property>
  <property name="value"><number>70</number></property>
</widget>

<widget class="QSpinBox" name="maxTokensSpinBox">
  <property name="minimum"><number>100</number></property>
  <property name="maximum"><number>8000</number></property>
  <property name="value"><number>1000</number></property>
</widget>
```

### 5.3 Dynamic UI Updates

```cpp
// In warpkateconfigpage.cpp
void WarpKateConfigPage::onAIModelChanged(int index) {
    AIProviderType type = static_cast<AIProviderType>(index);
    
    // Update visible options based on selection
    switch(type) {
        case AIProviderType::Local:
            m_ui->serviceProviderCombo->setVisible(false);
            m_ui->modelFolderButton->setVisible(true);
            // Populate model dropdown with local models
            populateLocalModels();
            break;
            
        case AIProviderType::Remote:
            m_ui->serviceProviderCombo->setVisible(true);
            m_ui->modelFolderButton->setVisible(false);
            // Populate model dropdown based on selected service
            populateRemoteModels(m_ui->serviceProviderCombo->currentIndex());
            break;
            
        case AIProviderType::CustomAPI:
            m_ui->serviceProviderCombo->setVisible(false);
            m_ui->modelFolderButton->setVisible(false);
            m_ui->endpointUrlEdit->setVisible(true);
            break;
    }
}
```

## 6. Implementation Roadmap

### Phase 1: Foundation
1. Create AI provider interface
2. Implement OpenAI provider
3. Basic API key storage
4. Replace simulated responses with OpenAI

### Phase 2: Advanced Features
1. Add Anthropic provider
2. Implement proper context building
3. Enhance UI with additional models
4. Add streaming response support

### Phase 3: Local Models
1. Add local model support
2. Implement model downloading/management
3. Add custom API provider

### Phase 4: Polish
1. Optimize performance
2. Add conversation history
3. Implement persistent chats
4. Add automatic context handling

## 7. Dependencies

1. **Qt Network** module for API requests
2. **KWallet Framework** for secure API key storage
3. **OpenSSL** for secure communications
4. **Optional**: llama.cpp for local models

## 8. Testing Strategy

1. Create mock AI providers for testing
2. Implement unit tests for each provider
3. Test with various API keys and configurations
4. Verify secure handling of sensitive data

## Conclusion

This implementation plan provides a flexible, secure, and maintainable approach to integrating real AI capabilities into WarpKate. The abstracted provider interface ensures that we can support multiple AI services and easily add new ones in the future, while the secure API key handling protects user credentials.

