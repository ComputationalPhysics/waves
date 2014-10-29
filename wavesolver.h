#ifndef WAVESOLVER_H
#define WAVESOLVER_H
#include "cpgrid.h"
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

    inline float calcC(int i, int j) {
        return 1.0;
        // return std::max(-m_ground(i,j),1.0f);
    }

    inline CPGrid &solution() {
        return m_solution;
    }

    inline float solution(int i,int j, int di, int dj) {
//        if(m_walls(i+di,j+dj,true)) {
//            return m_solution(i-di,j-dj,true);
//        }
        return m_solution(i+di,j+dj,true);
    }

    inline float solutionPrevious(int i,int j, int di, int dj) {
//        if(m_walls(i+di,j+dj,true)) {
//            return m_solutionPrevious(i-di,j-dj,true);
//        }

        return m_solutionPrevious(i+di,j+dj,true);
    }
    float averageValue() const;
    float dr() const;
    void applyAction(std::function<void(int i, int j)> action);
    void applyAction(std::function<void (int, int, int)> action);
    CPGrid ground() const;
    void createPerlinNoiseGround(unsigned int seed, float amplitude, float lengthScale, float deltaZ);
    void createGround(GroundType type);
};

#endif // WAVESOLVER_H
