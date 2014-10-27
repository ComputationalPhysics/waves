#include "cpgrid.h"

CPGrid::CPGrid()
{

}

std::vector<std::vector<float> > CPGrid::points() const
{
    return m_points;
}

void CPGrid::setPoints(const std::vector<std::vector<float> > &points)
{
    m_points = points;
}

void CPGrid::for_each(std::function<void (int, int)> action)
{
    for(int i=0; i<gridSize(); i++) {
        for(int j=0; j<gridSize(); j++) {
            action(i,j);
        }
    }
}
