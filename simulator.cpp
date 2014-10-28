#include "simulator.h"
#include "cpgrid.h"
#include <iostream>

WaveSolver Simulator::solver() const
{
    return m_solver;
}

Simulator::Simulator()
{

}

void Simulator::step(double dt) {
    m_solver.step(dt);
}
