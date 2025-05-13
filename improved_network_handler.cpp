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
