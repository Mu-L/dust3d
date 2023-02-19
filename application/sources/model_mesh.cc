#include "model_mesh.h"
#include "version.h"
#include <QFile>
#include <QTextStream>
#include <assert.h>
#include <cmath>

float ModelMesh::m_defaultMetalness = 0.0;
float ModelMesh::m_defaultRoughness = 1.0;

ModelMesh::ModelMesh(const ModelMesh& mesh)
    : m_triangleVertices(nullptr)
    , m_triangleVertexCount(0)
    , m_textureImage(nullptr)
{
    if (nullptr != mesh.m_triangleVertices && mesh.m_triangleVertexCount > 0) {
        this->m_triangleVertices = new ModelOpenGLVertex[mesh.m_triangleVertexCount];
        this->m_triangleVertexCount = mesh.m_triangleVertexCount;
        for (int i = 0; i < mesh.m_triangleVertexCount; i++)
            this->m_triangleVertices[i] = mesh.m_triangleVertices[i];
    }
    if (nullptr != mesh.m_textureImage) {
        this->m_textureImage = new QImage(*mesh.m_textureImage);
    }
    if (nullptr != mesh.m_normalMapImage) {
        this->m_normalMapImage = new QImage(*mesh.m_normalMapImage);
    }
    if (nullptr != mesh.m_metalnessRoughnessAmbientOcclusionMapImage) {
        this->m_metalnessRoughnessAmbientOcclusionMapImage = new QImage(*mesh.m_metalnessRoughnessAmbientOcclusionMapImage);
        this->m_hasMetalnessInImage = mesh.m_hasMetalnessInImage;
        this->m_hasRoughnessInImage = mesh.m_hasRoughnessInImage;
        this->m_hasAmbientOcclusionInImage = mesh.m_hasAmbientOcclusionInImage;
    }
    this->m_vertices = mesh.m_vertices;
    this->m_faces = mesh.m_faces;
    this->m_triangulatedVertices = mesh.m_triangulatedVertices;
    this->m_meshId = mesh.meshId();
}

void ModelMesh::removeColor()
{
    delete this->m_textureImage;
    this->m_textureImage = nullptr;

    delete this->m_normalMapImage;
    this->m_normalMapImage = nullptr;

    delete this->m_metalnessRoughnessAmbientOcclusionMapImage;
    this->m_metalnessRoughnessAmbientOcclusionMapImage = nullptr;

    this->m_hasMetalnessInImage = false;
    this->m_hasRoughnessInImage = false;
    this->m_hasAmbientOcclusionInImage = false;

    for (int i = 0; i < this->m_triangleVertexCount; ++i) {
        auto& vertex = this->m_triangleVertices[i];
        vertex.colorR = 1.0;
        vertex.colorG = 1.0;
        vertex.colorB = 1.0;
    }
}

ModelMesh::ModelMesh(ModelOpenGLVertex* triangleVertices, int vertexNum)
    : m_triangleVertices(triangleVertices)
    , m_triangleVertexCount(vertexNum)
    , m_textureImage(nullptr)
{
}

ModelMesh::ModelMesh(const std::vector<dust3d::Vector3>& vertices,
    const std::vector<std::vector<size_t>>& triangles,
    const std::vector<std::vector<dust3d::Vector3>>& triangleVertexNormals,
    const dust3d::Color& color,
    float metalness,
    float roughness,
    const std::vector<std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/>>* vertexProperties,
    const std::vector<std::array<dust3d::Vector2, 3>>* triangleUvs)
{
    m_triangleVertexCount = (int)triangles.size() * 3;
    m_triangleVertices = new ModelOpenGLVertex[m_triangleVertexCount];
    int destIndex = 0;
    for (size_t i = 0; i < triangles.size(); ++i) {
        std::array<dust3d::Vector2, 3> uvs = {};
        if (nullptr != triangleUvs) {
            uvs = triangleUvs->at(i);
        }
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = (int)triangles[i][j];
            const dust3d::Vector3* srcVert = &vertices[vertexIndex];
            const dust3d::Vector3* srcNormal = &(triangleVertexNormals)[i][j];
            ModelOpenGLVertex* dest = &m_triangleVertices[destIndex];
            dest->posX = srcVert->x();
            dest->posY = srcVert->y();
            dest->posZ = srcVert->z();
            dest->texU = uvs[j][0];
            dest->texV = uvs[j][1];
            dest->normX = srcNormal->x();
            dest->normY = srcNormal->y();
            dest->normZ = srcNormal->z();
            if (nullptr == vertexProperties) {
                dest->colorR = color.r();
                dest->colorG = color.g();
                dest->colorB = color.b();
                dest->alpha = color.alpha();
                dest->metalness = metalness;
                dest->roughness = roughness;
            } else {
                const auto& property = (*vertexProperties)[vertexIndex];
                dest->colorR = std::get<0>(property).r();
                dest->colorG = std::get<0>(property).g();
                dest->colorB = std::get<0>(property).b();
                dest->alpha = std::get<0>(property).alpha();
                dest->metalness = std::get<1>(property);
                dest->roughness = std::get<2>(property);
            }
            dest->tangentX = 0;
            dest->tangentY = 0;
            dest->tangentZ = 0;
            destIndex++;
        }
    }
}

ModelMesh::ModelMesh(dust3d::Object& object)
    : m_triangleVertices(nullptr)
    , m_triangleVertexCount(0)
    , m_textureImage(nullptr)
{
    m_meshId = object.meshId;
    m_vertices = object.vertices;
    m_faces = object.triangleAndQuads;

    m_triangleVertexCount = (int)object.triangles.size() * 3;
    m_triangleVertices = new ModelOpenGLVertex[m_triangleVertexCount];
    int destIndex = 0;
    const auto triangleVertexNormals = object.triangleVertexNormals();
    const auto triangleVertexUvs = object.triangleVertexUvs();
    const auto triangleTangents = object.triangleTangents();
    const dust3d::Vector3 defaultNormal = dust3d::Vector3(0, 0, 0);
    const dust3d::Vector2 defaultUv = dust3d::Vector2(0, 0);
    const dust3d::Vector3 defaultTangent = dust3d::Vector3(0, 0, 0);
    for (size_t i = 0; i < object.triangles.size(); ++i) {
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = (int)object.triangles[i][j];
            const dust3d::Vector3* srcVert = &object.vertices[vertexIndex];
            const dust3d::Color* srcColor = &object.vertexColors[vertexIndex];
            const dust3d::Vector3* srcNormal = &defaultNormal;
            if (triangleVertexNormals)
                srcNormal = &(*triangleVertexNormals)[i][j];
            const dust3d::Vector2* srcUv = &defaultUv;
            if (triangleVertexUvs)
                srcUv = &(*triangleVertexUvs)[i][j];
            const dust3d::Vector3* srcTangent = &defaultTangent;
            if (triangleTangents)
                srcTangent = &(*triangleTangents)[i];
            ModelOpenGLVertex* dest = &m_triangleVertices[destIndex];
            dest->colorR = srcColor->r();
            dest->colorG = srcColor->g();
            dest->colorB = srcColor->b();
            dest->alpha = srcColor->alpha();
            dest->posX = srcVert->x();
            dest->posY = srcVert->y();
            dest->posZ = srcVert->z();
            dest->texU = srcUv->x();
            dest->texV = srcUv->y();
            dest->normX = srcNormal->x();
            dest->normY = srcNormal->y();
            dest->normZ = srcNormal->z();
            //auto findNode = nodeMap.find(object.vertexSourceNodes[vertexIndex]);
            //if (findNode != nodeMap.end()) {
            //    dest->metalness = findNode->second->metalness;
            //    dest->roughness = findNode->second->roughness;
            //} else {
            dest->metalness = m_defaultMetalness;
            dest->roughness = m_defaultRoughness;
            //}
            dest->tangentX = srcTangent->x();
            dest->tangentY = srcTangent->y();
            dest->tangentZ = srcTangent->z();
            destIndex++;
        }
    }
}

ModelMesh::ModelMesh()
    : m_triangleVertices(nullptr)
    , m_triangleVertexCount(0)
    , m_textureImage(nullptr)
{
}

ModelMesh::~ModelMesh()
{
    delete[] m_triangleVertices;
    m_triangleVertexCount = 0;
    delete m_textureImage;
    delete m_normalMapImage;
    delete m_metalnessRoughnessAmbientOcclusionMapImage;
}

const std::vector<dust3d::Vector3>& ModelMesh::vertices()
{
    return m_vertices;
}

const std::vector<std::vector<size_t>>& ModelMesh::faces()
{
    return m_faces;
}

const std::vector<dust3d::Vector3>& ModelMesh::triangulatedVertices()
{
    return m_triangulatedVertices;
}

ModelOpenGLVertex* ModelMesh::triangleVertices()
{
    return m_triangleVertices;
}

int ModelMesh::triangleVertexCount()
{
    return m_triangleVertexCount;
}

void ModelMesh::setTextureImage(QImage* textureImage)
{
    m_textureImage = textureImage;
}

const QImage* ModelMesh::textureImage()
{
    return m_textureImage;
}

QImage* ModelMesh::takeTextureImage()
{
    auto image = m_textureImage;
    m_textureImage = nullptr;
    return image;
}

void ModelMesh::setNormalMapImage(QImage* normalMapImage)
{
    m_normalMapImage = normalMapImage;
}

const QImage* ModelMesh::normalMapImage()
{
    return m_normalMapImage;
}

QImage* ModelMesh::takeNormalMapImage()
{
    auto image = m_normalMapImage;
    m_normalMapImage = nullptr;
    return image;
}

const QImage* ModelMesh::metalnessRoughnessAmbientOcclusionMapImage()
{
    return m_metalnessRoughnessAmbientOcclusionMapImage;
}

QImage* ModelMesh::takeMetalnessRoughnessAmbientOcclusionMapImage()
{
    auto image = m_metalnessRoughnessAmbientOcclusionMapImage;
    m_metalnessRoughnessAmbientOcclusionMapImage = nullptr;
    return image;
}

void ModelMesh::setMetalnessRoughnessAmbientOcclusionMapImage(QImage* image)
{
    m_metalnessRoughnessAmbientOcclusionMapImage = image;
}

bool ModelMesh::hasMetalnessInImage()
{
    return m_hasMetalnessInImage;
}

void ModelMesh::setHasMetalnessInImage(bool hasInImage)
{
    m_hasMetalnessInImage = hasInImage;
}

bool ModelMesh::hasRoughnessInImage()
{
    return m_hasRoughnessInImage;
}

void ModelMesh::setHasRoughnessInImage(bool hasInImage)
{
    m_hasRoughnessInImage = hasInImage;
}

bool ModelMesh::hasAmbientOcclusionInImage()
{
    return m_hasAmbientOcclusionInImage;
}

void ModelMesh::setHasAmbientOcclusionInImage(bool hasInImage)
{
    m_hasAmbientOcclusionInImage = hasInImage;
}

void ModelMesh::exportAsObj(QTextStream* textStream)
{
    auto& stream = *textStream;
    stream << "# " << APP_NAME << " " << APP_HUMAN_VER << "\n";
    stream << "# " << APP_HOMEPAGE_URL << "\n";
    for (std::vector<dust3d::Vector3>::const_iterator it = vertices().begin(); it != vertices().end(); ++it) {
        stream << "v " << QString::number((*it).x()) << " " << QString::number((*it).y()) << " " << QString::number((*it).z()) << "\n";
    }
    for (std::vector<std::vector<size_t>>::const_iterator it = faces().begin(); it != faces().end(); ++it) {
        stream << "f";
        for (std::vector<size_t>::const_iterator subIt = (*it).begin(); subIt != (*it).end(); ++subIt) {
            stream << " " << QString::number((1 + *subIt));
        }
        stream << "\n";
    }
}

void ModelMesh::exportAsObj(const QString& filename)
{
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        exportAsObj(&stream);
    }
}

void ModelMesh::updateTriangleVertices(ModelOpenGLVertex* triangleVertices, int triangleVertexCount)
{
    delete[] m_triangleVertices;
    m_triangleVertices = 0;
    m_triangleVertexCount = 0;

    m_triangleVertices = triangleVertices;
    m_triangleVertexCount = triangleVertexCount;
}

quint64 ModelMesh::meshId() const
{
    return m_meshId;
}

void ModelMesh::setMeshId(quint64 id)
{
    m_meshId = id;
}
