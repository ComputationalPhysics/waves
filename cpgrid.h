#ifndef CPGRID_H
#define CPGRID_H
#include <vector>
#include <functional>

class CPGrid
{
private:
    std::vector<std::vector<float> > m_points;
public:
    CPGrid();
    std::vector<std::vector<float> > points() const;
    void setPoints(const std::vector<std::vector<float> > &points);
    int gridSize() { return m_points.size(); }
    void setGridSize(int gridSize) {
        m_points.resize(gridSize, std::vector<float>(gridSize));
    }

    inline int idx(int i) { return (i+gridSize()) % gridSize(); }
    float operator()(int i, int j) {
        return m_points[idx(i)][idx(j)];
    }

    std::vector<float> &operator[](int i) {
        return m_points[i];
    }

    void for_each(std::function<void(int i, int j)> action);

    void zeros() {

    }
};

#endif // CPGRID_H
