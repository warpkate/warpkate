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
        responseCallback(QStringLiteral("Request to OpenAI API timed out after 15 seconds."), true);
    }
    
    reply->deleteLater();
}
