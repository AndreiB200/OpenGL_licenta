#define _VOXELGRID_
#ifdef _VOXELGRID_

#include <glm/glm.hpp>
#include <unordered_set>
#include <vector>
#include <cmath>

struct VoxelKey {
    int x, y, z;
    bool operator==(const VoxelKey& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct VoxelKeyHash {
    std::size_t operator()(const VoxelKey& k) const {
        std::size_t h1 = std::hash<int>{}(k.x);
        std::size_t h2 = std::hash<int>{}(k.y);
        std::size_t h3 = std::hash<int>{}(k.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class LidarVoxelGrid {
private:
    float cellSize;
    std::unordered_set<VoxelKey, VoxelKeyHash> occupiedVoxels;

public:
    LidarVoxelGrid(float size = 0.1f) : cellSize(size) { occupiedVoxels.reserve(1000); }

    VoxelKey pointToKey(const glm::vec3& point) const 
    {
        return VoxelKey{
            static_cast<int>(std::floor(point.x / cellSize)),
            static_cast<int>(std::floor(point.y / cellSize)),
            static_cast<int>(std::floor(point.z / cellSize))
        };
    }

    void addPoint(const glm::vec3& point) 
    {
        VoxelKey key = pointToKey(point);
        occupiedVoxels.insert(key);
    }

    void addPoints(const std::vector<glm::vec3>& points, glm::vec3 cameraPos) 
    {
        for (const auto& pt : points) {
            if(glm::length(pt - cameraPos) < 10.0f)
                addPoint(pt);
        }
    }

    std::vector<glm::vec3> getUniqueCenters() const {
        std::vector<glm::vec3> centers;
        centers.reserve(occupiedVoxels.size());

        for (const auto& key : occupiedVoxels) {
            glm::vec3 center(
                (key.x + 0.5f) * cellSize,
                (key.y + 0.5f) * cellSize,
                (key.z + 0.5f) * cellSize
            );
            centers.push_back(center);
        }
        return centers;
    }

    void clear() 
    { 
        occupiedVoxels.clear(); 
    }
};

#endif //_VOXELGRID_