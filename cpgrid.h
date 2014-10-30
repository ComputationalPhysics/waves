#ifndef CPGRID_H
#define CPGRID_H
#include <QtGui/QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include <functional>
#include <iostream>

class CPPoint
{
public:
    QVector3D position;
    QVector3D normal;
};

class CPTriangle
{
public:
    CPPoint* points[3];
    QVector3D normal;

    CPTriangle(CPPoint *p1, CPPoint *p2, CPPoint *p3) {
        points[0] = p1;
        points[1] = p2;
        points[2] = p3;
    }

    void calculateNormal() {
        normal = QVector3D::crossProduct(points[2]->position - points[0]->position, points[1]->position - points[0]->position).normalized();
        points[0]->normal = normal;
        points[1]->normal = normal;
        points[2]->normal = normal;
    }
};

enum class GridType {NotUsed = 0, Water = 1, Ground = 2};

class CPGrid
{
private:
    std::vector<CPPoint>      m_vertices;
    std::vector<GLushort>     m_indices;
    std::vector<CPTriangle>   m_triangles;
    QString                   m_waterVertexShader;
    QString                   m_waterFragmentShader;
    QString                   m_groundVertexShader;
    QString                   m_groundFragmentShader;

    int m_gridSize;
    GridType m_gridType;
    bool m_indicesDirty;

    // OpenGL stuff
    GLuint m_vboIds[2];
    QOpenGLFunctions *m_funcs;
    QOpenGLShaderProgram *m_program;

    inline int index(int i, int j) {
        return i*gridSize() + j;
    }
    inline int idx(int i) { return (i+gridSize()) % gridSize(); }

    void createShaderProgram();
    void generateVBOs();
    void ensureInitialized();
    void uploadVBO();
    void setShaders();

public:
    CPGrid();
    ~CPGrid();
    void for_each(std::function<void(CPPoint &p)> action);
    void for_each(std::function<void(CPPoint &p, int i, int j)> action);
    void for_each(std::function<void(CPPoint &p, int i, int j, int gridSize)> action);

    int gridSize() { return m_gridSize; }
    void resize(int gridSize, float rMin, float rMax);

    float &operator()(int i, int j, bool periodicBoundary) {
        return m_vertices[index(idx(i), idx(j))].position[2];
    }
    float &operator()(int i, int j) {
        return m_vertices[index(i, j)].position[2];
    }

    void zeros();
    void renderAsTriangles(QMatrix4x4 &modelViewProjectionMatrix, QMatrix4x4 &modelViewMatrix);
    void calculateNormals();
    std::vector<CPPoint> vertices() const;
    void setVertices(const std::vector<CPPoint> &vertices);
    GridType getGridType() const;
    void setGridType(const GridType &GridType);

    void createPerlin(unsigned int seed, float amplitude, float lengthScale, float deltaZ);
    void createDoubleSlit();
    void copyGridFrom(CPGrid &grid);
};

#endif // CPGRID_H
