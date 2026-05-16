#pragma once

#include <QString>
#include <QStringList>

class ProjectModel
{
public:
    void openRoot(const QString &path);

    QString rootPath() const;
    QStringList files() const;
    static bool isIgnoredDirectoryName(const QString &name);
    static bool isOpenableFile(const QString &path);
    static bool isValidChildName(const QString &name);
    static bool pathIsSameOrInside(const QString &candidatePath, const QString &rootPath);

private:
    QString root;
    QStringList projectFiles;
};

