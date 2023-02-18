/*
 *  Copyright (c) 2016-2022 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef DUST3D_MESH_STITCH_MESH_BUILDER_H_
#define DUST3D_MESH_STITCH_MESH_BUILDER_H_

#include <dust3d/base/uuid.h>
#include <dust3d/base/vector3.h>
#include <dust3d/mesh/mesh_node.h>

namespace dust3d {

class StitchMeshBuilder {
public:
    struct Spline {
        std::vector<MeshNode> nodes;
        Uuid sourceId;
    };

    StitchMeshBuilder(std::vector<Spline>&& splines, bool frontClosed, bool backClosed, bool sideClosed, size_t targetSegments);
    void build();
    const std::vector<Vector3>& generatedVertices() const;
    const std::vector<Uuid>& generatedVertexSources() const;
    const std::vector<std::vector<size_t>>& generatedFaces() const;
    const std::vector<std::vector<Vector2>>& generatedFaceUvs() const;
    const std::vector<Spline>& splines() const;

private:
    std::vector<Spline> m_splines;
    std::vector<Vector3> m_generatedVertices;
    std::vector<Uuid> m_generatedVertexSources;
    std::vector<std::vector<size_t>> m_generatedFaces;
    std::vector<std::vector<Vector2>> m_generatedFaceUvs;
    size_t m_targetSegments = 0;
    bool m_sideClosed = false;
    bool m_frontClosed = false;
    bool m_backClosed = false;

    bool interpolateSplinesToHaveEqualSizeOfNodes();
    void splitPolylineToSegments(const std::vector<Vector3>& polyline,
        const std::vector<double>& radiuses,
        size_t targetSegments,
        std::vector<Vector3>* targetPoints,
        std::vector<double>* targetRadiuses);
    double segmentsLength(const std::vector<Vector3>& segmentPoints);
};

}

#endif
