#include "waves.h"
#include <QtQuick/qquickwindow.h>
#include <cmath>
using std::vector;

WavesRenderer::WavesRenderer() :
    m_simulator(0)
{

}

WavesRenderer::~WavesRenderer() {

}

void WavesRenderer::resetProjection() {
    // Calculate aspect ratio
    qreal aspect = qreal(m_viewportSize.width()) / qreal(m_viewportSize.height() ? m_viewportSize.height() : 1);

    // Set near plane to 3.0, far plane to 7.0, field of view 65 degrees
    const qreal zNear = 2.0, zFar = 2000.0, fov = 65.0;

    // Reset projection
    m_projectionMatrix.setToIdentity();

    // Set perspective projection
    m_projectionMatrix.perspective(fov, aspect, zNear, zFar);
}

void WavesRenderer::setModelViewMatrices(double zoom, double tilt, double pan, double roll) {
    m_modelViewMatrix.setToIdentity();
    m_modelViewMatrix.translate(0,0,zoom);
    m_modelViewMatrix.rotate(-90, 1, 0, 0);
    m_modelViewMatrix.rotate(tilt, 1, 0, 0);
    m_modelViewMatrix.rotate(pan, 0, 0, 1);
    m_modelViewMatrix.rotate(roll, 0, 1, 0);

    float systemSizeMax = 1.0;
    m_lightModelViewMatrix.setToIdentity();
    m_lightModelViewMatrix.translate(0,0,-systemSizeMax / 2.0);
    m_lightModelViewMatrix.rotate(-90, 1, 0, 0);
    m_lightModelViewMatrix.rotate(tilt, 1, 0, 0);
    m_lightModelViewMatrix.rotate(pan, 0, 0, 1);
    m_lightModelViewMatrix.rotate(roll, 0, 1, 0);
}

void WavesRenderer::paint() {
    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    QMatrix4x4 modelViewProjectionMatrix = m_projectionMatrix * m_modelViewMatrix;
    QMatrix4x4 lightModelViewProjectionMatrix = m_projectionMatrix * m_lightModelViewMatrix;
    m_simulator->solver().solution().renderAsTriangles(modelViewProjectionMatrix, m_modelViewMatrix);

    glDepthMask(GL_TRUE);
}
Simulator *WavesRenderer::simulator() const
{
    return m_simulator;
}

void WavesRenderer::setSimulator(Simulator *simulator)
{
    m_simulator = simulator;
}


void Waves::step()
{
    if(!m_previousStepCompleted) {
        return;
    }
    if(window()) {
        window()->update();
    }
    m_previousStepCompleted = false;
}

Waves::Waves()
    : m_renderer(0),
      m_zoom(-1),
      m_tilt(0),
      m_pan(0),
      m_roll(0),
      m_running(true),
      m_previousStepCompleted(true)
{
    connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
    m_timer.start();
}

Waves::~Waves()
{
    cleanup();
}

void Waves::sync()
{
    if (!m_renderer) {
        m_renderer = new WavesRenderer();
        m_renderer->setSimulator(&m_simulator);

        connect(window(), SIGNAL(beforeRendering()), m_renderer, SLOT(paint()), Qt::DirectConnection);
    }
    m_renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
    m_renderer->resetProjection();
    m_renderer->setModelViewMatrices(m_zoom, m_tilt, m_pan, m_roll);

    double dt = m_timer.restart() / 1000.0;

    double c_max = 1.0;       			// Used to determine dt and Nt
    double safeDt = 0.9*m_simulator.solver().dr()/sqrt(2*c_max); 			// This guarantees (I guess) stability if c_max is correct
    qDebug() << "safe dt: " << safeDt;
    safeDt = std::min(safeDt, dt);

    if(m_running) {
        // Step if running
        m_simulator.step(safeDt);
    }

    m_previousStepCompleted = true;
}

void Waves::cleanup()
{
    if(m_renderer) {
        delete m_renderer;
        m_renderer = 0;
    }
}

void Waves::handleWindowChanged(QQuickWindow *win)
{
    if (win) {
        connect(win, SIGNAL(beforeSynchronizing()), this, SLOT(sync()), Qt::DirectConnection);
        connect(win, SIGNAL(sceneGraphInvalidated()), this, SLOT(cleanup()), Qt::DirectConnection);
        // If we allow QML to do the clearing, they would clear what we paint
        // and nothing would show.
        win->setClearBeforeRendering(false);
    }
}
Simulator Waves::simulator() const
{
    return m_simulator;
}

void Waves::setSimulator(const Simulator &simulator)
{
    m_simulator = simulator;
}

