#ifndef WAVESOLVER_H
#define WAVESOLVER_H
#include "cpgrid.h"
#include "cpbox.h"

#include <functional>

enum class GroundType {Slope = 0, PerlinNoise = 1};

class WaveSolver
{
private:
    CPGrid m_solution;
    CPGrid m_solutionNext;
    CPGrid m_solutionPrevious;
    CPGrid m_ground;
    CPGrid m_walls;
    CPGrid m_source;
    CPBox  m_box;
    float  m_dampingFactor;
    int    m_gridSize;
    float  m_dr;
    float  m_rMin;
    float  m_rMax;
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
    void stepSIMD(float dt);

    inline float calcC(int i, int j) {
        float factor = 1;
        if(m_ground(i,j,true) < 0) {
            return -m_ground(i,j,true);
        }
        float depth = m_solution(i,j,true) - m_ground(i,j,true);
        if(depth < -0.3) {
            return 0.001f;
        }
        return std::min(std::max(factor * (depth - 0.5f), 0.01f), 1.0f);
    }

    inline float solution(int i,int j, int di, int dj) {
        //        if(m_ground(i+di,j+dj,true) > m_solution(i+di, j+dj, true)) {
        //            return m_solution(i-di,j-dj,true);
        //        }
        return m_solution(i+di,j+dj, true);
    }

    inline float solutionPrevious(int i,int j, int di, int dj) {
        if(m_walls(i+di,j+dj,true)) {
            return m_solutionPrevious(i-di,j-dj,true);
        }

        return m_solutionPrevious(i+di,j+dj, true);
    }
    float averageValue() const;
    float dr() const;
    void applyAction(std::function<void(int i, int j)> action);
    void applyAction(std::function<void (int, int, int)> action);
    CPGrid &ground();
    CPGrid &solution();
    void createRandomGauss();
    CPBox &box();
};

#endif // WAVESOLVER_H
