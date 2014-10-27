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
    Simulator simulator() const;
    void setSimulator(const Simulator &simulator);

public slots:
    void paint();

private:
    QSize m_viewportSize;
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_modelViewMatrix;
    Simulator  m_simulator;
};

class Waves : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(double tilt READ tilt WRITE setTilt NOTIFY tiltChanged)
    Q_PROPERTY(double pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(double roll READ roll WRITE setRoll NOTIFY rollChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)

public:
    Waves();
    ~Waves();
    double tilt() const;
    double pan() const;
    double roll() const;
    double zoom() const;
    bool running() const;

public slots:
    void sync();
    void cleanup();
    void setTilt(double arg);
    void setPan(double arg);
    void setRoll(double arg);
    void setZoom(double arg);
    void setRunning(bool arg);

private slots:
    void handleWindowChanged(QQuickWindow *win);
signals:
    void tiltChanged(double arg);
    void panChanged(double arg);
    void rollChanged(double arg);
    void zoomChanged(double arg);
private:
    WavesRenderer *m_renderer;
    float m_zoom;
    float m_tilt;
    float m_pan;
    float m_roll;
    bool  m_running;
    QElapsedTimer m_timer;

};

#endif // WAVES_H
