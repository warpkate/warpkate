/*
 *  SPDX-FileCopyrightText: 2025 WarpKate Team <warpkate@example.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "aiprovider.h"

#include <QDebug>

// Forward declarations of provider implementations
// We'll implement these in separate files
class OpenAIProvider;
class AnthropicProvider;
class LocalProvider;
class CustomProvider;

// Include provider headers as they're implemented
#include "openai_provider.h"
// #include "anthropic_provider.h" // Uncomment when implemented
// #include "local_provider.h"     // Uncomment when implemented
// #include "custom_provider.h"    // Uncomment when implemented

AIServiceProvider* AIServiceProviderFactory::createProvider(AIProviderType type)
{
    switch (type) {
    case AIProviderType::Local:
        // We'll implement LocalProvider later
        qDebug() << "LocalProvider not yet implemented, falling back to OpenAI";
        return new OpenAIProvider();
        
    case AIProviderType::Remote:
        // For now, Remote always means OpenAI
        // Later, we can extend this to check a secondary option for Anthropic
        return new OpenAIProvider();
        
    case AIProviderType::CustomAPI:
        // We'll implement CustomProvider later
        qDebug() << "CustomProvider not yet implemented, falling back to OpenAI";
        return new OpenAIProvider();
        
    default:
        qWarning() << "Unknown provider type, falling back to OpenAI";
        return new OpenAIProvider();
    }
}

