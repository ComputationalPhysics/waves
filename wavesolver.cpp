#include "wavesolver.h"
#include "perlinnoise.h"

#include <cmath>

float WaveSolver::averageValue() const
{
    return m_averageValue;
}


float WaveSolver::dr() const
{
    return m_dr;
}


CPGrid WaveSolver::ground() const
{
    return m_ground;
}

void WaveSolver::calculateWalls()
{
    calculateMean();
    for(unsigned int i=0;i<gridSize();i++) {
        for(unsigned int j=0;j<gridSize();j++) {
            int oldValue = m_walls(i,j);
            m_walls(i,j) = m_ground(i,j) > m_averageValue;
            if(oldValue != m_walls(i,j)) {
                m_solutionPrevious(i,j) = m_solution(i,j) = m_averageValue;
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

WaveSolver::WaveSolver() :
    m_dampingFactor(1),
    m_gridSize(0),
    m_dr(0),
    m_rMin(-1),
    m_rMax(1),
    m_length(2),
    m_averageValue(0)
{
    m_ground.setGridType(GridType::Ground);
    setGridSize(128);
    float amplitude = 0.5;
    float length = m_rMax-m_rMin;
    setLength(length);
    float x0 = 0;
    float y0 = 0;
    float standardDeviation = 0.1;
    double maxValue = 0;
    applyAction([&](int i, int j) {
        float x = m_rMin+i*m_dr;
        float y = m_rMin+j*m_dr;

        m_solutionPrevious(i,j) = exp(-(pow(x - x0,2)+pow(y - y0,2))/(2*standardDeviation*standardDeviation));
        m_solution(i,j) = m_solutionPrevious(i,j);

        maxValue = std::max(maxValue,fabs(m_solution(i,j)));
    });

    applyAction([&](int i, int j) {
        m_solutionPrevious(i,j) *= amplitude/std::max(maxValue, 1.0);
        m_solution(i,j) *= amplitude/std::max(maxValue, 1.0);
    });

    createGround(GroundType::PerlinNoise);
    calculateWalls();
}

void WaveSolver::createGround(GroundType type) {
    if(type == GroundType::PerlinNoise) {
        createPerlinNoiseGround(0, 0.5, 5.0, -0.4);

    } else if(type == GroundType::Slope) {
        applyAction([&](int i, int j, int gridSize) {
            float height = 0.5 - 2*i/float(gridSize);
            m_ground(i,j) = height;
        });
    }

    m_ground.calculateNormals();
}

void WaveSolver::createPerlinNoiseGround(unsigned int seed, float amplitude, float lengthScale, float deltaZ) {
    PerlinNoise perlin(seed);
    m_ground.for_each([&](CPPoint &p, int i, int j, int gridSize) {
        float x = i/float(gridSize);
        float y = j/float(gridSize);

        float z = amplitude*(perlin.noise(x*lengthScale,y*lengthScale,0)) + deltaZ;
        p.position.setZ(z);
    });
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

void WaveSolver::step(float dt)
{
    float factor = 1.0/(1+0.5*m_dampingFactor*dt);
    float dtdtOverdrdr = dt*dt/(m_dr*m_dr);

    // calculateSource();

    // #pragma omp parallel for private(cx_m, cx_p,cy_m, cy_p, c, ddx, ddy, ddt_rest,i,j) num_threads(1)
    for(unsigned int i=0;i<gridSize();i++) {
        for(unsigned int j=0;j<gridSize();j++) {
            float c = calcC(i,j); // wave speed

            float cx_m = 0.5*(c+calcC(i-1,j)); 	// Calculate the 4 c's we need. We need c_{i \pm 1/2,j} and c_{i,j \pm 1/2}
            float cx_p = 0.5*(c+calcC(i+1,j));
            float cy_m = 0.5*(c+calcC(i,j-1));
            float cy_p = 0.5*(c+calcC(i,j+1));

            float ddx = cx_p*( solution(i,j,1,0)   - m_solution(i,j)) - cx_m*( m_solution(i,j) - solution(i,j,-1,0) );
            float ddy = cy_p*( solution(i,j,0,1)   - m_solution(i,j) ) - cy_m*( m_solution(i,j) - solution(i,j,0,-1) );
            float ddt_rest = -(1-0.5*m_dampingFactor*dt)*m_solutionPrevious(i,j) + 2*m_solution(i,j);

            // Set value to zero if we have a wall.
            m_solutionNext(i,j) = m_walls(i,j) ? 0 : factor*(dtdtOverdrdr*(ddx + ddy) + ddt_rest + m_source(i,j));
        }
    }

    m_solutionPrevious = m_solution;
    m_solution = m_solutionNext;

    calculateWalls();
    calculateMean();// Switched this and next line
    m_source.zeros();
}
