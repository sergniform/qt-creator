#ifndef BUILDJOB_H
#define BUILDJOB_H

#include <kjob.h>

namespace PlatformPlasma {
namespace Internal {

class BuildJob : public KJob
{
    Q_OBJECT
public:
    explicit BuildJob(QObject *parent = 0);

    void setProjectName(const QString& projectName);
    void registered();
    void completed();
    void setApplicationProgressRange(int min, int max);
    void setApplicationProgressValue(int progress);
    void setApplicationLabel(const QString& text);
protected:
    void start () {}

private:
    void showPercentage();
    int m_min, m_max, m_progress;
    QString m_projectName;
};

} // namespace Internal
} // namespace HelloWorld

#endif // BUILDJOB_H
