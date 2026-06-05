#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QVector>

struct LspServerCommand
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment;
};

struct LspInitializeResult
{
    QString serverName;
    QString serverVersion;
    bool textDocumentOpenClose = false;
    int textDocumentChange = 0;
    bool textDocumentSave = false;
    bool hoverProvider = false;
    bool completionProvider = false;
    bool semanticTokensProvider = false;
    bool documentSymbolProvider = false;
    bool workspaceSymbolProvider = false;
    QStringList semanticTokenTypes;
};

struct LspCompletionItem
{
    QString label;
    QString detail;
    QString documentation;
    QString insertText;
};

struct LspHoverResult
{
    QString markdown;
    bool hasContent = false;
};

struct LspLocation
{
    QString uri;
    int line = 0;
    int character = 0;
};

struct LspTextEdit
{
    QString uri;
    int startLine = 0;
    int startCharacter = 0;
    int endLine = 0;
    int endCharacter = 0;
    QString newText;
};

struct LspWorkspaceEdit
{
    QVector<LspTextEdit> edits;
};

struct LspSemanticToken
{
    int line = 0;
    int startCharacter = 0;
    int length = 0;
    QString tokenType;
};

struct LspSymbol
{
    QString name;
    QString detail;
    QString uri;
    int line = 0;
    int character = 0;
    int kind = 0;
};

class LspClient final : public QObject
{
    Q_OBJECT

public:
    explicit LspClient(QObject *parent = nullptr);
    ~LspClient() override;

    void setServerCommand(const LspServerCommand &command);
    LspServerCommand serverCommand() const;

    bool startAndInitialize(const QString &rootUri, int timeoutMs = 5000, QString *error = nullptr);
    bool isRunning() const;
    LspInitializeResult initializeResult() const;

    void openDocument(const QString &uri, const QString &languageId, int version, const QString &text);
    void changeDocument(const QString &uri, int version, const QString &text);
    void saveDocument(const QString &uri, const QString &text = QString());
    void closeDocument(const QString &uri);
    QVector<LspCompletionItem> requestCompletion(const QString &uri, int line, int character, int timeoutMs = 5000, QString *error = nullptr);
    LspHoverResult requestHover(const QString &uri, int line, int character, int timeoutMs = 5000, QString *error = nullptr);
    QVector<LspLocation> requestDefinition(const QString &uri, int line, int character, int timeoutMs = 5000, QString *error = nullptr);
    QVector<LspLocation> requestReferences(const QString &uri, int line, int character, bool includeDeclaration = true, int timeoutMs = 5000, QString *error = nullptr);
    LspWorkspaceEdit requestRename(const QString &uri, int line, int character, const QString &newName, int timeoutMs = 5000, QString *error = nullptr);
    QVector<LspSemanticToken> requestSemanticTokens(const QString &uri, int timeoutMs = 5000, QString *error = nullptr);
    QVector<LspSymbol> requestDocumentSymbols(const QString &uri, int timeoutMs = 5000, QString *error = nullptr);
    QVector<LspSymbol> requestWorkspaceSymbols(const QString &query, int timeoutMs = 5000, QString *error = nullptr);
    void shutdown(int timeoutMs = 2000);

private:
    LspServerCommand command;
    QProcess process;
    QByteArray incomingBuffer;
    int nextRequestId = 1;
    LspInitializeResult lastInitializeResult;
    QMap<int, QJsonObject> responses;

    int sendRequest(const QString &method, const QJsonObject &params);
    void sendNotification(const QString &method, const QJsonObject &params = {});
    void writePayload(const QJsonObject &payload);
    bool waitForResponse(int requestId, QJsonObject *response, int timeoutMs, QString *error);
    void readAvailableMessages();
    void parseBufferedMessages();
    void applyInitializeResult(const QJsonObject &result);
};
