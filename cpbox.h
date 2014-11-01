#ifndef CPBOX_H
#define CPBOX_H
#include "cpgrid.h"
#include <vector>

class CPBox
{
private:
    std::vector<CPPoint> m_vertices;
    std::vector<GLushort> m_indices;

    GLuint m_vboIds[2];
    QOpenGLFunctions *m_funcs;
    QOpenGLShaderProgram *m_program;
    QString   m_vertexShader;
    QString   m_fragmentShader;

    void generateVBOs();
    void ensureInitialized();
    virtual void createShaderProgram();
    virtual void uploadVBO();
    virtual void setShaders();

public:
    CPBox();
    ~CPBox();

    void update(QVector3D origin, QVector3D size);
    void render(QMatrix4x4 &modelViewProjectionMatrix);
};

#endif // CPBOX_H
