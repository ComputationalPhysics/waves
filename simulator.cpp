#include "simulator.h"
#include "cpgrid.h"
#include <iostream>

WaveSolver &Simulator::solver()
{
    return m_solver;
}

Simulator::Simulator()
{

}

void Simulator::step(double dt) {
    m_solver.stepSIMD(dt);
}
