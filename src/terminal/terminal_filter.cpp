QString WarpKateView::cleanTerminalOutput(const QString &rawOutput)
{
    // Regular expressions for filtering terminal output
    // These are language-neutral patterns that apply to all locales
    
    // ANSI escape sequences (color codes, cursor movement, etc.)
    static QRegularExpression ansiEscapeRE(QStringLiteral("\033\\[[0-9;]*[a-zA-Z]"));
    
    // Control characters (excluding newlines and tabs which we want to preserve)
    static QRegularExpression controlCharsRE(QStringLiteral("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F]"));
    
    // Bell character
    static QRegularExpression bellRE(QStringLiteral("\007"));
    
    // Other terminal control sequences
    static QRegularExpression otherControlSeqRE(QStringLiteral("\033\\][0-9];[^\007]*\007"));  // OSC sequences
    
    // Start cleaning the output
    QString cleaned = rawOutput;
    
    // Remove ANSI escape sequences
    cleaned = cleaned.replace(ansiEscapeRE, QStringLiteral(""));
    
    // Remove control characters (except tabs and newlines)
    cleaned = cleaned.replace(controlCharsRE, QStringLiteral(""));
    
    // Remove bell character
    cleaned = cleaned.replace(bellRE, QStringLiteral(""));
    
    // Remove other terminal control sequences
    cleaned = cleaned.replace(otherControlSeqRE, QStringLiteral(""));
    
    return cleaned;
}
