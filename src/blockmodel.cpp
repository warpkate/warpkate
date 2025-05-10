/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "blockmodel.h"
#include "terminalemulator.h"

#include <QDebug>

BlockModel::BlockModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_currentBlockId(-1)
    , m_nextBlockId(1)
    , m_terminal(nullptr)
    , m_isCommandExecuting(false)
{
}

BlockModel::~BlockModel()
{
}

void BlockModel::connectToTerminal(TerminalEmulator *terminal)
{
    if (m_terminal == terminal) {
        return;
    }
    
    // Disconnect from old terminal if any
    if (m_terminal) {
        disconnect(m_terminal, nullptr, this, nullptr);
    }
    
    m_terminal = terminal;
    
    if (m_terminal) {
        // Connect signals from terminal to this model
        connect(m_terminal, &TerminalEmulator::commandDetected, this, &BlockModel::onCommandDetected);
        connect(m_terminal, &TerminalEmulator::commandExecuted, this, &BlockModel::onCommandExecuted);
        connect(m_terminal, &TerminalEmulator::outputAvailable, this, &BlockModel::onOutputAvailable);
        connect(m_terminal, &TerminalEmulator::workingDirectoryChanged, this, &BlockModel::onWorkingDirectoryChanged);
        connect(m_terminal, &TerminalEmulator::shellFinished, this, &BlockModel::onShellFinished);
    }
}

QVariant BlockModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_blocks.size()) {
        return QVariant();
    }
    
    const CommandBlock &block = m_blocks[index.row()];
    
    switch (role) {
        case Qt::DisplayRole:
            return block.command;
            
        case IdRole:
            return block.id;
            
        case CommandRole:
            return block.command;
            
        case OutputRole:
            return block.output;
            
        case StateRole:
            return block.state;
            
        case StartTimeRole:
            return block.startTime;
            
        case EndTimeRole:
            return block.endTime;
            
        case ExitCodeRole:
            return block.exitCode;
            
        case DurationRole:
            return block.duration();
            
        case WorkingDirectoryRole:
            return block.workingDirectory;
            
        case IsCurrentRole:
            return (block.id == m_currentBlockId);
    }
    
    return QVariant();
}

int BlockModel::rowCount(const QModelIndex &parent) const
{
    // For list models, the root node (with invalid parent) returns the list size
    if (parent.isValid()) {
        return 0;
    }
    
    return m_blocks.size();
}

QHash<int, QByteArray> BlockModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "blockId";
    roles[CommandRole] = "command";
    roles[OutputRole] = "output";
    roles[StateRole] = "state";
    roles[StartTimeRole] = "startTime";
    roles[EndTimeRole] = "endTime";
    roles[ExitCodeRole] = "exitCode";
    roles[DurationRole] = "duration";
    roles[WorkingDirectoryRole] = "workingDirectory";
    roles[IsCurrentRole] = "isCurrent";
    return roles;
}

int BlockModel::createBlock(const QString &command, const QString &workingDirectory)
{
    // Generate a new block ID
    int blockId = generateBlockId();
    
    // Create the block
    CommandBlock block(blockId, command, workingDirectory.isEmpty() ? m_currentWorkingDirectory : workingDirectory);
    
    // Add to model
    beginInsertRows(QModelIndex(), m_blocks.size(), m_blocks.size());
    m_blocks.append(block);
    endInsertRows();
    
    // Set as current if this is the first block
    if (m_currentBlockId < 0) {
        setCurrentBlock(blockId);
    }
    
    // Emit signal
    emit blockCreated(blockId);
    
    return blockId;
}

CommandBlock BlockModel::blockById(int id) const
{
    int index = findBlockIndex(id);
    if (index >= 0) {
        return m_blocks[index];
    }
    
    // Return an invalid block
    return CommandBlock();
}

int BlockModel::currentBlockId() const
{
    return m_currentBlockId;
}

bool BlockModel::setCurrentBlock(int id)
{
    if (id == m_currentBlockId) {
        return true;
    }
    
    // Find the block
    int oldIndex = findBlockIndex(m_currentBlockId);
    int newIndex = findBlockIndex(id);
    
    if (newIndex < 0) {
        return false;
    }
    
    // Update the current block ID
    m_currentBlockId = id;
    
    // Update the old and new blocks to reflect current status
    if (oldIndex >= 0) {
        QModelIndex oldModelIndex = index(oldIndex, 0);
        emit dataChanged(oldModelIndex, oldModelIndex, {IsCurrentRole});
    }
    
    QModelIndex newModelIndex = index(newIndex, 0);
    emit dataChanged(newModelIndex, newModelIndex, {IsCurrentRole});
    
    // Emit signal
    emit currentBlockChanged(id);
    
    return true;
}

bool BlockModel::navigateToNextBlock()
{
    int currentIndex = findBlockIndex(m_currentBlockId);
    
    // If no current block or at the end, can't navigate next
    if (currentIndex < 0 || currentIndex >= m_blocks.size() - 1) {
        return false;
    }
    
    // Get the next block ID
    int nextId = m_blocks[currentIndex + 1].id;
    
    // Set as current
    return setCurrentBlock(nextId);
}

bool BlockModel::navigateToPreviousBlock()
{
    int currentIndex = findBlockIndex(m_currentBlockId);
    
    // If no current block or at the beginning, can't navigate previous
    if (currentIndex <= 0) {
        return false;
    }
    
    // Get the previous block ID
    int prevId = m_blocks[currentIndex - 1].id;
    
    // Set as current
    return setCurrentBlock(prevId);
}

int BlockModel::executeCommand(const QString &command, const QString &workingDirectory)
{
    // Don't execute if terminal is not available
    if (!m_terminal) {
        return -1;
    }
    
    // Create a new block for the command
    int blockId = createBlock(command, workingDirectory);
    
    // Start execution
    setBlockState(blockId, Executing);
    setBlockStartTime(blockId, QDateTime::currentDateTime());
    
    // Execute in terminal
    m_terminal->executeCommand(command);
    
    return blockId;
}

bool BlockModel::setBlockCommand(int id, const QString &command)
{
    int index = findBlockIndex(id);
    if (index < 0) {
        return false;
    }
    
    // Update command
    m_blocks[index].command = command;
    
    // Notify views
    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {CommandRole});
    emit blockChanged(id);
    
    return true;
}

bool BlockModel::appendBlockOutput(int id, const QString &output)
{
    int index = findBlockIndex(id);
    if (index < 0) {
        return false;
    }
    
    // Append output
    m_blocks[index].output.append(output);
    
    // Notify views
    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {OutputRole});
    emit blockChanged(id);
    
    return true;
}

bool BlockModel::setBlockState(int id, BlockState state)
{
    int index = findBlockIndex(id);
    if (index < 0) {
        return false;
    }
    
    // Update state
    m_blocks[index].state = state;
    
    // Notify views
    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {StateRole});
    emit blockStateChanged(id, state);
    emit blockChanged(id);
    
    return true;
}

bool BlockModel::setBlockExitCode(int id, int exitCode)
{
    int index = findBlockIndex(id);
    if (index < 0) {
        return false;
    }
    
    // Update exit code
    m_blocks[index].exitCode = exitCode;
    
    // Notify views
    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {ExitCodeRole});
    emit blockChanged(id);
    
    return true;
}

bool BlockModel::setBlockStartTime(int id, const QDateTime &startTime)
{
    int index = findBlockIndex(id);
    if (index < 0) {
        return false;
    }
    
    // Update start time
    m_blocks[index].startTime = startTime;
    
    // Notify views
    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {StartTimeRole, DurationRole});
    emit blockChanged(id);
    
    return true;
}

bool BlockModel::setBlockEndTime(int id, const QDateTime &endTime)
{
    int index = findBlockIndex(id);
    if (index < 0) {
        return false;
    }
    
    // Update end time
    m_blocks[index].endTime = endTime;
    
    // Notify views
    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {EndTimeRole, DurationRole});
    emit blockChanged(id);
    
    return true;
}

QList<CommandBlock> BlockModel::blocks() const
{
    return m_blocks;
}

void BlockModel::clear()
{
    if (m_blocks.isEmpty()) {
        return;
    }
    
    // Remove all blocks
    beginRemoveRows(QModelIndex(), 0, m_blocks.size() - 1);
    m_blocks.clear();
    endRemoveRows();
    
    // Reset current block
    m_currentBlockId = -1;
    
    // Reset next ID
    m_nextBlockId = 1;
}

QModelIndex BlockModel::indexForBlock(int id) const
{
    int index = findBlockIndex(id);
    if (index >= 0) {
        return this->index(index, 0);
    }
    
    return QModelIndex();
}

int BlockModel::findText(const QString &text, int startFrom, bool searchForward) const
{
    if (m_blocks.isEmpty() || text.isEmpty()) {
        return -1;
    }
    
    // Start from current block if not specified
    if (startFrom < 0) {
        startFrom = m_currentBlockId;
    }
    
    int startIndex = findBlockIndex(startFrom);
    if (startIndex < 0) {
        // If invalid starting block, use first or last block depending on direction
        startIndex = searchForward ? 0 : m_blocks.size() - 1;
    }
    
    // Search for text in blocks
    if (searchForward) {
        // Search forward from start index
        for (int i = startIndex; i < m_blocks.size(); ++i) {
            if (m_blocks[i].command.contains(text, Qt::CaseInsensitive) ||
                m_blocks[i].output.contains(text, Qt::CaseInsensitive)) {
                return m_blocks[i].id;
            }
        }
        
        // If not found and we didn't start from the beginning, wrap around
        if (startIndex > 0) {
            for (int i = 0; i < startIndex; ++i) {
                if (m_blocks[i].command.contains(text, Qt::CaseInsensitive) ||
                    m_blocks[i].output.contains(text, Qt::CaseInsensitive)) {
                    return m_blocks[i].id;
                }
            }
        }
    } else {
        // Search backward from start index
        for (int i = startIndex; i >= 0; --i) {
            if (m_blocks[i].command.contains(text, Qt::CaseInsensitive) ||
                m_blocks[i].output.contains(text, Qt::CaseInsensitive)) {
                return m_blocks[i].id;
            }
        }
        
        // If not found and we didn't start from the end, wrap around
        if (startIndex < m_blocks.size() - 1) {
            for (int i = m_blocks.size() - 1; i > startIndex; --i) {
                if (m_blocks[i].command.contains(text, Qt::CaseInsensitive) ||
                    m_blocks[i].output.contains(text, Qt::CaseInsensitive)) {
                    return m_blocks[i].id;
                }
            }
        }
    }
    
    // Not found
    return -1;
}

void BlockModel::onCommandDetected(const QString &command)
{
    // Create a new block if command is valid
    if (!command.trimmed().isEmpty()) {
        int blockId = createBlock(command);
        setBlockState(blockId, Executing);
        setBlockStartTime(blockId, QDateTime::currentDateTime());
        
        // Update state
        m_isCommandExecuting = true;
        m_currentOutput.clear();
    }
}

void BlockModel::onCommandExecuted(const QString &command, const QString &output, int exitCode)
{
    // Find the most recent executing block
    int blockId = -1;
    for (int i = m_blocks.size() - 1; i >= 0; --i) {
        if (m_blocks[i].state == Executing && m_blocks[i].command == command) {
            blockId = m_blocks[i].id;
            break;
        }
    }
    
    if (blockId < 0) {
        // If no matching block found, create one
        blockId = createBlock(command);
    }
    
    // Update block with execution results
    setBlockOutput(blockId, output);
    setBlockExitCode(blockId, exitCode);
    setBlockEndTime(blockId, QDateTime::currentDateTime());
    setBlockState(blockId, exitCode == 0 ? Completed : Failed);
    
    // Update state
    m_isCommandExecuting = false;
    m_currentOutput.clear();
}

void BlockModel::onOutputAvailable(const QString &output)
{
    // Accumulate output
    m_currentOutput.append(output);
    
    // If a command is executing, append the output to the current block
    if (m_isCommandExecuting) {
        // Find the most recent executing block
        for (int i = m_blocks.size() - 1; i >= 0; --i) {
            if (m_blocks[i].state == Executing) {
                appendBlockOutput(m_blocks[i].id, output);
                break;
            }
        }
    }
}

void BlockModel::onWorkingDirectoryChanged(const QString &directory)
{
    // Update working directory
    m_currentWorkingDirectory = directory;
    
    // If a command is executing, update its working directory
    if (m_isCommandExecuting) {
        for (int i = m_blocks.size() - 1; i >= 0; --i) {
            if (m_blocks[i].state == Executing) {
                int index = i;
                m_blocks[index].workingDirectory = directory;
                
                // Notify views
                QModelIndex modelIndex = this->index(index, 0);
                emit dataChanged(modelIndex, modelIndex, {WorkingDirectoryRole});
                emit blockChanged(m_blocks[index].id);
                break;
            }
        }
    }
}

void BlockModel::onShellFinished(int exitCode)
{
    // If any commands are still executing, mark them as finished
    for (int i = 0; i < m_blocks.size(); ++i) {
        if (m_blocks[i].state == Executing) {
            setBlockState(m_blocks[i].id, exitCode == 0 ? Completed : Failed);
            setBlockExitCode(m_blocks[i].id, exitCode);
            setBlockEndTime(m_blocks[i].id, QDateTime::currentDateTime());
        }
    }
    
    // Reset command execution state
    m_isCommandExecuting = false;
    m_currentOutput.clear();
}

bool BlockModel::setBlockOutput(int id, const QString &output)
{
    int index = findBlockIndex(id);
    if (index < 0) {
        return false;
    }
    
    // Set output (replacing any existing output)
    m_blocks[index].output = output;
    
    // Notify views
    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {OutputRole});
    emit blockChanged(id);
    
    return true;
}

int BlockModel::findBlockIndex(int id) const
{
    // Linear search for the block with the given ID
    for (int i = 0; i < m_blocks.size(); ++i) {
        if (m_blocks[i].id == id) {
            return i;
        }
    }
    
    // Not found
    return -1;
}

int BlockModel::generateBlockId() const
{
    // Use the next available ID
    return m_nextBlockId++;
}

void BlockModel::updateBlockMetadata(const QModelIndex &index)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_blocks.size()) {
        return;
    }
    
    // Get the block
    CommandBlock &block = m_blocks[index.row()];
    
    // Update metadata based on the block's content
    // For now, this is a placeholder for future functionality
    // such as parsing command output for context, detecting errors, etc.
    
    // Notify that the block has changed
    emit blockChanged(block.id);
}

#include "moc_blockmodel.cpp"
