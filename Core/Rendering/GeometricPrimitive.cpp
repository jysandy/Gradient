#include "pch.h"
#include "Core/Rendering/GeometricPrimitive.h"
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <map>

using namespace DirectX::SimpleMath;

namespace Gradient::Rendering
{

    // -------------------------------------------------------------------------------
    // Geometry building code
    // Taken from https://github.com/microsoft/DirectXTK/blob/main/Src/Geometry.cpp
    // -------------------------------------------------------------------------------


    inline void CheckIndexOverflow(size_t value)
    {
        // Use >=, not > comparison, because some D3D level 9_x hardware does not support 0xFFFF index values.
        if (value >= USHRT_MAX)
            throw std::out_of_range("Index value out of range: cannot tesselate primitive so finely");
    }


    // Collection types used when generating the geometry.
    inline void index_push_back(GeometricPrimitive::IndexCollection& indices, size_t value)
    {
        CheckIndexOverflow(value);
        indices.push_back(static_cast<uint16_t>(value));
    }


    // Helper for flipping winding of geometric primitives for LH vs. RH coords
    inline void ReverseWinding(GeometricPrimitive::IndexCollection& indices,
        GeometricPrimitive::VertexCollection& vertices)
    {
        assert((indices.size() % 3) == 0);
        for (auto it = indices.begin(); it != indices.end(); it += 3)
        {
            std::swap(*it, *(it + 2));
        }

        for (auto& it : vertices)
        {
            it.textureCoordinate.x = (1.f - it.textureCoordinate.x);
        }
    }


    // Helper for inverting normals of geometric primitives for 'inside' vs. 'outside' viewing
    inline void InvertNormals(GeometricPrimitive::VertexCollection& vertices)
    {
        for (auto& it : vertices)
        {
            it.normal.x = -it.normal.x;
            it.normal.y = -it.normal.y;
            it.normal.z = -it.normal.z;
        }
    }

    void ComputeBox(GeometricPrimitive::VertexCollection& vertices,
        GeometricPrimitive::IndexCollection& indices,
        const DirectX::XMFLOAT3& size,
        bool rhcoords,
        bool invertn)
    {
        using namespace DirectX;
        vertices.clear();
        indices.clear();

        // A box has six faces, each one pointing in a different direction.
        constexpr int FaceCount = 6;

        static const XMVECTORF32 faceNormals[FaceCount] =
        {
            { { {  0,  0,  1, 0 } } },
            { { {  0,  0, -1, 0 } } },
            { { {  1,  0,  0, 0 } } },
            { { { -1,  0,  0, 0 } } },
            { { {  0,  1,  0, 0 } } },
            { { {  0, -1,  0, 0 } } },
        };

        static const XMVECTORF32 textureCoordinates[4] =
        {
            { { { 1, 0, 0, 0 } } },
            { { { 1, 1, 0, 0 } } },
            { { { 0, 1, 0, 0 } } },
            { { { 0, 0, 0, 0 } } },
        };

        XMVECTOR tsize = XMLoadFloat3(&size);
        tsize = XMVectorDivide(tsize, g_XMTwo);

        // Create each face in turn.
        for (int i = 0; i < FaceCount; i++)
        {
            const XMVECTOR normal = faceNormals[i];

            // Get two vectors perpendicular both to the face normal and to each other.
            const XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

            const XMVECTOR side1 = XMVector3Cross(normal, basis);
            const XMVECTOR side2 = XMVector3Cross(normal, side1);

            // Six indices (two triangles) per face.
            const size_t vbase = vertices.size();
            index_push_back(indices, vbase + 0);
            index_push_back(indices, vbase + 1);
            index_push_back(indices, vbase + 2);

            index_push_back(indices, vbase + 0);
            index_push_back(indices, vbase + 2);
            index_push_back(indices, vbase + 3);

            // Four vertices per face.
            // (normal - side1 - side2) * tsize // normal // t0
            vertices.push_back(VertexPositionNormalTexture(XMVectorMultiply(XMVectorSubtract(XMVectorSubtract(normal, side1), side2), tsize), normal, textureCoordinates[0]));

            // (normal - side1 + side2) * tsize // normal // t1
            vertices.push_back(VertexPositionNormalTexture(XMVectorMultiply(XMVectorAdd(XMVectorSubtract(normal, side1), side2), tsize), normal, textureCoordinates[1]));

            // (normal + side1 + side2) * tsize // normal // t2
            vertices.push_back(VertexPositionNormalTexture(XMVectorMultiply(XMVectorAdd(normal, XMVectorAdd(side1, side2)), tsize), normal, textureCoordinates[2]));

            // (normal + side1 - side2) * tsize // normal // t3
            vertices.push_back(VertexPositionNormalTexture(XMVectorMultiply(XMVectorSubtract(XMVectorAdd(normal, side1), side2), tsize), normal, textureCoordinates[3]));
        }

        // Build RH above
        if (!rhcoords)
            ReverseWinding(indices, vertices);

        if (invertn)
            InvertNormals(vertices);
    }

    void ComputeSphere(GeometricPrimitive::VertexCollection& vertices,
        GeometricPrimitive::IndexCollection& indices,
        float diameter,
        size_t tessellation,
        bool rhcoords,
        bool invertn)
    {
        using namespace DirectX;
        vertices.clear();
        indices.clear();

        if (tessellation < 3)
            throw std::invalid_argument("tesselation parameter must be at least 3");

        const size_t verticalSegments = tessellation;
        const size_t horizontalSegments = tessellation * 2;

        const float radius = diameter / 2;

        // Create rings of vertices at progressively higher latitudes.
        for (size_t i = 0; i <= verticalSegments; i++)
        {
            const float v = 1 - float(i) / float(verticalSegments);

            const float latitude = (float(i) * XM_PI / float(verticalSegments)) - XM_PIDIV2;
            float dy, dxz;

            XMScalarSinCos(&dy, &dxz, latitude);

            // Create a single ring of vertices at this latitude.
            for (size_t j = 0; j <= horizontalSegments; j++)
            {
                const float u = float(j) / float(horizontalSegments);

                const float longitude = float(j) * XM_2PI / float(horizontalSegments);
                float dx, dz;

                XMScalarSinCos(&dx, &dz, longitude);

                dx *= dxz;
                dz *= dxz;

                const XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);
                const XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

                vertices.push_back(VertexPositionNormalTexture(XMVectorScale(normal, radius), normal, textureCoordinate));
            }
        }

        // Fill the index buffer with triangles joining each pair of latitude rings.
        const size_t stride = horizontalSegments + 1;

        for (size_t i = 0; i < verticalSegments; i++)
        {
            for (size_t j = 0; j <= horizontalSegments; j++)
            {
                const size_t nextI = i + 1;
                const size_t nextJ = (j + 1) % stride;

                index_push_back(indices, i * stride + j);
                index_push_back(indices, nextI * stride + j);
                index_push_back(indices, i * stride + nextJ);

                index_push_back(indices, i * stride + nextJ);
                index_push_back(indices, nextI * stride + j);
                index_push_back(indices, nextI * stride + nextJ);
            }
        }

        // Build RH above
        if (!rhcoords)
            ReverseWinding(indices, vertices);

        if (invertn)
            InvertNormals(vertices);
    }

    void ComputeGeoSphere(GeometricPrimitive::VertexCollection& vertices,
        GeometricPrimitive::IndexCollection& indices,
        float diameter,
        size_t tessellation,
        bool rhcoords)
    {
        using namespace DirectX;
        vertices.clear();
        indices.clear();

        // An undirected edge between two vertices, represented by a pair of indexes into a vertex array.
        // Becuse this edge is undirected, (a,b) is the same as (b,a).
        using UndirectedEdge = std::pair<uint16_t, uint16_t>;

        // Makes an undirected edge. Rather than overloading comparison operators to give us the (a,b)==(b,a) property,
        // we'll just ensure that the larger of the two goes first. This'll simplify things greatly.
        auto makeUndirectedEdge = [](uint16_t a, uint16_t b) noexcept
            {
                return std::make_pair(std::max(a, b), std::min(a, b));
            };

        // Key: an edge
        // Value: the index of the vertex which lies midway between the two vertices pointed to by the key value
        // This map is used to avoid duplicating vertices when subdividing triangles along edges.
        using EdgeSubdivisionMap = std::map<UndirectedEdge, uint16_t>;


        static const XMFLOAT3 OctahedronVertices[] =
        {
            // when looking down the negative z-axis (into the screen)
            XMFLOAT3(0,  1,  0), // 0 top
            XMFLOAT3(0,  0, -1), // 1 front
            XMFLOAT3(1,  0,  0), // 2 right
            XMFLOAT3(0,  0,  1), // 3 back
            XMFLOAT3(-1,  0,  0), // 4 left
            XMFLOAT3(0, -1,  0), // 5 bottom
        };
        static const uint16_t OctahedronIndices[] =
        {
            0, 1, 2, // top front-right face
            0, 2, 3, // top back-right face
            0, 3, 4, // top back-left face
            0, 4, 1, // top front-left face
            5, 1, 4, // bottom front-left face
            5, 4, 3, // bottom back-left face
            5, 3, 2, // bottom back-right face
            5, 2, 1, // bottom front-right face
        };

        const float radius = diameter / 2.0f;

        // Start with an octahedron; copy the data into the vertex/index collection.

        std::vector<XMFLOAT3> vertexPositions(std::begin(OctahedronVertices), std::end(OctahedronVertices));

        indices.insert(indices.begin(), std::begin(OctahedronIndices), std::end(OctahedronIndices));

        // We know these values by looking at the above index list for the octahedron. Despite the subdivisions that are
        // about to go on, these values aren't ever going to change because the vertices don't move around in the array.
        // We'll need these values later on to fix the singularities that show up at the poles.
        constexpr uint16_t northPoleIndex = 0;
        constexpr uint16_t southPoleIndex = 5;

        for (size_t iSubdivision = 0; iSubdivision < tessellation; ++iSubdivision)
        {
            assert(indices.size() % 3 == 0); // sanity

            // We use this to keep track of which edges have already been subdivided.
            EdgeSubdivisionMap subdividedEdges;

            // The new index collection after subdivision.
            GeometricPrimitive::IndexCollection newIndices;

            const size_t triangleCount = indices.size() / 3;
            for (size_t iTriangle = 0; iTriangle < triangleCount; ++iTriangle)
            {
                // For each edge on this triangle, create a new vertex in the middle of that edge.
                // The winding order of the triangles we output are the same as the winding order of the inputs.

                // Indices of the vertices making up this triangle
                const uint16_t iv0 = indices[iTriangle * 3 + 0];
                const uint16_t iv1 = indices[iTriangle * 3 + 1];
                const uint16_t iv2 = indices[iTriangle * 3 + 2];

                // Get the new vertices
                XMFLOAT3 v01; // vertex on the midpoint of v0 and v1
                XMFLOAT3 v12; // ditto v1 and v2
                XMFLOAT3 v20; // ditto v2 and v0
                uint16_t iv01; // index of v01
                uint16_t iv12; // index of v12
                uint16_t iv20; // index of v20

                // Function that, when given the index of two vertices, creates a new vertex at the midpoint of those vertices.
                auto const divideEdge = [&](uint16_t i0, uint16_t i1, XMFLOAT3& outVertex, uint16_t& outIndex)
                    {
                        const UndirectedEdge edge = makeUndirectedEdge(i0, i1);

                        // Check to see if we've already generated this vertex
                        auto it = subdividedEdges.find(edge);
                        if (it != subdividedEdges.end())
                        {
                            // We've already generated this vertex before
                            outIndex = it->second; // the index of this vertex
                            outVertex = vertexPositions[outIndex]; // and the vertex itself
                        }
                        else
                        {
                            // Haven't generated this vertex before: so add it now

                            // outVertex = (vertices[i0] + vertices[i1]) / 2
                            XMStoreFloat3(
                                &outVertex,
                                XMVectorScale(
                                    XMVectorAdd(XMLoadFloat3(&vertexPositions[i0]), XMLoadFloat3(&vertexPositions[i1])),
                                    0.5f
                                )
                            );

                            outIndex = static_cast<uint16_t>(vertexPositions.size());
                            CheckIndexOverflow(outIndex);
                            vertexPositions.push_back(outVertex);

                            // Now add it to the map.
                            auto entry = std::make_pair(edge, outIndex);
                            subdividedEdges.insert(entry);
                        }
                    };

                // Add/get new vertices and their indices
                divideEdge(iv0, iv1, v01, iv01);
                divideEdge(iv1, iv2, v12, iv12);
                divideEdge(iv0, iv2, v20, iv20);

                // Add the new indices. We have four new triangles from our original one:
                //        v0
                //        o
                //       /a\
                //  v20 o---o v01
                //     /b\c/d\
                // v2 o---o---o v1
                //       v12
                const uint16_t indicesToAdd[] =
                {
                     iv0, iv01, iv20, // a
                    iv20, iv12,  iv2, // b
                    iv20, iv01, iv12, // c
                    iv01,  iv1, iv12, // d
                };
                newIndices.insert(newIndices.end(), std::begin(indicesToAdd), std::end(indicesToAdd));
            }

            indices = std::move(newIndices);
        }

        // Now that we've completed subdivision, fill in the final vertex collection
        vertices.reserve(vertexPositions.size());
        for (const auto& it : vertexPositions)
        {
            auto const normal = XMVector3Normalize(XMLoadFloat3(&it));
            auto const pos = XMVectorScale(normal, radius);

            XMFLOAT3 normalFloat3;
            XMStoreFloat3(&normalFloat3, normal);

            // calculate texture coordinates for this vertex
            const float longitude = atan2f(normalFloat3.x, -normalFloat3.z);
            const float latitude = acosf(normalFloat3.y);

            const float u = longitude / XM_2PI + 0.5f;
            const float v = latitude / XM_PI;

            auto const texcoord = XMVectorSet(1.0f - u, v, 0.0f, 0.0f);
            vertices.push_back(VertexPositionNormalTexture(pos, normal, texcoord));
        }

        // There are a couple of fixes to do. One is a texture coordinate wraparound fixup. At some point, there will be
        // a set of triangles somewhere in the mesh with texture coordinates such that the wraparound across 0.0/1.0
        // occurs across that triangle. Eg. when the left hand side of the triangle has a U coordinate of 0.98 and the
        // right hand side has a U coordinate of 0.0. The intent is that such a triangle should render with a U of 0.98 to
        // 1.0, not 0.98 to 0.0. If we don't do this fixup, there will be a visible seam across one side of the sphere.
        //
        // Luckily this is relatively easy to fix. There is a straight edge which runs down the prime meridian of the
        // completed sphere. If you imagine the vertices along that edge, they circumscribe a semicircular arc starting at
        // y=1 and ending at y=-1, and sweeping across the range of z=0 to z=1. x stays zero. It's along this edge that we
        // need to duplicate our vertices - and provide the correct texture coordinates.
        const size_t preFixupVertexCount = vertices.size();
        for (size_t i = 0; i < preFixupVertexCount; ++i)
        {
            // This vertex is on the prime meridian if position.x and texcoord.u are both zero (allowing for small epsilon).
            const bool isOnPrimeMeridian = XMVector2NearEqual(
                XMVectorSet(vertices[i].position.x, vertices[i].textureCoordinate.x, 0.0f, 0.0f),
                XMVectorZero(),
                XMVectorSplatEpsilon());

            if (isOnPrimeMeridian)
            {
                size_t newIndex = vertices.size(); // the index of this vertex that we're about to add
                CheckIndexOverflow(newIndex);

                // copy this vertex, correct the texture coordinate, and add the vertex
                VertexPositionNormalTexture v = vertices[i];
                v.textureCoordinate.x = 1.0f;
                vertices.push_back(v);

                // Now find all the triangles which contain this vertex and update them if necessary
                for (size_t j = 0; j < indices.size(); j += 3)
                {
                    uint16_t* triIndex0 = &indices[j + 0];
                    uint16_t* triIndex1 = &indices[j + 1];
                    uint16_t* triIndex2 = &indices[j + 2];

                    if (*triIndex0 == i)
                    {
                        // nothing; just keep going
                    }
                    else if (*triIndex1 == i)
                    {
                        std::swap(triIndex0, triIndex1); // swap the pointers (not the values)
                    }
                    else if (*triIndex2 == i)
                    {
                        std::swap(triIndex0, triIndex2); // swap the pointers (not the values)
                    }
                    else
                    {
                        // this triangle doesn't use the vertex we're interested in
                        continue;
                    }

                    // If we got to this point then triIndex0 is the pointer to the index to the vertex we're looking at
                    assert(*triIndex0 == i);
                    assert(*triIndex1 != i && *triIndex2 != i); // assume no degenerate triangles

                    const VertexPositionNormalTexture& v0 = vertices[*triIndex0];
                    const VertexPositionNormalTexture& v1 = vertices[*triIndex1];
                    const VertexPositionNormalTexture& v2 = vertices[*triIndex2];

                    // check the other two vertices to see if we might need to fix this triangle

                    if (abs(v0.textureCoordinate.x - v1.textureCoordinate.x) > 0.5f ||
                        abs(v0.textureCoordinate.x - v2.textureCoordinate.x) > 0.5f)
                    {
                        // yep; replace the specified index to point to the new, corrected vertex
                        *triIndex0 = static_cast<uint16_t>(newIndex);
                    }
                }
            }
        }

        // And one last fix we need to do: the poles. A common use-case of a sphere mesh is to map a rectangular texture onto
        // it. If that happens, then the poles become singularities which map the entire top and bottom rows of the texture
        // onto a single point. In general there's no real way to do that right. But to match the behavior of non-geodesic
        // spheres, we need to duplicate the pole vertex for every triangle that uses it. This will introduce seams near the
        // poles, but reduce stretching.
        auto const fixPole = [&](size_t poleIndex)
            {
                const auto& poleVertex = vertices[poleIndex];
                bool overwrittenPoleVertex = false; // overwriting the original pole vertex saves us one vertex

                for (size_t i = 0; i < indices.size(); i += 3)
                {
                    // These pointers point to the three indices which make up this triangle. pPoleIndex is the pointer to the
                    // entry in the index array which represents the pole index, and the other two pointers point to the other
                    // two indices making up this triangle.
                    uint16_t* pPoleIndex;
                    uint16_t* pOtherIndex0;
                    uint16_t* pOtherIndex1;
                    if (indices[i + 0] == poleIndex)
                    {
                        pPoleIndex = &indices[i + 0];
                        pOtherIndex0 = &indices[i + 1];
                        pOtherIndex1 = &indices[i + 2];
                    }
                    else if (indices[i + 1] == poleIndex)
                    {
                        pPoleIndex = &indices[i + 1];
                        pOtherIndex0 = &indices[i + 2];
                        pOtherIndex1 = &indices[i + 0];
                    }
                    else if (indices[i + 2] == poleIndex)
                    {
                        pPoleIndex = &indices[i + 2];
                        pOtherIndex0 = &indices[i + 0];
                        pOtherIndex1 = &indices[i + 1];
                    }
                    else
                    {
                        continue;
                    }

                    const auto& otherVertex0 = vertices[*pOtherIndex0];
                    const auto& otherVertex1 = vertices[*pOtherIndex1];

                    // Calculate the texcoords for the new pole vertex, add it to the vertices and update the index
                    VertexPositionNormalTexture newPoleVertex = poleVertex;
                    newPoleVertex.textureCoordinate.x = (otherVertex0.textureCoordinate.x + otherVertex1.textureCoordinate.x) / 2;
                    newPoleVertex.textureCoordinate.y = poleVertex.textureCoordinate.y;

                    if (!overwrittenPoleVertex)
                    {
                        vertices[poleIndex] = newPoleVertex;
                        overwrittenPoleVertex = true;
                    }
                    else
                    {
                        CheckIndexOverflow(vertices.size());

                        *pPoleIndex = static_cast<uint16_t>(vertices.size());
                        vertices.push_back(newPoleVertex);
                    }
                }
            };

        fixPole(northPoleIndex);
        fixPole(southPoleIndex);

        // Build RH above
        if (!rhcoords)
            ReverseWinding(indices, vertices);
    }

    //  End copied geometry building code -----------------------------------------------------------------------

    void ComputeGrid(GeometricPrimitive::VertexCollection& vertices,
        GeometricPrimitive::IndexCollection& indices,
        const float& width,
        const float& height,
        const float& divisions,
        bool tiled)
    {
        using namespace DirectX::SimpleMath;

        auto xStep = width / divisions;
        auto zStep = height / divisions;
        auto topLeftX = -width / 2.f;
        auto topLeftZ = -height / 2.f;

        for (int i = 0; i < divisions + 1; i++)
        {
            GeometricPrimitive::VertexType vertex;
            vertex.position = Vector3{ topLeftX + i * xStep, 0, topLeftZ };
            vertex.normal = Vector3::UnitY;

            auto texcoord = Vector2{ (float)i, 0.f };
            if (tiled)
                vertex.textureCoordinate = texcoord;
            else
                vertex.textureCoordinate = texcoord * 1.f / divisions;

            vertices.push_back(vertex);
        }

        bool invertAlongX = false;
        bool invertAlongZ = true;
        for (int zIndex = 1; zIndex < divisions + 1; zIndex++)
        {
            for (int i = 0; i < divisions + 1; i++)
            {
                GeometricPrimitive::VertexType vertex;
                vertex.position = Vector3{
                    topLeftX + i * xStep,
                    0,
                    topLeftZ + zIndex * zStep
                };
                vertex.normal = Vector3::UnitY;
                auto texcoord = Vector2{ (float)i, (float)zIndex };

                if (tiled)
                    vertex.textureCoordinate = texcoord;
                else
                    vertex.textureCoordinate = texcoord * 1.f / divisions;


                vertices.push_back(vertex);
            }

            invertAlongX = false;
            for (int i = 0; i < divisions; i++)
            {
                auto baseIndex = zIndex * (divisions + 1) + i;

                if (invertAlongZ)
                {
                    if (invertAlongX)
                    {
                        indices.push_back(baseIndex);
                        indices.push_back(baseIndex - divisions - 1);
                        indices.push_back(baseIndex - divisions);
                        indices.push_back(baseIndex);
                        indices.push_back(baseIndex - divisions);
                        indices.push_back(baseIndex + 1);
                    }
                    else
                    {
                        indices.push_back(baseIndex);
                        indices.push_back(baseIndex - divisions - 1);
                        indices.push_back(baseIndex + 1);
                        indices.push_back(baseIndex + 1);
                        indices.push_back(baseIndex - divisions - 1);
                        indices.push_back(baseIndex - divisions);
                    }
                }
                else
                {
                    if (invertAlongX)
                    {
                        indices.push_back(baseIndex);
                        indices.push_back(baseIndex - divisions - 1);
                        indices.push_back(baseIndex + 1);
                        indices.push_back(baseIndex + 1);
                        indices.push_back(baseIndex - divisions - 1);
                        indices.push_back(baseIndex - divisions);
                    }
                    else
                    {
                        indices.push_back(baseIndex);
                        indices.push_back(baseIndex - divisions - 1);
                        indices.push_back(baseIndex - divisions);
                        indices.push_back(baseIndex);
                        indices.push_back(baseIndex - divisions);
                        indices.push_back(baseIndex + 1);
                    }
                }
                invertAlongX = !invertAlongX;
            }
            invertAlongZ = !invertAlongZ;
        }
    }

    void ComputeFrustum(
        GeometricPrimitive::VertexCollection& vertices,
        GeometricPrimitive::IndexCollection& indices,
        float topRadius,
        float bottomRadius,
        int numStacks,
        int numVerticalSections,
        float height)
    {
        using namespace DirectX;

        int numSlices = numStacks + 1;
        int verticesPerSlice = numVerticalSections + 1;
        int numVertices = verticesPerSlice * numSlices;
        int numIndices = 6 * numVerticalSections * numStacks;

        float dh = height / numStacks;

        //Vertex generation
        for (int i = 0; i < numSlices; i++)
        {
            float y = i * dh;
            //Radius of slice
            float r = bottomRadius - (y * (bottomRadius - topRadius)) / height;

            float dTheta = XM_2PI / numVerticalSections;

            //The first and last vertices in each ring are duplicated,
            //because they have different texture coordinates.
            for (int j = 0; j <= numVerticalSections; j++)
            {
                float x = r * cosf(j * dTheta);
                float z = r * sinf(j * dTheta);

                GeometricPrimitive::VertexType vertex;
                vertex.position = XMFLOAT3(x, y, z);
                vertex.textureCoordinate.x = static_cast<float>(j) / numVerticalSections;
                vertex.textureCoordinate.y = 1.0f - static_cast<float>(i) / numStacks;

                XMFLOAT4 temp = XMFLOAT4(x, 0, z, 1.0f);
                XMVECTOR vertexXZ = XMLoadFloat4(&temp);
                vertexXZ = XMVector3Normalize(vertexXZ);
                XMVECTOR radiusTopXZ = topRadius * vertexXZ;
                XMVECTOR radiusBottomXZ = bottomRadius * vertexXZ;
                temp = XMFLOAT4(0, height, 0, 1.0f);
                XMVECTOR axis = XMLoadFloat4(&temp);
                XMVECTOR hue = radiusTopXZ + axis;
                XMVECTOR tangent1 = hue - radiusBottomXZ;
                XMVECTOR tangent2 = XMVector3Cross(radiusBottomXZ, axis);
                XMVECTOR normal = XMVector3Cross(tangent1, tangent2);
                normal = XMVector3Normalize(normal);
                XMStoreFloat3(&vertex.normal, normal);

                vertices.push_back(vertex);
            }
        }

        //Index generation
        for (int i = 0; i < (numSlices - 1) * (numVerticalSections + 1); i++)
        {

            indices.push_back(i);
            indices.push_back(i + (numVerticalSections + 1));
            indices.push_back(i + 1);
            indices.push_back(i + 1);
            indices.push_back(i + (numVerticalSections + 1));
            indices.push_back((i + 1) + numVerticalSections + 1);
        }

        //Vertex and index generation for the caps (top and bottom)

        int lastVertex = vertices.size() - 1;
        //Bottom cap

        //Center vertex.
        GeometricPrimitive::VertexType center;
        center.position = XMFLOAT3(0, 0, 0);
        center.normal = XMFLOAT3(0, -1, 0);
        center.textureCoordinate = XMFLOAT2(0.5, 0.5);

        vertices.push_back(center);

        int centerIndex = vertices.size() - 1;

        //Copy the bottom ring of vertices.
        GeometricPrimitive::VertexCollection bottomVertices;
        for (int i = 0; i < verticesPerSlice; i++)
        {
            bottomVertices.push_back(vertices[i]);
        }

        for (auto& x : bottomVertices)
        {
            XMFLOAT3 normal = XMFLOAT3(0, -1, 0);
            x.normal = normal;
            float u = x.position.x / height + 0.5;
            float v = x.position.z / height + 0.5;
            x.textureCoordinate = XMFLOAT2(u, v);
        }

        vertices.insert(vertices.end(), bottomVertices.begin(), bottomVertices.end());

        for (int i = centerIndex + 1; i < centerIndex + verticesPerSlice; i++)
        {
            indices.push_back(i + 1);
            indices.push_back(centerIndex);
            indices.push_back(i);
        }

        //Top cap

        //Center vertex
        center.position = XMFLOAT3(0, height, 0);
        center.normal = XMFLOAT3(0, 1, 0);
        center.textureCoordinate = XMFLOAT2(0.5, 0.5);

        vertices.push_back(center);
        centerIndex = vertices.size() - 1;

        //Copy the top ring of vertices.
        GeometricPrimitive::VertexCollection topVertices;
        int base = lastVertex - verticesPerSlice + 1;

        for (int i = base; i < base + verticesPerSlice; i++)
        {
            topVertices.push_back(vertices[i]);
        }

        for (auto& x : topVertices)
        {
            XMFLOAT3 normal = XMFLOAT3(0, 1, 0);
            x.normal = normal;
            float u = x.position.x / height + 0.5;
            float v = x.position.z / height + 0.5;
            x.textureCoordinate = XMFLOAT2(u, v);
        }

        vertices.insert(vertices.end(), topVertices.begin(), topVertices.end());

        for (int i = centerIndex + 1; i < centerIndex + verticesPerSlice; i++)
        {
            indices.push_back(i);
            indices.push_back(centerIndex);
            indices.push_back(i + 1);
        }

        ReverseWinding(indices, vertices);
    }

    void ComputeAngledFrustum(
        GeometricPrimitive::VertexCollection& vertices,
        GeometricPrimitive::IndexCollection& indices,
        float bottomRadius,
        float topRadius,
        Vector3 topCentre,
        Vector3 topNormal,
        int numVerticalSections)
    {
        int numSlices = 2;
        int verticesPerSlice = numVerticalSections + 1;
        int numVertices = verticesPerSlice * numSlices;
        int numIndices = 6 * numVerticalSections;

        float dTheta = DirectX::XM_2PI / numVerticalSections;

        // Bottom ring
        // The first and last vertex are duplicated because 
        // they have different texcoords.
        {
            Vector3 r = Vector3::UnitX;
            r *= bottomRadius;

            for (int j = 0; j <= numVerticalSections; j++)
            {
                float theta = dTheta * j;
                auto rotationMatrix = Matrix::CreateFromAxisAngle(Vector3::UnitY, theta);
                auto centreOffset = Vector3::Transform(r, rotationMatrix);

                GeometricPrimitive::VertexType vertex;
                vertex.position = centreOffset;
                auto normal = centreOffset;
                normal.Normalize();
                vertex.normal = normal;
                vertex.textureCoordinate.x = static_cast<float>(j) / numVerticalSections;
                vertex.textureCoordinate.y = 1.f;

                vertices.push_back(vertex);
            }
        }

        // Top ring
        {
            Vector3 r = topNormal.Cross(Vector3::UnitZ);
            r.Normalize();
            r *= topRadius;

            for (int j = 0; j <= numVerticalSections; j++)
            {
                float theta = dTheta * j;
                auto rotationMatrix = Matrix::CreateFromAxisAngle(topNormal, theta);
                auto centreOffset = Vector3::Transform(r, rotationMatrix);

                GeometricPrimitive::VertexType vertex;
                vertex.position = topCentre + centreOffset;
                auto normal = centreOffset;
                normal.Normalize();
                vertex.normal = normal;
                vertex.textureCoordinate.x = static_cast<float>(j) / numVerticalSections;
                vertex.textureCoordinate.y = 0.f;

                vertices.push_back(vertex);
            }
        }

        //Index generation
        for (int i = 0; i < numVerticalSections; i++)
        {
            indices.push_back(i);
            indices.push_back(i + numVerticalSections + 1);
            indices.push_back(i + 1);

            indices.push_back(i + 1);
            indices.push_back(i + numVerticalSections + 1);
            indices.push_back(i + 1 + numVerticalSections + 1);
        }

        int lastVertex = vertices.size() - 1;

        {
            //Bottom cap

            //Center vertex.
            GeometricPrimitive::VertexType center;
            center.position = { 0, 0, 0 };
            center.normal = { 0, -1, 0 };
            center.textureCoordinate = { 0.5, 0.5 };

            vertices.push_back(center);

            int centerIndex = vertices.size() - 1;

            //Copy the bottom ring of vertices.
            GeometricPrimitive::VertexCollection bottomVertices;
            for (int i = 0; i < verticesPerSlice; i++)
            {
                bottomVertices.push_back(vertices[i]);
            }

            for (auto& x : bottomVertices)
            {
                x.normal = { 0, -1, 0 };

                // x and z belong to [-bottomRadius, bottomRadius].
                // Map this to [0, 1]
                float u = ((x.position.x / bottomRadius) + 1) / 2.f;
                float v = ((x.position.z / bottomRadius) + 1) / 2.f;
                x.textureCoordinate = { u, v };
            }

            vertices.insert(vertices.end(), bottomVertices.begin(), bottomVertices.end());

            for (int i = centerIndex + 1; i < centerIndex + verticesPerSlice; i++)
            {
                indices.push_back(i + 1);
                indices.push_back(centerIndex);
                indices.push_back(i);
            }
        }

        {
            // Top cap

            //Center vertex
            GeometricPrimitive::VertexType center;
            center.position = topCentre;
            center.normal = topNormal;
            center.textureCoordinate = { 0.5, 0.5 };

            vertices.push_back(center);
            auto centerIndex = vertices.size() - 1;

            //Copy the top ring of vertices.
            GeometricPrimitive::VertexCollection topVertices;
            int base = lastVertex - verticesPerSlice + 1;

            for (int i = base; i < base + verticesPerSlice; i++)
            {
                topVertices.push_back(vertices[i]);
            }

            // To construct UVs, we map into a space 
            // with the top normal as the Y axis.
            Vector3 basisY = topNormal;
            basisY.Normalize();
            Vector3 basisX = basisY.Cross(Vector3::UnitZ);
            basisX.Normalize();
            Vector3 basisZ = basisX.Cross(basisY);
            basisZ.Normalize();

            auto fromNewBasis = Matrix(basisX, basisY, basisZ);
            fromNewBasis.Translation(topCentre);
            fromNewBasis.Transpose();
            Matrix toNewBasis;
            fromNewBasis.Invert(toNewBasis);

            for (auto& x : topVertices)
            {
                x.normal = topNormal;

                Vector3 transformedPosition =
                    Vector3::Transform(x.position,
                        toNewBasis);

                float u = ((transformedPosition.x / topRadius) + 1) / 2.f;
                float v = ((transformedPosition.z / topRadius) + 1) / 2.f;
                x.textureCoordinate = { u, v };
            }

            vertices.insert(vertices.end(), topVertices.begin(), topVertices.end());

            for (int i = centerIndex + 1; i < centerIndex + verticesPerSlice; i++)
            {
                indices.push_back(i);
                indices.push_back(centerIndex);
                indices.push_back(i + 1);
            }
        }
    }

    void GeometricPrimitive::Draw(ID3D12GraphicsCommandList* cl)
    {
        cl->IASetVertexBuffers(0,
            1,
            &m_vbv);
        cl->IASetIndexBuffer(&m_ibv);

        cl->DrawIndexedInstanced(m_indexCount,
            1,
            0,
            0,
            0);
    }

    void GeometricPrimitive::Initialize(ID3D12Device* device,
        ID3D12CommandQueue* cq,
        VertexCollection vertices,
        IndexCollection indices)
    {
        DirectX::ResourceUploadBatch uploadBatch(device);

        uploadBatch.Begin();

        DX::ThrowIfFailed(
            DirectX::CreateStaticBuffer(device, uploadBatch,
                vertices,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                m_vertexBuffer.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(
            DirectX::CreateStaticBuffer(device,
                uploadBatch,
                indices,
                D3D12_RESOURCE_STATE_INDEX_BUFFER,
                m_indexBuffer.ReleaseAndGetAddressOf()));

        auto uploadFinished = uploadBatch.End(cq);

        uploadFinished.wait();

        m_vbv.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vbv.StrideInBytes = sizeof(VertexType);
        m_vbv.SizeInBytes = m_vbv.StrideInBytes * vertices.size();

        m_ibv.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_ibv.Format = DXGI_FORMAT_R16_UINT;
        m_ibv.SizeInBytes = sizeof(uint16_t) * indices.size();

        m_vertexCount = vertices.size();
        m_indexCount = indices.size();
    }

    std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateBox(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const DirectX::XMFLOAT3& size,
        bool rhcoords,
        bool invertn)
    {
        VertexCollection vertices;
        IndexCollection indices;
        ComputeBox(vertices, indices, size, rhcoords, invertn);

        std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());
        primitive->Initialize(device, cq, vertices, indices);

        return primitive;
    }

    std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateSphere(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        float diameter,
        size_t tessellation,
        bool rhcoords,
        bool invertn)
    {
        VertexCollection vertices;
        IndexCollection indices;
        ComputeSphere(vertices, indices, diameter, tessellation, rhcoords, invertn);

        std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

        primitive->Initialize(device, cq, vertices, indices);

        return primitive;
    }

    std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateGeoSphere(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        float diameter,
        size_t tessellation,
        bool rhcoords)
    {
        VertexCollection vertices;
        IndexCollection indices;
        ComputeGeoSphere(vertices, indices, diameter, tessellation, rhcoords);

        std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

        primitive->Initialize(device, cq, vertices, indices);

        return primitive;
    }

    std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateGrid(ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const float& width,
        const float& height,
        const float& divisions,
        bool tiled)
    {
        VertexCollection vertices;
        IndexCollection indices;
        ComputeGrid(vertices, indices, width, height, divisions, tiled);

        std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());
        primitive->Initialize(device, cq, vertices, indices);

        return primitive;
    }

    std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateFrustum(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const float& topRadius,
        const float& bottomRadius,
        const float& height)
    {
        VertexCollection vertices;
        IndexCollection indices;

        ComputeFrustum(vertices, indices,
            topRadius, bottomRadius,
            static_cast<int>(height) * 2,
            18,
            height);

        std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());
        primitive->Initialize(device, cq, vertices, indices);

        return primitive;
    }

    std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateAngledFrustum(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const float& bottomRadius,
        const float& topRadius,
        const DirectX::SimpleMath::Vector3& topCentre,
        const DirectX::SimpleMath::Vector3& topNormal)
    {
        VertexCollection vertices;
        IndexCollection indices;

        ComputeAngledFrustum(vertices, indices,
            bottomRadius, topRadius,
            topCentre,
            topNormal,
            18);

        std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());
        primitive->Initialize(device, cq, vertices, indices);

        return primitive;
    }
}