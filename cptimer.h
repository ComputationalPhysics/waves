#ifndef CPTIMER_H
#define CPTIMER_H
#include <QElapsedTimer>
#include <QDebug>
class CPTimingObject {
private:
    QElapsedTimer m_timer;
    double m_timeElapsed;
public:
    CPTimingObject() : m_timeElapsed(0) { }

    void start() {
        m_timer.restart();
    }

    void stop() {
        m_timeElapsed += m_timer.elapsed() / double(1000);
        m_timer.restart();
    }

    double elapsedTime() { return m_timeElapsed; }
};

class CPTimer
{
public:
    CPTimer();

    static CPTimer& getInstance()
    {
        static CPTimer instance; // Guaranteed to be destroyed.
                                 // Instantiated on first use.
        return instance;
    }

    QElapsedTimer  m_timer;
    CPTimingObject m_computeTimestep;
    CPTimingObject m_normalVectors;
    CPTimingObject m_rendering;
    CPTimingObject m_uploadVBO;
    CPTimingObject m_drawElements;
    CPTimingObject m_sync;
    CPTimingObject m_copyData;
    CPTimingObject m_temp;

    static CPTimingObject &computeTimestep() { return CPTimer::getInstance().m_computeTimestep; }
    static CPTimingObject &normalVectors() { return CPTimer::getInstance().m_normalVectors; }
    static CPTimingObject &rendering() { return CPTimer::getInstance().m_rendering; }
    static CPTimingObject &uploadVBO() { return CPTimer::getInstance().m_uploadVBO; }
    static CPTimingObject &drawElements() { return CPTimer::getInstance().m_drawElements; }
    static CPTimingObject &sync() { return CPTimer::getInstance().m_sync; }
    static CPTimingObject &copyData() { return CPTimer::getInstance().m_copyData; }
    static CPTimingObject &temp() { return CPTimer::getInstance().m_temp; }
    static double totalTime() { return CPTimer::getInstance().m_timer.elapsed() / double(1000); }
};

#endif // CPTIMER_H
