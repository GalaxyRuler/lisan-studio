#include "OutputTranscript.h"

namespace
{
bool includesChannel(const OutputTranscriptFilter &filter, OutputTranscriptChannel channel)
{
    switch (channel) {
    case OutputTranscriptChannel::Stdout:
        return filter.includeStdout;
    case OutputTranscriptChannel::Stderr:
        return filter.includeStderr;
    case OutputTranscriptChannel::System:
        return filter.includeSystem;
    }
    return false;
}

bool matchesQuery(const OutputTranscriptFilter &filter, const OutputTranscriptChunk &chunk)
{
    const QString query = filter.query.trimmed().toCaseFolded();
    if (query.isEmpty()) {
        return true;
    }

    const QString haystack = (chunk.label + QLatin1Char('\n') + chunk.text).toCaseFolded();
    return haystack.contains(query);
}
}

void OutputTranscript::append(OutputTranscriptChannel channel, const QString &label, const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    OutputTranscriptChunk chunk;
    chunk.channel = channel;
    chunk.label = label;
    chunk.text = text.trimmed();
    recordedChunks.push_back(chunk);
}

void OutputTranscript::clear()
{
    recordedChunks.clear();
}

QVector<OutputTranscriptChunk> OutputTranscript::chunks() const
{
    return recordedChunks;
}

QString OutputTranscript::render(const OutputTranscriptFilter &filter) const
{
    QStringList renderedChunks;
    for (const OutputTranscriptChunk &chunk : recordedChunks) {
        if (!includesChannel(filter, chunk.channel) || !matchesQuery(filter, chunk)) {
            continue;
        }
        renderedChunks.append(QStringLiteral("[%1]\n%2").arg(chunk.label, chunk.text));
    }
    return renderedChunks.join(QLatin1Char('\n'));
}
