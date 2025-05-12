#!/bin/bash
export QT_DEBUG_PLUGINS=1
export QT_LOGGING_RULES="*.debug=true"
kate --new-window > kate_debug.log 2>&1
echo "Debug log written to kate_debug.log"
