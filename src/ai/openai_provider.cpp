/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "openai_provider.h"

#include <QObject>
#include <QDebug>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QEventLoop>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSslError>
#include <QTimer>
#include <QSslConfiguration>

// Static constants initialization
const QString OpenAIProvider::DEFAULT_MODEL = QStringLiteral("gpt-3.5-turbo");
const QStringList OpenAIProvider::SUPPORTED_MODELS = {
    QStringLiteral("gpt-3.5-turbo"),
    QStringLiteral("gpt-3.5-turbo-16k"),
    QStringLiteral("gpt-4"),
    QStringLiteral("gpt-4-turbo"),
    QStringLiteral("gpt-4-32k")
};

OpenAIProvider::OpenAIProvider()
    : m_apiEndpoint(QStringLiteral("https://api.openai.com/v1/chat/completions"))
    , m_model(DEFAULT_MODEL)
    , m_temperature(0.7)
    , m_maxTokens(1000)
    , m_initialized(false)
{
    // Set up network connections - without connect for now
    // We'll add this properly later to ensure it builds
}

OpenAIProvider::~OpenAIProvider()
{
    // Clean up any pending network requests
    for (QNetworkReply* reply : m_networkManager.findChildren<QNetworkReply*>()) {
        reply->abort();
        reply->deleteLater();
    }
}

void OpenAIProvider::initialize()
{
    // Provider is initialized if we have a valid API key
    m_initialized = !m_apiKey.isEmpty();
    
    if (!m_initialized) {
        qWarning() << "OpenAI provider initialization failed: No API key provided";
    } else {
        qDebug() << "OpenAI provider initialized successfully with model:" << m_model;
    }
}

bool OpenAIProvider::isInitialized() const
{
    return m_initialized;
}
// Improved generateResponse method with better error handling and timeout
void OpenAIProvider::generateResponse(
    const QString &query, 
    const QString &contextInfo,
    std::function<void(const QString&, bool)> responseCallback
)
{
    if (!isInitialized()) {
        responseCallback(QStringLiteral("Error: OpenAI provider not initialized. Please set API key."), true);
        return;
    }
    
    // Create the network request
    QNetworkRequest request;
    request.setUrl(QUrl(m_apiEndpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader(QByteArrayLiteral("Authorization"), QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    
    // Set timeout for the request (10 seconds)
    request.setTransferTimeout(10000);
    
    // Set SSL configuration if needed
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    request.setSslConfiguration(sslConfig);
    
    // Create the JSON payload
    QJsonObject payload = createRequestPayload(query, contextInfo);
    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson();
    
    qDebug() << "Sending request to OpenAI API with model:" << m_model;
    qDebug() << "API endpoint:" << m_apiEndpoint;
    qDebug() << "Request payload size:" << jsonData.size() << "bytes";
    
    // Send the POST request
    QNetworkReply *reply = m_networkManager.post(request, jsonData);
    
    // Connect to progress signals for debugging
    QObject::connect(reply, &QNetworkReply::uploadProgress, 
        [](qint64 bytesSent, qint64 bytesTotal) {
            qDebug() << "Upload progress:" << bytesSent << "/" << bytesTotal;
        });
    
    QObject::connect(reply, &QNetworkReply::downloadProgress, 
        [](qint64 bytesReceived, qint64 bytesTotal) {
            qDebug() << "Download progress:" << bytesReceived << "/" << bytesTotal;
        });
    
    // For now, use a synchronous approach to handle the reply
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    // Add a timeout to the event loop
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000);  // 15 second timeout
    
    loop.exec();
    
    // Check if the request timed out
    if (timer.isActive()) {
        // Timer is still active, meaning the request completed before timeout
        timer.stop();
        
        // Handle the response
        this->handleNetworkReply(reply, responseCallback);
    } else {
        // Timer expired, meaning the request timed out
        reply->abort();
        QString errorMessage = QStringLiteral("Request to OpenAI API timed out after 15 seconds.");
        qWarning() << "OpenAI API request failed:" << errorMessage;
        responseCallback(errorMessage, true);
    }
    
    reply->deleteLater();
}
// Improved handleNetworkReply method with better error reporting
void OpenAIProvider::handleNetworkReply(QNetworkReply *reply, std::function<void(const QString&, bool)> callback)
{
    if (reply->error() != QNetworkReply::NoError) {
        // Read the response data even in case of error, as it might contain useful information
        QByteArray responseData = reply->readAll();
        
        // Create a detailed error message
        QString errorMessage = QStringLiteral("Network error: %1").arg(reply->errorString());
        
        // Add HTTP status code if available
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatus > 0) {
            errorMessage += QStringLiteral(" (HTTP status: %1)").arg(httpStatus);
        }
        
        // Try to parse the response as JSON for more error details
        QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
        if (!jsonResponse.isNull() && jsonResponse.isObject()) {
            QJsonObject obj = jsonResponse.object();
            if (obj.contains(QStringLiteral("error"))) {
                QJsonObject error = obj[QStringLiteral("error")].toObject();
                if (error.contains(QStringLiteral("message"))) {
                    errorMessage += QStringLiteral("\nAPI Error: %1").arg(error[QStringLiteral("message")].toString());
                }
                if (error.contains(QStringLiteral("type"))) {
                    errorMessage += QStringLiteral(" (Type: %1)").arg(error[QStringLiteral("type")].toString());
                }
            }
        } else if (!responseData.isEmpty()) {
            // If not JSON, add the raw response if it's not empty
            QString responseText = QString::fromUtf8(responseData);
            if (responseText.length() > 100) {
                responseText = responseText.left(100) + QStringLiteral("...");
            }
            errorMessage += QStringLiteral("\nServer response: %1").arg(responseText);
        }
        
        qWarning() << "OpenAI API request failed:" << errorMessage;
        callback(errorMessage, true);
        return;
    }
    
    // Read the response
    QByteArray responseData = reply->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
    
    if (jsonResponse.isNull() || !jsonResponse.isObject()) {
        // Handle invalid JSON
        QString errorMessage = QStringLiteral("Invalid response from OpenAI API.");
        if (!responseData.isEmpty()) {
            QString responseText = QString::fromUtf8(responseData);
            if (responseText.length() > 100) {
                responseText = responseText.left(100) + QStringLiteral("...");
            }
            errorMessage += QStringLiteral(" Response: %1").arg(responseText);
        }
        qWarning() << errorMessage;
        callback(errorMessage, true);
        return;
    }
    
    QJsonObject jsonObject = jsonResponse.object();
    
    // Check for errors
    if (jsonObject.contains(QStringLiteral("error"))) {
        QString errorMessage = formatErrorMessage(jsonObject[QStringLiteral("error")].toObject());
        qWarning() << "OpenAI API error:" << errorMessage;
        callback(errorMessage, true);
        return;
    }
    
    // Extract the response content
    QString content = extractContentFromResponse(jsonObject);
    
    // If no content was found, report an error
    if (content.isEmpty()) {
        QString errorMessage = QStringLiteral("No response content found in OpenAI API response.");
        qWarning() << errorMessage;
        callback(errorMessage, true);
        return;
    }
    
    // Success! Return the content
    callback(content, true);
}

QJsonObject OpenAIProvider::createRequestPayload(const QString &query, const QString &contextInfo)
{
    QJsonObject payload;
    
    // Set the model
    payload[QStringLiteral("model")] = m_model;
    
    // Set parameters
    payload[QStringLiteral("temperature")] = m_temperature;
    payload[QStringLiteral("max_tokens")] = m_maxTokens;
    
    // Create messages array
    QJsonArray messages;
    
    // Add system message with context if available
    if (!contextInfo.isEmpty()) {
        QJsonObject systemMessage;
        systemMessage[QStringLiteral("role")] = QStringLiteral("system");
        systemMessage[QStringLiteral("content")] = formatSystemMessage(contextInfo);
        messages.append(systemMessage);
    }
    
    // Add user message
    QJsonObject userMessage;
    userMessage[QStringLiteral("role")] = QStringLiteral("user");
    userMessage[QStringLiteral("content")] = query;
    messages.append(userMessage);
    
    payload[QStringLiteral("messages")] = messages;
    
    return payload;
}

QString OpenAIProvider::extractContentFromResponse(const QJsonObject &jsonResponse)
{
    // Navigate the JSON structure to find the content
    if (!jsonResponse.contains(QStringLiteral("choices")) || !jsonResponse[QStringLiteral("choices")].isArray()) {
        return QString();
    }
    
    QJsonArray choices = jsonResponse[QStringLiteral("choices")].toArray();
    if (choices.isEmpty()) {
        return QString();
    }
    
    QJsonObject firstChoice = choices.at(0).toObject();
    if (!firstChoice.contains(QStringLiteral("message")) || !firstChoice[QStringLiteral("message")].isObject()) {
        return QString();
    }
    
    QJsonObject message = firstChoice[QStringLiteral("message")].toObject();
    if (!message.contains(QStringLiteral("content")) || !message[QStringLiteral("content")].isString()) {
        return QString();
    }
    
    return message[QStringLiteral("content")].toString();
}

QString OpenAIProvider::formatErrorMessage(const QJsonObject &errorData)
{
    QString errorMessage = QStringLiteral("OpenAI API Error: ");
    
    if (errorData.contains(QStringLiteral("message")) && errorData[QStringLiteral("message")].isString()) {
        errorMessage += errorData[QStringLiteral("message")].toString();
    } else {
        errorMessage += QStringLiteral("Unknown error");
    }
    
    if (errorData.contains(QStringLiteral("type")) && errorData[QStringLiteral("type")].isString()) {
        errorMessage += QStringLiteral(" (Type: ") + errorData[QStringLiteral("type")].toString() + QStringLiteral(")");
    }
    
    return errorMessage;
}

QString OpenAIProvider::formatSystemMessage(const QString &contextInfo)
{
    // Create a system message that includes:
    // 1. Basic instructions for the assistant
    // 2. Context information provided
    // 3. Guidelines for response formatting
    
    return QStringLiteral(
        "You are an AI assistant integrated into the Kate text editor through the WarpKate plugin. "
        "You help users with coding, text editing, terminal commands, and other technical tasks. "
        "Please provide concise, helpful responses.\n\n"
        "CONTEXT INFORMATION:\n%1\n\n"
        "When providing code, use appropriate markdown formatting. "
        "For multiple options or steps, use numbered lists. "
        "Keep explanations clear and focused on the user's needs."
    ).arg(contextInfo);
}

void OpenAIProvider::setApiKey(const QString &apiKey)
{
    // Only reinitialize if the API key has changed
    if (m_apiKey != apiKey) {
        m_apiKey = apiKey;
        initialize();
    }
}

void OpenAIProvider::setModelParameters(const QVariantMap &parameters)
{
    // Update model if present and valid
    if (parameters.contains(QStringLiteral("model"))) {
        QString modelName = parameters[QStringLiteral("model")].toString();
        if (SUPPORTED_MODELS.contains(modelName)) {
            m_model = modelName;
            qDebug() << "Model set to:" << m_model;
        } else {
            qWarning() << "Unsupported model:" << modelName << "Using default:" << m_model;
        }
    }
    
    // Update temperature if present (should be between 0.0 and 2.0)
    if (parameters.contains(QStringLiteral("temperature"))) {
        double temp = parameters[QStringLiteral("temperature")].toDouble();
        // Ensure temperature is within valid range
        if (temp >= 0.0 && temp <= 2.0) {
            m_temperature = temp;
            qDebug() << "Temperature set to:" << m_temperature;
        } else {
            qWarning() << "Invalid temperature value:" << temp << "Valid range is 0.0-2.0";
        }
    }
    
    // Update max tokens if present (should be > 0)
    if (parameters.contains(QStringLiteral("max_tokens"))) {
        int tokens = parameters[QStringLiteral("max_tokens")].toInt();
        if (tokens > 0) {
            m_maxTokens = tokens;
            qDebug() << "Max tokens set to:" << m_maxTokens;
        } else {
            qWarning() << "Invalid max_tokens value:" << tokens << "Value must be > 0";
        }
    }
}

QStringList OpenAIProvider::availableModels() const
{
    return SUPPORTED_MODELS;
}
