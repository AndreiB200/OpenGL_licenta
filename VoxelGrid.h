#define _VOXELGRID_
#ifdef _VOXELGRID_
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <queue>
#include <cmath>
#include <unordered_set>

struct VoxelKey {
    int x, y, z;

    bool operator==(const VoxelKey& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator<(const VoxelKey& other) const {
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        return z < other.z;
    }
};


struct VoxelKeyHash {
    std::size_t operator()(const VoxelKey& k) const {
        // Hash simplu și rapid pe 64-bit pentru coordonate 3D
        std::size_t h1 = std::hash<int>{}(k.x);
        std::size_t h2 = std::hash<int>{}(k.y);
        std::size_t h3 = std::hash<int>{}(k.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class LidarVoxelGrid {
private:
    float cellSize;
    int max_capacity;

    std::vector<VoxelKey> occupiedVoxels;
    std::vector<glm::vec3> cachedCenters;

    std::unordered_set<VoxelKey, VoxelKeyHash> voxelLookup;

    std::queue<std::vector<VoxelKey>> voxelsQueue;

    glm::vec3 keyToCenter(const VoxelKey& key) const {
        return glm::vec3(
            (key.x + 0.5f) * cellSize,
            (key.y + 0.5f) * cellSize,
            (key.z + 0.5f) * cellSize
        );
    }

public:
    LidarVoxelGrid(float size = 2.0f, int _max_capacity = 512) : cellSize(size), max_capacity(_max_capacity) {
        occupiedVoxels.reserve(max_capacity);
        cachedCenters.assign(max_capacity, glm::vec3(0.0f));
        voxelLookup.reserve(max_capacity);
    }

    VoxelKey pointToKey(const glm::vec3& point) const {
        return VoxelKey{
            static_cast<int>(std::floor(point.x / cellSize)),
            static_cast<int>(std::floor(point.y / cellSize)),
            static_cast<int>(std::floor(point.z / cellSize))
        };
    }

    float getCellSize() const { return cellSize; }

    void changePointCache(const glm::vec3& point)
    {
        static int i = 0;
        cachedCenters[i % max_capacity] = point;
        i++;
        if (i >= max_capacity) i = 0;
    }

    void addPoint(const glm::vec3& point) {
        VoxelKey key = pointToKey(point);

        if (voxelLookup.insert(key).second) {
            occupiedVoxels.push_back(key);
            changePointCache(keyToCenter(key));
        }
    }

    void addPoints(const std::vector<glm::vec3>& points, glm::vec3 cameraPos) {
        for (const auto& pt : points) {
            if (glm::length(pt - cameraPos) < 15.0f) {
                addPoint(pt);
            }
        }

        if (occupiedVoxels.size() > max_capacity) {
            voxelsQueue.push(occupiedVoxels);

            occupiedVoxels.clear();
            voxelLookup.clear();
        }

        if (voxelsQueue.size() > 5) {
            voxelsQueue.pop();
        }
    }

    const std::vector<glm::vec3>& getUniqueCenters() const {
        return cachedCenters;
    }

    void clear() {
        occupiedVoxels.clear();
        voxelLookup.clear();
    }
};

struct DistanceGradientGPU {
    float distance;
    float padding[3];
    glm::vec4 gradient;
};

class LidarVoxelGridGPU {
private:
    float cellSize;
    glm::ivec3 gridDimensions;
    glm::vec3 gridOrigin;

    GLuint ssboPoints;
    GLuint ssboVoxelGrid;
    GLuint ssboQueries;
    GLuint ssboResults;

    size_t maxPoints;
    size_t maxQueries;

    Shader voxelizeShaderProgram = Shader("voxelize.comp");
    Shader distGradShaderProgram = Shader("distance_gradient.comp");

public:
    LidarVoxelGridGPU(float size = 0.5f, glm::ivec3 dims = glm::ivec3(128, 128, 128), glm::vec3 origin = glm::vec3(-32.0f))
        : cellSize(size), gridDimensions(dims), gridOrigin(origin), maxPoints(100000), maxQueries(1000)
    {
        initBuffers();
    }

    ~LidarVoxelGridGPU() {
        glDeleteBuffers(1, &ssboPoints);
        glDeleteBuffers(1, &ssboVoxelGrid);
        glDeleteBuffers(1, &ssboQueries);
        glDeleteBuffers(1, &ssboResults);
    }

    void initBuffers() {
        // 1. Buffer Puncte LiDAR
        glGenBuffers(1, &ssboPoints);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboPoints);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxPoints * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

        // 2. Buffer Voxel Grid (Ocupanță 3D Liniarizată)
        size_t totalVoxels = gridDimensions.x * gridDimensions.y * gridDimensions.z;
        glGenBuffers(1, &ssboVoxelGrid);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVoxelGrid);
        glBufferData(GL_SHADER_STORAGE_BUFFER, totalVoxels * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        // 3. Buffer Interogări (Query Positions)
        glGenBuffers(1, &ssboQueries);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboQueries);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxQueries * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

        // 4. Buffer Rezultate (Distance & Gradient)
        glGenBuffers(1, &ssboResults);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboResults);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxQueries * sizeof(DistanceGradientGPU), nullptr, GL_DYNAMIC_READ);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void clearGridOnGPU() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVoxelGrid);
        size_t totalVoxels = gridDimensions.x * gridDimensions.y * gridDimensions.z;
        GLuint zero = 0;
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
    }

    void bindBuffersAndUploadData(const std::vector<glm::vec3>& inputPoints, const std::vector<glm::vec3>& queryPositions) {
        if (inputPoints.size() > maxPoints) {
            maxPoints = inputPoints.size();
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboPoints);
            glBufferData(GL_SHADER_STORAGE_BUFFER, maxPoints * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
        }

        std::vector<glm::vec4> pointsv4(inputPoints.size());
        for (size_t i = 0; i < inputPoints.size(); ++i) {
            pointsv4[i] = glm::vec4(inputPoints[i], 1.0f);
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboPoints);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, pointsv4.size() * sizeof(glm::vec4), pointsv4.data());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboPoints); // binding = 0

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboVoxelGrid); // binding = 1

        if (!queryPositions.empty()) {
            std::vector<glm::vec4> queriesv4(queryPositions.size());
            for (size_t i = 0; i < queryPositions.size(); ++i) {
                queriesv4[i] = glm::vec4(queryPositions[i], 1.0f);
            }

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboQueries);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, queriesv4.size() * sizeof(glm::vec4), queriesv4.data());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboQueries); // binding = 2
        }

        // Binding Rezultate
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboResults); // binding = 3
    }

    std::vector<DistanceGradientGPU> downloadResults(size_t queryCount) {
        std::vector<DistanceGradientGPU> results(queryCount);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboResults);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, queryCount * sizeof(DistanceGradientGPU), results.data());
        return results;
    }

    int debugGetOccupiedVoxelCount() {
        size_t totalVoxels = gridDimensions.x * gridDimensions.y * gridDimensions.z;
        std::vector<uint32_t> voxels(totalVoxels);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVoxelGrid);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, totalVoxels * sizeof(uint32_t), voxels.data());

        int count = 0;
        for (uint32_t v : voxels) {
            if (v > 0) count++;
        }
        return count;
    }

    void bindVoxels(const std::vector<glm::vec3>& inputPoints, const std::vector<glm::vec3>& queryPositions, glm::vec3 &cameraPos, std::vector<DistanceGradientGPU> &results)
    {
        clearGridOnGPU();
        bindBuffersAndUploadData(inputPoints, queryPositions);

        voxelizeShaderProgram.use();

        voxelizeShaderProgram.setInt("u_PointCount", inputPoints.size());
        voxelizeShaderProgram.setFloat("u_CellSize", getCellSize());
        voxelizeShaderProgram.setVec3i("u_GridDims", getGridDimensions());
        voxelizeShaderProgram.setVec3("u_GridOrigin", getGridOrigin());
        voxelizeShaderProgram.setVec3("u_CameraPos", cameraPos);

        GLuint numPointGroups = (static_cast<GLuint>(inputPoints.size()) + 255) / 256;
        glDispatchCompute(numPointGroups, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

       
        distGradShaderProgram.use();
        distGradShaderProgram.setInt("u_QueryCount", queryPositions.size());
        distGradShaderProgram.setFloat("u_CellSize", getCellSize());
        distGradShaderProgram.setVec3i("u_GridDims", getGridDimensions());
        distGradShaderProgram.setVec3("u_GridOrigin", getGridOrigin());
        distGradShaderProgram.setFloat("u_SearchRadiusWorld", 5.0f);

        GLuint numQueryGroups = (static_cast<GLuint>(queryPositions.size()) + 63) / 64;
        glDispatchCompute(numQueryGroups, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        results = downloadResults(queryPositions.size());

        float distance = results[0].distance;
        glm::vec3 gradient = glm::vec3(results[0].gradient);
        //debugGetOccupiedVoxelCount();
    }

    // Getteri pentru Uniforms
    float getCellSize() const { return cellSize; }
    glm::ivec3 getGridDimensions() const { return gridDimensions; }
    glm::vec3 getGridOrigin() const { return gridOrigin; }
};

#endif //_VOXELGRID_