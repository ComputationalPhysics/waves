#include "waves.h"
#include <QtQuick/qquickwindow.h>
#include <cmath>
#include "cptimer.h"

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
    const qreal zNear = 1.0, zFar = 200.0, fov = 65.0;

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

    glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glFrontFace(GL_CCW);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if(m_simulator) {
        QMatrix4x4 modelViewProjectionMatrix = m_projectionMatrix * m_modelViewMatrix;
//        QMatrix4x4 lightModelViewProjectionMatrix = m_projectionMatrix * m_lightModelViewMatrix;
//        m_simulator->solver().box().render(modelViewProjectionMatrix);
        m_simulator->solver().ground().renderAsTriangles(modelViewProjectionMatrix, m_modelViewMatrix);
        m_simulator->solver().solution().renderAsTriangles(modelViewProjectionMatrix, m_modelViewMatrix);
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

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
      m_previousStepCompleted(true),
      m_steps(0)
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
    CPTimer::sync().start();
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

    if(m_running) {
        // Step if running
        CPTimer::computeTimestep().start();
        m_simulator.step(safeDt);
        CPTimer::computeTimestep().stop();
    }

    if(!(m_steps++ % 60)) {
        float computeTimestepFraction = round(10000*CPTimer::computeTimestep().elapsedTime() / CPTimer::totalTime())/100;
        float normalVectorsFraction = round(10000*CPTimer::normalVectors().elapsedTime() / CPTimer::totalTime())/100;
        float renderingFraction = round(10000*CPTimer::rendering().elapsedTime() / CPTimer::totalTime())/100;
        float uploadVBOFraction = round(10000*CPTimer::uploadVBO().elapsedTime() / CPTimer::totalTime())/100;
        float drawElementsFraction = round(10000*CPTimer::drawElements().elapsedTime() / CPTimer::totalTime())/100;
        float syncFraction = round(10000*CPTimer::sync().elapsedTime() / CPTimer::totalTime())/100;
        float tempFraction = round(10000*CPTimer::temp().elapsedTime() / CPTimer::totalTime())/100;
        float copyDataFraction = round(10000*CPTimer::copyData().elapsedTime() / CPTimer::totalTime())/100;
        qDebug() << endl <<"Computing timesteps: " << CPTimer::computeTimestep().elapsedTime() << " s (" << computeTimestepFraction << "%)";
        qDebug() << "Normal vectors: " << CPTimer::normalVectors().elapsedTime() << " s (" << normalVectorsFraction << "%)";
        qDebug() << "Upload VBO: " << CPTimer::uploadVBO().elapsedTime() << " s (" << uploadVBOFraction << "%)";
        qDebug() << "Draw elements: " << CPTimer::drawElements().elapsedTime() << " s (" << drawElementsFraction << "%)";
        qDebug() << "Sync: " << CPTimer::sync().elapsedTime() << " s (" << syncFraction << "%)";
        qDebug() << "Rendering: " << CPTimer::rendering().elapsedTime() << " s (" << renderingFraction << "%)";
        qDebug() << "Temp: " << CPTimer::temp().elapsedTime() << " s (" << tempFraction << "%)";
        qDebug() << "Copy data: " << CPTimer::copyData().elapsedTime() << " s (" << copyDataFraction << "%)";
        qDebug() << "Timestep: " << safeDt;
    }

    m_previousStepCompleted = true;
    CPTimer::sync().stop();
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

