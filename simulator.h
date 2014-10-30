#ifndef SIMULATOR_H
#define SIMULATOR_H
#include "wavesolver.h"

class Simulator
{
private:
    WaveSolver m_solver;
public:
    Simulator();
    void step(double dt);
    WaveSolver &solver();
};

#endif // SIMULATOR_H
