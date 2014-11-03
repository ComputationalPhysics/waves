#include "cpgrid.h"
#include "perlinnoise.h"
#include "cptimer.h"
#include <cmath>
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

CPGrid::CPGrid() :
    m_gridSize(0),
    m_funcs(0),
    m_program(0),
    m_indicesDirty(true)
{
    m_gridType = GridType::Water;
    setShaders();
}

CPGrid::~CPGrid() {
    if(m_funcs)  delete m_funcs;
    if(m_program) delete m_program;
    m_vertices.clear();
    m_indices.clear();
    m_triangles.clear();
}

void CPGrid::zeros()
{
    for_each([&](CPPoint &p) {
        p.position.setZ(0);
    });
}

std::vector<CPPoint> &CPGrid::vertices()
{
    return m_vertices;
}

void CPGrid::setVertices(const std::vector<CPPoint> &vertices)
{
    m_vertices = vertices;
}


GridType CPGrid::getGridType() const
{
    return m_gridType;
}

void CPGrid::setGridType(const GridType &gridType)
{
    m_gridType = gridType;
    if(m_program) {
        delete m_program;
        m_program = 0;
        createShaderProgram();
    }
}

void CPGrid::createShaderProgram()
{
    if (!m_program) {
        m_program = new QOpenGLShaderProgram();

        if(m_gridType == GridType::Water) {
            m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, m_waterVertexShader);
            m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, m_waterFragmentShader);
        } else if(m_gridType == GridType::Ground) {
            m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, m_groundVertexShader);
            m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, m_groundFragmentShader);
        } else {
            qDebug() << "Warning, tried to create shader of unknown type." << endl;
            exit(1);
        }

        m_program->link();
    }
}

void CPGrid::generateVBOs()
{
    m_funcs->glGenBuffers(2, m_vboIds);
}

void CPGrid::ensureInitialized()
{
    if(!m_funcs) {
        m_funcs = new QOpenGLFunctions(QOpenGLContext::currentContext());
        generateVBOs();
        createShaderProgram();
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

void CPGrid::for_each(std::function<void (CPPoint &p, int i, int j, int gridSize)> action)
{
    for(int i=0; i<gridSize(); i++) {
        for(int j=0; j<gridSize(); j++) {
            action(m_vertices[index(i,j)], i, j, gridSize());
        }
    }
}

void CPGrid::for_each(std::function<void (CPPoint &p, int i, int j, int gridSize, int index)> action)
{
    for(int i=0; i<gridSize(); i++) {
        for(int j=0; j<gridSize(); j++) {
            action(m_vertices[index(i,j)], i, j, gridSize(), index(i,j));
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
    m_z.resize(gridSize*gridSize);

    for_each([&](CPPoint &p, int i, int j) {
        p.position.setX(rMin + dr*i);
        p.position.setY(rMin + dr*j);
        p.position.setZ(0);
    });

    m_indices.clear();
    m_triangles.clear();
    for_each([&](CPPoint &p, int i, int j) {
        // Skip the end points
        if(i<gridSize - 1 && j<gridSize - 1) {
            // Triangle 1
            int index1 = index(i,j);
            int index2 = index(i,j+1);
            int index3 = index(i+1,j);
            m_indices.push_back(index1);
            m_indices.push_back(index2);
            m_indices.push_back(index3);
            CPTriangle triangle1(index1, index2, index3);
            m_triangles.push_back(triangle1);

            // Triangle 2
            index1 = index(i+1,j);
            index2 = index(i,j+1);
            index3 = index(i+1,j+1);
            m_indices.push_back(index1);
            m_indices.push_back(index2);
            m_indices.push_back(index3);
            CPTriangle triangle2(index1, index2, index3);
            m_triangles.push_back(triangle2);
        }
    });

    m_indicesDirty = true;
}

#if defined(__ARM_NEON)
void CPGrid::calculateNormals() {
    CPTimer::normalVectors().start();
    for(CPTriangle &triangle : m_triangles) {
        triangle.calculateNormal(m_vertices);
    }
    CPTimer::normalVectors().stop();
}

#else
void CPGrid::calculateNormals() {
    CPTimer::normalVectors().start();
    for(CPTriangle &triangle : m_triangles) {
        triangle.calculateNormal(m_vertices);
    }
    CPTimer::normalVectors().stop();
}
#endif

void CPGrid::uploadVBO() {
    ensureInitialized();
    if(m_gridType == GridType::Water) calculateNormals();

    CPTimer::uploadVBO().start();
    // Transfer vertex data to VBO 0
    m_funcs->glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
    m_funcs->glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(CPPoint), &m_vertices[0], GL_STATIC_DRAW);

    if(m_indicesDirty) {
        qDebug() << "Uploading " << m_indices.size() << " indices with total size " << m_indices.size() *sizeof(GLushort) << " bytes.";
        qDebug() << "This corresponds to " << m_vertices.size() << " vertices with total size " << m_vertices.size() *sizeof(CPPoint) << " bytes.";
        // Transfer index data to VBO 1
        m_funcs->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIds[1]);
        m_funcs->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLushort), &m_indices[0], GL_STATIC_DRAW);

        CPTimer::uploadVBO().stop();
        m_indicesDirty = false;
    }
}

void CPGrid::setShaders()
{
    m_waterVertexShader =
            "uniform highp mat4 modelViewProjectionMatrix;\n"
            "uniform highp mat4 modelViewMatrix;\n"
            "uniform highp vec3 lightpos; \n"
            "attribute highp vec4 a_position;\n"
            "attribute highp vec3 a_normal;\n"
            "varying highp vec3 normal; \n"
            "varying highp vec3 lightDirection;\n"
            "void main(void) \n"
            "{ \n"
            "   vec4 modelViewPosition = modelViewMatrix * a_position;\n"
            "   lightDirection = vec4(lightpos, 1.0) - modelViewPosition;\n"
            "	normal = a_normal;\n"
            "   gl_Position = modelViewProjectionMatrix * a_position;\n"
            "}\n";

    m_waterFragmentShader =
            "uniform highp vec3 targetdir; \n"
            "varying highp vec3 normal; \n"
            "varying highp vec3 lightDirection;\n"
            "void main(void)\n"
            "{\n "
            "  highp vec4 val = vec4(0.2,0.25,1.0,1.0);\n"
            "  highp float light = clamp(dot(normalize(lightDirection), normalize(normal)), 0.0, 1.0);\n"
            "  highp float shininess = 40.0;"
            "  highp float specular = pow(clamp(dot(reflect(-normalize(lightDirection), normalize(normal)), targetdir), 0.0, 1.0), shininess);"
            "  gl_FragColor = val*light + specular*vec4(1,1,1,1); \n"
            "  gl_FragColor.w = 0.7;"
            "}";

    m_groundVertexShader =
            "uniform highp mat4 modelViewProjectionMatrix;\n"
            "uniform highp mat4 modelViewMatrix;\n"
            "uniform highp vec3 lightpos; \n"
            "attribute highp vec4 a_position;\n"
            "attribute highp vec3 a_normal;\n"
            "varying highp vec3 normal; \n"
            "varying highp vec3 lightDirection;\n"
            "void main(void) \n"
            "{ \n"
            "   vec4 modelViewPosition = modelViewMatrix * a_position;\n"
            "   lightDirection = vec4(lightpos, 1.0) - modelViewPosition;\n"
            "	normal = a_normal;\n"
            "   gl_Position = modelViewProjectionMatrix * a_position;\n"
            "}\n";

    m_groundFragmentShader =
            "varying highp vec3 normal; \n"
            "varying highp vec3 lightDirection;\n"
            "void main(void)\n"
            "{\n "
            "  highp vec4 val = vec4(0.7,0.5,0.3,1);"
            "  highp float light = clamp(dot(normalize(lightDirection), normalize(normal)), 0.0, 1.0);"
            "  gl_FragColor = vec4(val.rgb*light, 1.0); \n"
            "}\n";
}

void CPGrid::renderAsTriangles(QMatrix4x4 &modelViewProjectionMatrix, QMatrix4x4 &modelViewMatrix) {
    if(m_vertices.size() == 0) return;
    ensureInitialized();
    uploadVBO();

    CPTimer::rendering().start();
    m_program->bind();

    QVector3D cameraDirection;
    QVector3D lightPos(2,2,2);
    cameraDirection.setX(-modelViewMatrix(2,0));
    cameraDirection.setY(-modelViewMatrix(2,1));
    cameraDirection.setZ(-modelViewMatrix(2,2));
    cameraDirection.normalize();

    lightPos.setX(modelViewMatrix(0,0));
    lightPos.setY(modelViewMatrix(1,1));
    lightPos.setZ(modelViewMatrix(2,2));

    m_program->setUniformValue("modelViewProjectionMatrix", modelViewProjectionMatrix);
    m_program->setUniformValue("modelViewMatrix", modelViewMatrix);
    m_program->setUniformValue("targetdir", cameraDirection);
    m_program->setUniformValue("lightpos",  lightPos);

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Draw cube geometry using indices from VBO 1
    CPTimer::drawElements().start();
    m_funcs->glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_SHORT, 0);
    CPTimer::drawElements().stop();
    glDisable(GL_BLEND);

    m_program->release();
    CPTimer::rendering().stop();
}

void CPGrid::createPerlin(unsigned int seed, float amplitude, float lengthScale, float deltaZ)
{
    PerlinNoise perlin(seed);

    for_each([&](CPPoint &p, int i, int j, int gridSize) {
        float x = i/float(gridSize);
        float y = j/float(gridSize);

        float z = amplitude*(perlin.noise(x*lengthScale,y*lengthScale,0)) + deltaZ;
        if(i==0 || i == gridSize-1 || j==0 || j==gridSize-1) {
            z = 0.2;
        }
        p.position.setZ(z);
    });

    calculateNormals();
}

void CPGrid::createDoubleSlit()
{
    int slitSize = 3;
    for_each([&](CPPoint &p, int i, int j, int gridSize) {
        bool wall = i==0 || i==gridSize-1 || j==0 || j==gridSize-1;
        int slit1 = gridSize/2 + 6;
        int slit2 = gridSize/2 - 6;

//        wall |= (j==gridSize/2) && (abs(i-slit1)>=slitSize & abs(i-slit2)>=slitSize);
        wall |= (j==gridSize/2);

        float z = wall ? 0.1 : -5;
        p.position.setZ(z);
    });

    calculateNormals();
}

void CPGrid::createSinus()
{
    for_each([&](CPPoint &p, int i, int j, int gridSize) {
        float y = j/float(gridSize)*2*3.1415;
        float omega = 1.0;
        float x0 = 0.1*sin(y*omega);
        float x = (i-gridSize/2) / float(gridSize);
        bool wall = i==0 || i==gridSize-1 || j==0 || j==gridSize-1;
        wall |= fabs(x-x0) > 0.05;

        float z = wall ? 0.2 : -0.5;
        p.position.setZ(z);
    });

    calculateNormals();
}

void CPGrid::swapWithGrid(CPGrid &grid)
{
    m_vertices.swap(grid.vertices());
}

void CPGrid::updateGridFromZ()
{
    for_each([&](CPPoint &p, int i, int j, int gridSize, int index) {
        p.position.setZ(m_z[index]);
    });
}

void CPGrid::updateZFromGrid()
{
    for_each([&](CPPoint &p, int i, int j, int gridSize, int index) {
        m_z[index] = p.position.z();
    });
}
