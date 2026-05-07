#pragma once

#include "ApyHighlighter.h"

#include <QPlainTextEdit>
#include <QVector>

struct HiddenBidiFinding
{
    int position = 0;
    QString character;
    QString unicodeName;
};

class EditorSurface final : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit EditorSurface(QWidget *parent = nullptr);

    bool openFile(const QString &path, QString *error = nullptr);
    bool saveFile(QString *error = nullptr);
    bool saveFileAs(const QString &path, QString *error = nullptr);

    QString currentFilePath() const;
    bool isDirty() const;
    QVector<HiddenBidiFinding> findHiddenBidiControls(const QString &text) const;

signals:
    void filePathChanged(const QString &path);
    void dirtyStateChanged(bool dirty);

private:
    QString filePath;
    ApyHighlighter *highlighter = nullptr;

    static QString unicodeName(QChar ch);
    void setCurrentFilePath(const QString &path);
};

