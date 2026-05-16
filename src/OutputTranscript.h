#pragma once

#include <QString>
#include <QVector>

enum class OutputTranscriptChannel
{
    Stdout,
    Stderr,
    System
};

struct OutputTranscriptChunk
{
    OutputTranscriptChannel channel = OutputTranscriptChannel::System;
    QString label;
    QString text;
};

struct OutputTranscriptFilter
{
    bool includeStdout = true;
    bool includeStderr = true;
    bool includeSystem = true;
    QString query;
};

class OutputTranscript final
{
public:
    void append(OutputTranscriptChannel channel, const QString &label, const QString &text);
    void clear();

    QVector<OutputTranscriptChunk> chunks() const;
    QString render(const OutputTranscriptFilter &filter = OutputTranscriptFilter()) const;

private:
    QVector<OutputTranscriptChunk> recordedChunks;
};
