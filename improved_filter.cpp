QString WarpKateView::cleanTerminalOutput(const QString &rawOutput)
{
    // More comprehensive regular expressions for filtering terminal output
    
    // ANSI escape sequences (including more specific patterns)
    static QRegularExpression ansiEscapeRE(QStringLiteral("\033\\[[0-9;]*[a-zA-Z]"));
    
    // Terminal title sequences and similar OSC sequences
    static QRegularExpression oscSequenceRE(QStringLiteral("\033\\][0-9].*;.*(\007|\033\\\\)"));
    
    // Terminal status sequences like [?2004h and [?2004l
    static QRegularExpression termStatusRE(QStringLiteral("\\[\\?[0-9;]*[a-zA-Z]"));
    
    // Simple terminal prompts like ]0;user@host:path
    static QRegularExpression termPromptRE(QStringLiteral("\\][0-9];[^\007]*"));
    
    // Control characters (excluding newlines and tabs which we want to preserve)
    static QRegularExpression controlCharsRE(QStringLiteral("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F]"));
    
    // Bell character
    static QRegularExpression bellRE(QStringLiteral("\007"));
    
    // Start cleaning the output
    QString cleaned = rawOutput;
    
    // Log the original and cleaned output for debugging
    qDebug() << "Original terminal output:" << (rawOutput.length() > 50 ? rawOutput.left(50) + QStringLiteral("...") : rawOutput);
    
    // Apply all the filters
    cleaned = cleaned.replace(ansiEscapeRE, QStringLiteral(""));
    cleaned = cleaned.replace(oscSequenceRE, QStringLiteral(""));
    cleaned = cleaned.replace(termStatusRE, QStringLiteral(""));
    cleaned = cleaned.replace(termPromptRE, QStringLiteral(""));
    cleaned = cleaned.replace(controlCharsRE, QStringLiteral(""));
    cleaned = cleaned.replace(bellRE, QStringLiteral(""));
    
    // Additional cleanup for common escape sequences in bash/zsh
    cleaned = cleaned.replace(QStringLiteral("\\]0;"), QStringLiteral(""));
    
    // Log the cleaned output for debugging
    qDebug() << "Cleaned terminal output:" << (cleaned.length() > 50 ? cleaned.left(50) + QStringLiteral("...") : cleaned);
    
    return cleaned;
}
