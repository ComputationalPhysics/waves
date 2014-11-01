#include "cpbox.h"
#include "cptimer.h"

void CPBox::createShaderProgram()
{
    if (!m_program) {
        m_program = new QOpenGLShaderProgram();

        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, m_vertexShader);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, m_fragmentShader);

        m_program->link();
    }
}

void CPBox::generateVBOs()
{
    m_funcs->glGenBuffers(2, m_vboIds);
}

void CPBox::ensureInitialized()
{
    if(!m_funcs) {
        m_funcs = new QOpenGLFunctions(QOpenGLContext::currentContext());
        generateVBOs();
        createShaderProgram();
    }
}

void CPBox::uploadVBO()
{
    ensureInitialized();

    CPTimer::uploadVBO().start();
    // Transfer vertex data to VBO 0
    m_funcs->glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
    m_funcs->glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(CPPoint), &m_vertices[0], GL_STATIC_DRAW);

    // Transfer index data to VBO 1
    m_funcs->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIds[1]);
    m_funcs->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLushort), &m_indices[0], GL_STATIC_DRAW);
    CPTimer::uploadVBO().stop();
}

void CPBox::setShaders()
{
    m_vertexShader =
            "attribute highp vec4 a_position;\n"
            "uniform highp mat4 modelViewProjectionMatrix;\n"
            "void main(void) \n"
            "{ \n"
            "   gl_Position = modelViewProjectionMatrix * a_position;\n"
            "}\n";

    m_fragmentShader =
            "void main(void)\n"
            "{\n "
            "  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0); \n"
            "}\n";
}

CPBox::CPBox() :
    m_funcs(0),
    m_program(0)
{
    setShaders();
}

void CPBox::render(QMatrix4x4 &modelViewProjectionMatrix)
{
    if(m_vertices.size() == 0) return;
    ensureInitialized();
    uploadVBO();

    CPTimer::rendering().start();
    m_program->bind();

    m_program->setUniformValue("modelViewProjectionMatrix", modelViewProjectionMatrix);

    // Tell OpenGL which VBOs to use
    m_funcs->glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
    m_funcs->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIds[1]);

    // Offset for position
    quintptr offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = m_program->attributeLocation("a_position");
    m_program->enableAttributeArray(vertexLocation);
    m_funcs->glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(CPPoint), (const void *)offset);

    glDisable(GL_BLEND);

    // Draw cube geometry using indices from VBO 1
    glDisable(GL_CULL_FACE);
    CPTimer::drawElements().start();
    m_funcs->glDrawElements(GL_QUADS, m_indices.size(), GL_UNSIGNED_SHORT, 0);
    CPTimer::drawElements().stop();

    m_program->release();
    CPTimer::rendering().stop();
}

CPBox::~CPBox()
{
    if(m_funcs)  delete m_funcs;
    if(m_program) delete m_program;
}



void CPBox::update(QVector3D origin, QVector3D size)
{
    float x0 = origin.x();
    float x1 = x0 + size.x();

    float y0 = origin.y();
    float y1 = y0 + size.y();

    float z0 = origin.z();
    float z1 = z0 + size.z();

    m_vertices.clear();
    m_indices.clear();

    m_vertices.push_back(CPPoint(QVector3D(x0, y0, z0)));
    m_vertices.push_back(CPPoint(QVector3D(x1, y0, z0)));
    m_vertices.push_back(CPPoint(QVector3D(x1, y0, z1)));
    m_vertices.push_back(CPPoint(QVector3D(x0, y0, z1)));
    m_vertices.push_back(CPPoint(QVector3D(x0, y1, z0)));
    m_vertices.push_back(CPPoint(QVector3D(x1, y1, z0)));
    m_vertices.push_back(CPPoint(QVector3D(x1, y1, z1)));
    m_vertices.push_back(CPPoint(QVector3D(x0, y1, z1)));

    m_indices = {0, 1, 2, 3, 1, 5, 6, 2, 4, 0, 3, 7, 5, 4, 7, 6};
}
