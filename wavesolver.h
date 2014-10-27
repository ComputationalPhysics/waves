#ifndef WAVESOLVER_H
#define WAVESOLVER_H
#include "cpgrid.h"
#include <functional>

class WaveSolver
{
private:
    CPGrid m_solution;
    CPGrid m_solutionNext;
    CPGrid m_solutionPrevious;
    CPGrid m_ground;
    CPGrid m_walls;
    CPGrid m_source;
    float  m_dampingFactor;
    int    m_gridSize;
    float  m_dr;
    float  m_length;
    float  m_averageValue;
    void calculateWalls();
    void calculateMean();
public:
    WaveSolver();
    void setGridSize(int gridSize);
    unsigned int gridSize() { return m_solution.gridSize(); }
    void setLength(float length);
    void step(float dt);
    void applyAction(std::function<void(int i, int j)> action);

    inline float calcC(int i, int j) {
        return std::max(-m_ground(i,j),1.0f);
    }

    inline float solution(int i,int j, int di, int dj) {
        if(m_walls(i+di,j+dj)) {
            return m_solution(i-di,j-dj);
        }
        return m_solution(i+di,j+dj);
    }

    inline float solutionPrevious(int i,int j, int di, int dj) {
        if(m_walls(i+di,j+dj)) {
            return m_solutionPrevious(i-di,j-dj);
        }

        return m_solutionPrevious(i+di,j+dj);
    }
    float averageValue() const;
};

#endif // WAVESOLVER_H
