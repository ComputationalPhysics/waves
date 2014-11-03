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

    CPPoint() { }

    CPPoint(QVector3D pos) {
        position = pos;
    }

    CPPoint(QVector3D pos, QVector3D norm) {
        position = pos;
        normal = norm;
    }
};

class CPTriangle
{
public:
    int      pointIndices[3];
    QVector3D normal;

    CPTriangle(int p1, int p2, int p3) {
        pointIndices[0] = p1;
        pointIndices[1] = p2;
        pointIndices[2] = p3;
    }

    void calculateNormal(std::vector<CPPoint> &allPoints) {
        CPPoint &p0 = allPoints[pointIndices[0]];
        CPPoint &p1 = allPoints[pointIndices[1]];
        CPPoint &p2 = allPoints[pointIndices[2]];
        // normal = QVector3D::crossProduct(points[2]->position - points[0]->position, points[1]->position - points[0]->position).normalized();
        // normal = QVector3D::crossProduct(p2.position - p0.position, p1.position - p0.position).normalized();
        normal = QVector3D::crossProduct(p2.position - p0.position, p1.position - p0.position);
        p0.normal = normal;
        p1.normal = normal;
        p2.normal = normal;
    }
};

enum class GridType {NotUsed = 0, Water = 1, Ground = 2};

// typedef GLushort index_t;
typedef GLuint index_t;

class CPGrid
{
private:
    std::vector<CPPoint>      m_vertices;
    std::vector<index_t>     m_indices;
    std::vector<CPTriangle>   m_triangles;
    std::vector<float>        m_z;
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
    void for_each(std::function<void(CPPoint &p, int i, int j, int gridSize, int index)> action);

    inline int index(int i, int j) {
        return i*gridSize() + j;
    }
    inline int idx(int i) { return (i+gridSize()) % gridSize(); }

    int gridSize() { return m_gridSize; }
    void resize(int gridSize, float rMin, float rMax);

    float &operator()(int i, int j, bool) {
        return m_vertices[index(idx(i), idx(j))].position[2];
    }
    float &operator()(int i, int j) {
        return m_vertices[index(i, j)].position[2];
    }

    float &operator[](int i) {
        return m_z[i];
    }

    void zeros();
    void renderAsTriangles(QMatrix4x4 &modelViewProjectionMatrix, QMatrix4x4 &modelViewMatrix);
    void calculateNormals();
    std::vector<CPPoint> &vertices();
    void setVertices(const std::vector<CPPoint> &vertices);
    GridType getGridType() const;
    void setGridType(const GridType &GridType);

    void createPerlin(unsigned int seed, float amplitude, float lengthScale, float deltaZ);
    void createDoubleSlit();
    void createSinus();
    void swapWithGrid(CPGrid &grid);
    void updateGridFromZ();

    void updateZFromGrid();
    void createLand();
};

#endif // CPGRID_H
