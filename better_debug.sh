#!/bin/bash
export QT_DEBUG_PLUGINS=1
export QT_LOGGING_RULES="*.debug=true;qt.*.debug=true;kf.*.debug=true"
export QT_MESSAGE_PATTERN="[%{time h:mm:ss.zzz}] %{if-category}%{category}: %{endif}%{message}"

# Kill any running Kate instances first
echo "Killing any running Kate instances..."
killall kate 2>/dev/null

echo "Starting Kate with debug logging..."
kate --new-window > kate_full_debug.log 2>&1

echo "Debug log written to kate_full_debug.log"
