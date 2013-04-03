#include <QtDebug>

#include "BuildJob.h"

namespace PlatformPlasma {
namespace Internal {


BuildJob::BuildJob(QObject *parent)
    : KJob(parent)
    , m_min(0)
    , m_max(0)
    , m_progress(0)
{
}

void BuildJob::completed()
{
    kill(KJob::EmitResult);
}

void BuildJob::setApplicationProgressRange(int min, int max)
{
    m_min = min;
    m_max = max;
    showPercentage();
}

void BuildJob::setApplicationProgressValue(int progress)
{
    m_progress = progress;
    showPercentage();
}

void BuildJob::setApplicationLabel(const QString &text)
{
    qDebug() << "BuildJob::setApplicationLabel:" << text;
    if (!text.isEmpty()) {
        setErrorText(tr("%1 error(s).").arg(text));
    }
}

void BuildJob::showPercentage()
{
    const int range = m_max - m_min;
    const int current = m_progress - m_min;
    const long percentage = range == 0 ? 0 : 0.5 + 100.0 * (1.0 * current)/range;
    setPercent(percentage);
}

} // namespace Internal
} // namespace HelloWorld
