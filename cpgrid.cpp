#include "cpgrid.h"

void CPGrid::zeros()
{
    for_each([&](CPPoint &p) {
        p.position.setZ(0);
    });
}

std::vector<CPPoint> CPGrid::vertices() const
{
    return m_vertices;
}

void CPGrid::setVertices(const std::vector<CPPoint> &vertices)
{
    m_vertices = vertices;
}
void CPGrid::createShaderProgram()
{
    if (!m_program) {
        m_program = new QOpenGLShaderProgram();

        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "attribute highp vec4 a_position;\n"
                                           "attribute highp vec3 a_normal;\n"
                                           "uniform highp mat4 modelViewProjectionMatrix;\n"
                                           "varying highp vec3 normal;\n"
                                           "varying highp vec3 mypos;\n"
                                           "void main() {\n"
                                           "    gl_Position = modelViewProjectionMatrix*a_position;\n"
                                           "    normal = a_normal.xyz;\n"
                                           "    mypos = a_position.xyz;\n"
                                           "}");

        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "uniform vec3 lightpos; \n"
                                           "uniform vec3 targetdir; \n"
                                           "varying highp vec3 normal;"
                                           "varying highp vec3 mypos;\n"
                                           "void main() {\n"
                                           "  vec4 val = vec4(0.2,0.25,1.0,1.0);\n"
                                           "  gl_FragColor = val;\n"
//                                           "  float light = clamp(dot(normalize(lightpos), normal), 0.0, 1.0);\n"
//                                           "  float shininess = 40.0;"
//                                           "  float specular = pow(clamp(dot(reflect(-normalize(lightpos), normal), targetdir), 0.0, 1.0), shininess);"
//                                           "  gl_FragColor = val*light + specular*vec4(1,1,1,1); \n"
//                                           "  gl_FragColor.w = 0.7;"
                                           "}");


        m_program->link();
    }
}

void CPGrid::generateVBOs()
{
    ensureInitialized();
    m_funcs->glGenBuffers(2, m_vboIds);
}

void CPGrid::ensureInitialized()
{
    if(!m_funcs) m_funcs = new QOpenGLFunctions(QOpenGLContext::currentContext());
}

CPGrid::CPGrid() :
    m_gridSize(0),
    m_funcs(0),
    m_program(0)
{

}

CPGrid::~CPGrid() {
    if(m_funcs)  delete m_funcs;
    if(m_program) delete m_program;
    m_vertices.clear();
    m_indices.clear();
    m_triangles.clear();
}

void CPGrid::for_each(std::function<void (int, int)> action)
{
    for(int i=0; i<gridSize(); i++) {
        for(int j=0; j<gridSize(); j++) {
            action(i,j);
        }
    }
}

void CPGrid::for_each(std::function<void (CPPoint &p)> action)
{
    for(CPPoint &p : m_vertices) {
        action(p);
    }
}

void CPGrid::for_each(std::function<void (CPPoint &p, int i, int j)> action)
{
    for(int i=0; i<gridSize(); i++) {
        for(int j=0; j<gridSize(); j++) {
            action(m_vertices[index(i,j)], i, j);
        }
    }
}

void CPGrid::resize(int gridSize, float rMin, float rMax)
{
    m_gridSize = gridSize;
    float length = rMax-rMin;
    float dr = length / (gridSize - 1);
    int numTriangles = 2*(gridSize-1)*(gridSize-1);
    int numIndices = 3*numTriangles;

    m_triangles.reserve(numTriangles);
    m_indices.reserve(numIndices);
    m_vertices.resize(gridSize*gridSize);

    for_each([&](CPPoint &p, int i, int j) {
        p.position.setX(rMin + dr*i);
        p.position.setY(rMin + dr*j);
        p.position.setZ(0);
    });

    m_indices.clear();
    m_triangles.clear();
    for_each([&](int i, int j) {
        // Skip the end points
        if(i<gridSize - 1 && j<gridSize - 1) {
            // Triangle 1
            int index1 = index(i,j);
            int index2 = index(i,j+1);
            int index3 = index(i+1,j);
            m_indices.push_back(index1);
            m_indices.push_back(index2);
            m_indices.push_back(index3);
            CPTriangle triangle1(&m_vertices[index1], &m_vertices[index2], &m_vertices[index3]);
            m_triangles.push_back(triangle1);

            // Triangle 2
            index1 = index(i+1,j);
            index2 = index(i,j+1);
            index3 = index(i+1,j+1);
            m_indices.push_back(index1);
            m_indices.push_back(index2);
            m_indices.push_back(index3);
            CPTriangle triangle2(&m_vertices[index1], &m_vertices[index2], &m_vertices[index3]);
            m_triangles.push_back(triangle2);
        }
    });
}

void CPGrid::calculateNormals() {
    for(CPTriangle &triangle : m_triangles) {
        triangle.calculateNormal();
    }
}

void CPGrid::uploadVBO() {
    ensureInitialized();

    // Transfer vertex data to VBO 0
    m_funcs->glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
    m_funcs->glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(CPPoint), &m_vertices[0], GL_STATIC_DRAW);

    // Transfer index data to VBO 1
    m_funcs->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIds[1]);
    m_funcs->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLushort), &m_indices[0], GL_STATIC_DRAW);
}

void CPGrid::renderAsTriangles(QMatrix4x4 &modelViewProjectionMatrix, QMatrix4x4 &modelViewMatrix) {
    if(m_vertices.size() == 0) return;
    ensureInitialized();
    uploadVBO();

    createShaderProgram();
    m_program->bind();

    QVector3D cameraDirection;
    cameraDirection.setX(-modelViewMatrix(2,0));
    cameraDirection.setY(-modelViewMatrix(2,1));
    cameraDirection.setZ(-modelViewMatrix(2,2));
    cameraDirection.normalize();

    m_program->setUniformValue("modelViewProjectionMatrix", modelViewProjectionMatrix);
    m_program->setUniformValue("targetdir", cameraDirection);
    m_program->setUniformValue("lightpos",  QVector3D(1,1,1));

    // Tell OpenGL which VBOs to use
    m_funcs->glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
    m_funcs->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIds[1]);

    // Offset for position
    quintptr offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = m_program->attributeLocation("a_position");
    m_program->enableAttributeArray(vertexLocation);
    m_funcs->glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(CPPoint), (const void *)offset);

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex color data
    int normalLocation = m_program->attributeLocation("a_normal");
    m_program->enableAttributeArray(normalLocation);
    m_funcs->glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(CPPoint), (const void *)offset);

    // Draw cube geometry using indices from VBO 1
    m_funcs->glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_SHORT, 0);


    // m_program->release();
}
