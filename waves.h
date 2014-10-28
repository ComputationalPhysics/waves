#ifndef WAVES_H
#define WAVES_H
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLShaderProgram>
#include <QElapsedTimer>
#include <QMatrix4x4>

#include "simulator.h"

class WavesRenderer : public QObject {
    Q_OBJECT
public:
    WavesRenderer();
    ~WavesRenderer();
    void setViewportSize(const QSize &size) { m_viewportSize = size; }
    void resetProjection();
    void setModelViewMatrices(double zoom, double tilt, double pan, double roll);

public slots:
    void paint();

private:
    QSize m_viewportSize;
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_modelViewMatrix;
    QMatrix4x4 m_lightModelViewMatrix;
};

class Waves : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(double tilt READ tilt WRITE setTilt NOTIFY tiltChanged)
    Q_PROPERTY(double pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(double roll READ roll WRITE setRoll NOTIFY rollChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool previousStepCompleted READ previousStepCompleted NOTIFY previousStepCompletedChanged)
public:
    Q_INVOKABLE void step();
    Waves();
    ~Waves();

    float zoom() const
    {
        return m_zoom;
    }

    double tilt() const
    {
        return m_tilt;
    }

    double pan() const
    {
        return m_pan;
    }

    double roll() const
    {
        return m_roll;
    }

    bool running() const
    {
        return m_running;
    }

    Simulator simulator() const;
    void setSimulator(const Simulator &simulator);

    bool previousStepCompleted() const
    {
        return m_previousStepCompleted;
    }

public slots:
    void sync();
    void cleanup();

    void setZoom(float arg)
    {
        if (m_zoom == arg)
            return;

        m_zoom = arg;
        emit zoomChanged(arg);
    }

    void setTilt(double arg)
    {
        if (m_tilt == arg)
            return;

        m_tilt = arg;
        emit tiltChanged(arg);
    }

    void setPan(double arg)
    {
        if (m_pan == arg)
            return;

        m_pan = arg;
        emit panChanged(arg);
    }

    void setRoll(double arg)
    {
        if (m_roll == arg)
            return;

        m_roll = arg;
        emit rollChanged(arg);
    }

    void setRunning(bool arg)
    {
        if (m_running == arg)
            return;

        m_running = arg;
        emit runningChanged(arg);
    }

signals:
    void zoomChanged(float arg);

    void tiltChanged(double arg);

    void panChanged(double arg);

    void rollChanged(double arg);

    void runningChanged(bool arg);

    void previousStepCompletedChanged(bool arg);

private slots:
    void handleWindowChanged(QQuickWindow *win);

private:
    WavesRenderer *m_renderer;
    Simulator m_simulator;
    float m_zoom;
    float m_tilt;
    float m_pan;
    float m_roll;
    bool  m_running;
    QElapsedTimer m_timer;

    bool m_previousStepCompleted;
};

#endif // WAVES_H
