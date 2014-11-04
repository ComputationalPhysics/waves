#include "wavesolver.h"
#include "perlinnoise.h"
#include "cptimer.h"

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include <cmath>

WaveSolver::WaveSolver() :
    m_dampingFactor(0.1),
    m_gridSize(0),
    m_dr(0),
    m_rMin(-1),
    m_rMax(1),
    m_length(2),
    m_averageValue(0.0)
{
    m_ground.setGridType(GridType::Ground);
    m_solution.setGridType(GridType::Water);

    m_rMin = -5;
    m_rMax = 5;
    float length = m_rMax-m_rMin;
    setLength(length);
    setGridSize(200);

    float x0 = 0;
    float y0 = -1.5;
    float amplitude = 0.5;
    float standardDeviation = 0.1;
    double maxValue = 0;
    applyAction([&](int i, int j) {
        float x = m_rMin+i*m_dr;
        float y = m_rMin+j*m_dr;

        // m_solutionPrevious(i,j) = exp(-(pow(x - x0,2)+pow(y - y0,2))/(2*standardDeviation*standardDeviation));
        // m_solution(i,j) = m_solutionPrevious(i,j);

        maxValue = std::max(maxValue,fabs(m_solution(i,j)));
    });

    applyAction([&](int i, int j) {
        m_solutionPrevious(i,j) *= amplitude/std::max(maxValue, 1.0);
        m_solution(i,j) *= amplitude/std::max(maxValue, 1.0);
        m_ground(i,j) = -1;
    });

    // m_ground.createPerlin(15, 0.8, 10.0, -0.45);
    // m_ground.createDoubleSlit();
    // m_ground.createSinus();
    m_ground.createVolcano(m_solution);
    m_solution.copyToGrid(m_solutionPrevious);
    calculateWalls();
}

float WaveSolver::averageValue() const
{
    return m_averageValue;
}


float WaveSolver::dr() const
{
    return m_dr;
}


CPGrid &WaveSolver::ground()
{
    return m_ground;
}

CPGrid &WaveSolver::solution()
{
    return m_solution;
}


CPBox &WaveSolver::box()
{
    return m_box;
}

void WaveSolver::calculateWalls()
{
    calculateMean();
    for(unsigned int i=0;i<gridSize();i++) {
        for(unsigned int j=0;j<gridSize();j++) {
            int oldValue = m_walls(i,j);
            m_walls(i,j) = m_ground(i,j) >= m_solution(i,j);// || i==0 || j==0 || i==gridSize()-1 || j==gridSize()-1;
            if(oldValue != m_walls(i,j) && m_walls(i,j)) {
                // m_solutionPrevious(i,j) = m_solution(i,j) = m_averageValue;
            }
        }
    }
}

void WaveSolver::calculateMean()
{
    m_averageValue = 0;
    unsigned int count = 0;
    for(unsigned int i=0;i<gridSize();i++) {
        for(unsigned int j=0;j<gridSize();j++) {
            if(!m_walls(i,j)) {
                m_averageValue += m_solutionNext(i,j);
                count++;
            }
        }
    }

    m_averageValue /= count;
}

void WaveSolver::setGridSize(int gridSize)
{
    m_solution.resize(gridSize, m_rMin, m_rMax);
    m_solutionNext.resize(gridSize, m_rMin, m_rMax);
    m_solutionPrevious.resize(gridSize, m_rMin, m_rMax);
    m_walls.resize(gridSize, m_rMin, m_rMax);
    m_ground.resize(gridSize, m_rMin, m_rMax);
    m_source.resize(gridSize, m_rMin, m_rMax);
    m_gridSize = gridSize;
    m_dr = m_length / (gridSize-1);
}

void WaveSolver::setLength(float length)
{
    m_length = length;
    m_dr = m_length / (gridSize()-1);
    m_box.update(QVector3D(-m_length/2, -m_length/2, -0.2), QVector3D(m_length, m_length, 0.4));
}

void WaveSolver::applyAction(std::function<void(int i, int j)> action) {
    for(unsigned int i=0; i<gridSize(); i++) {
        for(unsigned int j=0; j<gridSize(); j++) {
            action(i,j);
        }
    }
}

void WaveSolver::applyAction(std::function<void(int i, int j, int gridSize)> action) {
    for(unsigned int i=0; i<gridSize(); i++) {
        for(unsigned int j=0; j<gridSize(); j++) {
            action(i,j, gridSize());
        }
    }
}

void WaveSolver::createRandomGauss() {
    qDebug() << "Actually creating random gauss";
    double x0 = m_rMin + (m_rMax-m_rMin)*rand()/(double)RAND_MAX;
    double y0 = m_rMin + (m_rMax-m_rMin)*rand()/(double)RAND_MAX;
    double stddev = 0.2;
    float amplitude = 0.5;
    applyAction([&](int i, int j) {
        float x = m_rMin + i*m_dr; 					// The x- and y-center can have an offset
        float y = m_rMin + j*m_dr;

        m_solutionPrevious (i,j) += amplitude*exp(-(pow(x-x0,2)+pow(y-y0,2))/(2*stddev*stddev));
        m_solution(i,j)          += amplitude*exp(-(pow(x-x0,2)+pow(y-y0,2))/(2*stddev*stddev));
    });
}

void WaveSolver::step(float dt)
{
    float factor = 1.0/(1+0.5*m_dampingFactor*dt);
    float factor2 = -(1.0-0.5*m_dampingFactor*dt);
    float dtdtOverdrdr = dt*dt/(m_dr*m_dr);

    CPTimer::temp().start();
    for(unsigned int i=0;i<gridSize();i++) {
#pragma clang loop vectorize(enable) interleave(enable)
        for(unsigned int j=0;j<gridSize();j++) {
#ifdef CONSTANTWAVESPEED
            float ddx = solution(i,j,1,0) + solution(i,j,-1,0);
            float ddy = solution(i,j,0,1) + solution(i,j,0,-1) ;
            float ddt_rest = factor2*m_solutionPrevious(i,j) + 2*m_solution(i,j);

            // Set value to zero if we have a wall.
            m_solutionNext(i,j) = m_walls(i,j) ? 0 : factor*(dtdtOverdrdr*(ddx + ddy - 4*m_solution(i,j)) + ddt_rest + m_source(i,j));
#else
            float c = calcC(i,j); // wave speed

            float cx_m = 0.5*(c+calcC(i-1,j)); 	// Calculate the 4 c's we need. We need c_{i \pm 1/2,j} and c_{i,j \pm 1/2}
            float cx_p = 0.5*(c+calcC(i+1,j));
            float cy_m = 0.5*(c+calcC(i,j-1));
            float cy_p = 0.5*(c+calcC(i,j+1));

            float ddx = cx_p*( solution(i,j,1,0)   - m_solution(i,j)) - cx_m*( m_solution(i,j) - solution(i,j,-1,0) );
            float ddy = cy_p*( solution(i,j,0,1)   - m_solution(i,j)) - cy_m*( m_solution(i,j) - solution(i,j,0,-1) );
            float ddt_rest = factor2*m_solutionPrevious(i,j) + 2*m_solution(i,j);

            m_solutionNext(i,j) = m_walls(i,j) ? 0 : factor*(dtdtOverdrdr*(ddx + ddy) + ddt_rest + m_source(i,j));
#endif
        }
    }
    CPTimer::temp().stop();

    CPTimer::copyData().start();
    m_solutionPrevious.swapWithGrid(m_solution);
    m_solution.swapWithGrid(m_solutionNext);
    CPTimer::copyData().stop();

    calculateWalls();
}

#if defined(__ARM_NEON)
void WaveSolver::stepSIMD(float dt)
{
    m_solution.updateZFromGrid();
    m_solutionPrevious.updateZFromGrid();
    const float factor_ = 1.0/(1+0.5*m_dampingFactor*dt);
    const float factor2_ = -(1.0-0.5*m_dampingFactor*dt);
    float32_t factor = factor_;
    float32_t factor2 = factor2_;
    float32_t dtdtOverdrdr = dt*dt/(m_dr*m_dr);

    // ONE SIMD LOOP ------------------------------------------------------------------------
    float32_t *nextSol = &m_solutionNext[m_solutionNext.index(1,1)];
    float *pSol = &m_solution[m_solution.index(1,1)];
    float *pPrevSol = &m_solutionPrevious[m_solutionPrevious.index(1,1)];
    float *pSolDxp = &m_solution[m_solution.index(2,1)];
    float *pSolDxn = &m_solution[m_solution.index(0,1)];
    float *pSolDyp = &m_solution[m_solution.index(1,2)];
    float *pSolDyn = &m_solution[m_solution.index(1,0)];

    // This for loop skips the first row (i=0) and the last (i=gridSize()-1) because of boundary conditions.
    // TODO: Fix this by adding ghost rows in m_z in cpgrid.

    for(unsigned int n=gridSize(); n<gridSize()*(gridSize()-1); n += 4) {
        float32x4_t sol = vld1q_f32(pSol);       // u(i,j)
        float32x4_t prevSol = vld1q_f32(pPrevSol);       // u_prev(i,j)
        float32x4_t solDxp = vld1q_f32(pSolDxp);    // u(i+1,j)
        float32x4_t solDxn = vld1q_f32(pSolDxn);   // u(i-1,j)
        float32x4_t solDyp = vld1q_f32(pSolDyp);  // u(i,j+1)
        float32x4_t solDyn = vld1q_f32(pSolDyn);  // u(i,j-1)

        float32x4_t ddx = vaddq_f32(solDxp, solDxn); // solution(i,j,1,0) + solution(i,j,-1,0);
        float32x4_t ddy = vaddq_f32(solDyp, solDyn); // solution(i,j,0,1) + solution(i,j,0,-1) ;
        float32x4_t ddt_rest = vmulq_n_f32(sol, 2);  // 2*m_solution(i,j);
        ddt_rest = vmlaq_n_f32(ddt_rest, prevSol, factor2); // ddt_rest = factor2*m_solutionPrevious(i,j) + 2*m_solution(i,j);

        float32x4_t next = vaddq_f32(ddx, ddy); // ddx + ddy
        next = vmlaq_n_f32(next, sol, -4);      // ddx + ddy - 4*m_solution(i,j)
        next = vmulq_n_f32(next, dtdtOverdrdr); // dtdtOverdrdr*(ddx + ddy - 4*m_solution(i,j))
        next = vaddq_f32(next, ddt_rest);       // (dtdtOverdrdr*(ddx + ddy - 4*m_solution(i,j)) + ddt_rest)
        next = vmulq_n_f32(next, factor);       // factor*(dtdtOverdrdr*(ddx + ddy - 4*m_solution(i,j)) + ddt_rest)

        vst1q_f32(nextSol, next);

        nextSol += 4; pSol +=4; pPrevSol+=4; pSolDxp += 4; pSolDxn += 4; pSolDyp += 4; pSolDyn += 4;
    }

    int i=0;
    for(unsigned int  j=0; j<gridSize(); j++) {
        float ddx = solution(i,j,1,0) + solution(i,j,-1,0);
        float ddy = solution(i,j,0,1) + solution(i,j,0,-1) ;
        float ddt_rest = factor2*m_solutionPrevious(i,j) + 2*m_solution(i,j);

        // Set value to zero if we have a wall.
        m_solutionNext[m_solution.index(i,j)] = m_walls(i,j) ? 0 : factor*(dtdtOverdrdr*(ddx + ddy - 4*m_solution(i,j)) + ddt_rest + m_source(i,j));
    }

    i=gridSize()-1;
    for(unsigned int j=0; j<gridSize(); j++) {
        float ddx = solution(i,j,1,0) + solution(i,j,-1,0);
        float ddy = solution(i,j,0,1) + solution(i,j,0,-1) ;
        float ddt_rest = factor2*m_solutionPrevious(i,j) + 2*m_solution(i,j);

        // Set value to zero if we have a wall.
        m_solutionNext[m_solution.index(i,j)] = m_walls(i,j) ? 0 : factor*(dtdtOverdrdr*(ddx + ddy - 4*m_solution(i,j)) + ddt_rest + m_source(i,j));
    }

    // ONE SIMD LOOP END------------------------------------------------------------------------

    CPTimer::copyData().start();
    m_solutionNext.updateGridFromZ();
    m_solutionPrevious.swapWithGrid(m_solution);
    m_solution.swapWithGrid(m_solutionNext);
    CPTimer::copyData().stop();
}
#endif
















