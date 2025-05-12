/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "openai_provider.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QEventLoop>

// Static constants initialization
const QString OpenAIProvider::DEFAULT_MODEL = "gpt-3.5-turbo";
const QStringList OpenAIProvider::SUPPORTED_MODELS = {
    "gpt-3.5-turbo",
    "gpt-3.5-turbo-16k",
    "gpt-4",
    "gpt-4-turbo",
    "gpt-4-32k"
};

OpenAIProvider::OpenAIProvider()
    : m_apiEndpoint("https://api.openai.com/v1/chat/completions")
    , m_model(DEFAULT_MODEL)
    , m_temperature(0.7)
    , m_maxTokens(1000)
    , m_initialized(false)
{
    // Set up network connections
    connect(&m_networkManager, &QNetworkAccessManager::sslErrors, 
            this, [](QNetworkReply* reply, const QList<QSslError>& errors) {
        qWarning() << "SSL errors in OpenAI provider:" << errors;
        reply->ignoreSslErrors(); // In production, handle this more carefully
    });
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

void OpenAIProvider::generateResponse(
    const QString &query, 
    const QString &contextInfo,
    std::function<void(const QString&, bool)> responseCallback
)
{
    if (!isInitialized()) {
        responseCallback("Error: OpenAI provider not initialized. Please set API key.", true);
        return;
    }
    
    // Create the network request
    QNetworkRequest request(QUrl(m_apiEndpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    
    // Create the JSON payload
    QJsonObject payload = createRequestPayload(query, contextInfo);
    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson();
    
    qDebug() << "Sending request to OpenAI API...";
    
    // Send the POST request
    QNetworkReply *reply = m_networkManager.post(request, jsonData);
    
    // Handle the response asynchronously
    connect(reply, &QNetworkReply::finished, this, [this, reply, responseCallback]() {
        this->handleNetworkReply(reply, responseCallback);
        reply->deleteLater();
    });
}

void OpenAIProvider::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
    m_initialized = !m_apiKey.isEmpty();
}

void OpenAIProvider::setModelParameters(const QVariantMap &parameters)
{
    // Set model if provided
    if (parameters.contains("model")) {
        QString model = parameters["model"].toString();
        if (SUPPORTED_MODELS.contains(model)) {
            m_model = model;
        } else {
            qWarning() << "Unsupported model:" << model << "Falling back to:" << m_model;
        }
    }
    
    // Set temperature if provided
    if (parameters.contains("temperature")) {
        m_temperature = parameters["temperature"].toDouble();
    }
    
    // Set max tokens if provided
    if (parameters.contains("maxTokens")) {
        m_maxTokens = parameters["maxTokens"].toInt();
    }
}

QStringList OpenAIProvider::availableModels() const
{
    return SUPPORTED_MODELS;
}

void OpenAIProvider::handleNetworkReply(QNetworkReply *reply, std::function<void(const QString&, bool)> callback)
{
    if (reply->error() != QNetworkReply::NoError) {
        // Handle network error
        QString errorMessage = QString("Network error: %1").arg(reply->errorString());
        qWarning() << "OpenAI API request failed:" << errorMessage;
        callback(errorMessage, true);
        return;
    }
    
    // Read the response
    QByteArray responseData = reply->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
    
    if (jsonResponse.isNull() || !jsonResponse.isObject()) {
        // Handle invalid JSON
        QString errorMessage = "Invalid response from OpenAI API.";
        qWarning() << errorMessage << "Response:" << responseData;
        callback(errorMessage, true);
        return;
    }
    
    QJsonObject jsonObject = jsonResponse.object();
    
    // Check for errors
    if (jsonObject.contains("error")) {
        QString errorMessage = formatErrorMessage(jsonObject["error"].toObject());
        qWarning() << "OpenAI API error:" << errorMessage;
        callback(errorMessage, true);
        return;
    }
    
    // Extract the response content
    QString content = extractContentFromResponse(jsonObject);
    
    // If no content was found, report an error
    if (content.isEmpty()) {
        QString errorMessage = "No response content found in OpenAI API response.";
        qWarning() << errorMessage;
        callback(errorMessage, true);
        return;
    }
    
    // Return the content through the callback
    callback(content, true);
}

QJsonObject OpenAIProvider::createRequestPayload(const QString &query, const QString &contextInfo)
{
    QJsonObject payload;
    
    // Set the model
    payload["model"] = m_model;
    
    // Set parameters
    payload["temperature"] = m_temperature;
    payload["max_tokens"] = m_maxTokens;
    
    // Create messages array
    QJsonArray messages;
    
    // Add system message with context if available
    if (!contextInfo.isEmpty()) {
        QJsonObject systemMessage;
        systemMessage["role"] = "system";
        systemMessage["content"] = formatSystemMessage(contextInfo);
        messages.append(systemMessage);
    }
    
    // Add user message
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = query;
    messages.append(userMessage);
    
    payload["messages"] = messages;
    
    return payload;
}

QString OpenAIProvider::extractContentFromResponse(const QJsonObject &jsonResponse)
{
    // Navigate the JSON structure to find the content
    if (!jsonResponse.contains("choices") || !jsonResponse["choices"].isArray()) {
        return QString();
    }
    
    QJsonArray choices = jsonResponse["choices"].toArray();
    if (choices.isEmpty()) {
        return QString();
    }
    
    QJsonObject firstChoice = choices.at(0).toObject();
    if (!firstChoice.contains("message") || !firstChoice["message"].isObject()) {
        return QString();
    }
    
    QJsonObject message = firstChoice["message"].toObject();
    if (!message.contains("content") || !message["content"].isString()) {
        return QString();
    }
    
    return message["content"].toString();
}

QString OpenAIProvider::formatErrorMessage(const QJsonObject &errorData)
{
    QString errorMessage = "OpenAI API Error: ";
    
    if (errorData.contains("message") && errorData["message"].isString()) {
        errorMessage += errorData["message"].toString();
    } else {
        errorMessage += "Unknown error";
    }
    
    if (errorData.contains("type") && errorData["type"].isString()) {
        errorMessage += " (Type: " + errorData["type"].toString() + ")";
    }
    
    return errorMessage;
}

QString OpenAIProvider::formatSystemMessage(const QString &contextInfo)
{
    // Create a system message that includes:
    // 1. Basic instructions for the assistant
    // 2. Context information provided
    // 3. Guidelines for response formatting
    
    return QString(
        "You are an AI assistant integrated into the Kate text editor through the WarpKate plugin. "
        "You help users with coding, text editing, terminal commands, and other technical tasks. "
        "Please provide concise, helpful responses.\n\n"
        "CONTEXT INFORMATION:\n%1\n\n"
        "When providing code, use appropriate markdown formatting. "
        "For multiple options or steps, use numbered lists. "
        "Keep explanations clear and focused on the user's needs."
    ).arg(contextInfo);
}

