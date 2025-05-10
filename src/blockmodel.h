/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BLOCKMODEL_H
#define BLOCKMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

class TerminalEmulator;

/**
 * Block execution state
 */
enum BlockState {
    Pending,        ///< Block has been created but command not yet executed
    Executing,      ///< Command is currently executing
    Completed,      ///< Command completed successfully
    Failed          ///< Command failed (non-zero exit code)
};

/**
 * Command Block structure
 * Represents a single command and its output
 */
struct CommandBlock {
    int id;                             ///< Unique block identifier
    QString command;                    ///< The executed command
    QString output;                     ///< Command output
    QDateTime startTime;                ///< Command execution start time
    QDateTime endTime;                  ///< Command execution end time
    int exitCode;                       ///< Command exit code
    BlockState state;                   ///< Block state
    QString workingDirectory;           ///< Working directory for this command
    
    /**
     * Constructor
     */
    CommandBlock() 
        : id(0)
        , exitCode(0)
        , state(Pending) 
    {}
    
    /**
     * Parameterized constructor
     */
    CommandBlock(int blockId, const QString &cmd, const QString &dir = QString())
        : id(blockId)
        , command(cmd)
        , exitCode(0)
        , state(Pending)
        , workingDirectory(dir) 
    {}
    
    /**
     * Get the duration of command execution
     * @return Duration in milliseconds, or -1 if still executing
     */
    qint64 duration() const {
        if (state == Executing || !startTime.isValid()) {
            return -1;
        }
        
        QDateTime end = endTime.isValid() ? endTime : QDateTime::currentDateTime();
        return startTime.msecsTo(end);
    }
    
    /**
     * Check if this block contains a valid command
     * @return true if the block has a non-empty command
     */
    bool isValid() const {
        return !command.isEmpty();
    }
};

/**
 * Block model roles used for item views
 */
enum BlockModelRoles {
    IdRole = Qt::UserRole + 1,         ///< Block ID
    CommandRole,                        ///< Command string
    OutputRole,                         ///< Command output
    StateRole,                          ///< Block state
    StartTimeRole,                      ///< Command start time
    EndTimeRole,                        ///< Command end time
    ExitCodeRole,                       ///< Command exit code
    DurationRole,                       ///< Command execution duration
    WorkingDirectoryRole,               ///< Working directory
    IsCurrentRole                       ///< Whether this is the current block
};

/**
 * Block model class
 * 
 * This class manages command blocks and their states. It implements the
 * Qt model/view framework to make it easy to display blocks in views.
 * It integrates with the terminal emulator to capture commands and output.
 */
class BlockModel : public QAbstractListModel
{
    Q_OBJECT
    
public:
    /**
     * Constructor
     * @param parent Parent object
     */
    explicit BlockModel(QObject *parent = nullptr);
    
    /**
     * Destructor
     */
    ~BlockModel() override;
    
    /**
     * Connect to a terminal emulator
     * @param terminal The terminal emulator to connect to
     */
    void connectToTerminal(TerminalEmulator *terminal);
    
    /**
     * Get data for a model index
     * @param index The model index
     * @param role The data role
     * @return The requested data
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    
    /**
     * Get the number of rows in the model
     * @param parent The parent index (unused in list models)
     * @return The number of blocks
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    
    /**
     * Get the role names for QML
     * @return Hash mapping role enum values to role names
     */
    QHash<int, QByteArray> roleNames() const override;
    
    /**
     * Create a new command block
     * @param command The command to execute
     * @param workingDirectory Working directory for the command
     * @return ID of the created block
     */
    int createBlock(const QString &command, const QString &workingDirectory = QString());
    
    /**
     * Get a block by its ID
     * @param id Block ID
     * @return The block, or an invalid block if not found
     */
    CommandBlock blockById(int id) const;
    
    /**
     * Get the current block ID
     * @return ID of the currently active block
     */
    int currentBlockId() const;
    
    /**
     * Set the current block
     * @param id Block ID to set as current
     * @return True if successful
     */
    bool setCurrentBlock(int id);
    
    /**
     * Navigate to the next block
     * @return True if successful
     */
    bool navigateToNextBlock();
    
    /**
     * Navigate to the previous block
     * @return True if successful
     */
    bool navigateToPreviousBlock();
    
    /**
     * Execute a command in a new block
     * @param command Command to execute
     * @param workingDirectory Working directory for the command
     * @return ID of the created block
     */
    int executeCommand(const QString &command, const QString &workingDirectory = QString());
    
    /**
     * Set a block's command
     * @param id Block ID
     * @param command New command text
     * @return True if successful
     */
    bool setBlockCommand(int id, const QString &command);
    
    /**
     * Append output to a block
     * @param id Block ID
     * @param output Output text to append
     * @return True if successful
     */
    bool appendBlockOutput(int id, const QString &output);
    
    /**
     * Set a block's state
     * @param id Block ID
     * @param state New block state
     * @return True if successful
     */
    bool setBlockState(int id, BlockState state);
    
    /**
     * Set a block's exit code
     * @param id Block ID
     * @param exitCode Command exit code
     * @return True if successful
     */
    bool setBlockExitCode(int id, int exitCode);
    
    /**
     * Set a block's start time
     * @param id Block ID
     * @param startTime Command start time
     * @return True if successful
     */
    bool setBlockStartTime(int id, const QDateTime &startTime);
    
    /**
     * Set a block's end time
     * @param id Block ID
     * @param endTime Command end time
     * @return True if successful
     */
    bool setBlockEndTime(int id, const QDateTime &endTime);
    
    /**
     * Get all blocks
     * @return List of all command blocks
     */
    QList<CommandBlock> blocks() const;
    
    /**
     * Clear all blocks
     */
    void clear();
    
    /**
     * Get the index of a block by ID
     * @param id Block ID
     * @return Model index for the block
     */
    QModelIndex indexForBlock(int id) const;
    
    /**
     * Find a block containing a specific text
     * @param text Text to search for
     * @param startFrom Block ID to start search from
     * @param searchForward Direction to search
     * @return ID of the found block, or -1 if not found
     */
    int findText(const QString &text, int startFrom = -1, bool searchForward = true) const;
    
public Q_SLOTS:
    /**
     * Handle a new command being entered
     * @param command Command text
     */
    void onCommandDetected(const QString &command);
    
    /**
     * Handle a command completing execution
     * @param command Command text
     * @param output Command output
     * @param exitCode Command exit code
     */
    void onCommandExecuted(const QString &command, const QString &output, int exitCode);
    
    /**
     * Handle terminal output being available
     * @param output New output text
     */
    void onOutputAvailable(const QString &output);
    
    /**
     * Handle working directory changes
     * @param directory New working directory
     */
    void onWorkingDirectoryChanged(const QString &directory);
    
    /**
     * Handle terminal process finishing
     * @param exitCode Exit code of the shell process
     */
    void onShellFinished(int exitCode);
    
Q_SIGNALS:
    /**
     * Emitted when the current block changes
     * @param currentId ID of the current block
     */
    void currentBlockChanged(int currentId);
    
    /**
     * Emitted when a block is created
     * @param id ID of the new block
     */
    void blockCreated(int id);
    
    /**
     * Emitted when a block's state changes
     * @param id Block ID
     * @param state New state
     */
    void blockStateChanged(int id, BlockState state);
    
    /**
     * Emitted when a block's content changes
     * @param id Block ID
     */
    void blockChanged(int id);
    
private:
    /**
     * Find the index of a block by ID
     * @param id Block ID
     * @return Index in the blocks list, or -1 if not found
     */
    int findBlockIndex(int id) const;
    
    /**
     * Generate a new unique block ID
     * @return New block ID
     */
    int generateBlockId() const;
    
    /**
     * Update a block's metadata from a model index
     * @param index Model index to update
     */
    void updateBlockMetadata(const QModelIndex &index);
    
private:
    QList<CommandBlock> m_blocks;                   ///< List of all blocks
    int m_currentBlockId;                           ///< ID of the current block
    int m_nextBlockId;                              ///< Next ID to use
    TerminalEmulator *m_terminal;                   ///< Connected terminal emulator
    QString m_currentWorkingDirectory;              ///< Current working directory
    bool m_isCommandExecuting;                      ///< Whether a command is currently executing
    QString m_currentOutput;                        ///< Current accumulated output
};

#endif // BLOCKMODEL_H

