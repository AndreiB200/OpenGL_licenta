#define _VFH_
#ifdef _VFH_

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


struct BSplineCubic {
    glm::vec3 P0;
    glm::vec3 P1;
    glm::vec3 P2;
    glm::vec3 P3;

    glm::vec3 evaluate(float t) const {
        float u = 1.0f - t;
        float b0 = u * u * u;
        float b1 = 3.0f * u * u * t;
        float b2 = 3.0f * u * t * t;
        float b3 = t * t * t;

        return b0 * P0 + b1 * P1 + b2 * P2 + b3 * P3;
    }
    glm::vec3 velocity(float t) const {
        float u = 1.0f - t;
        return 3.0f * u * u * (P1 - P0) + 6.0f * u * t * (P2 - P1) + 3.0f * t * t * (P3 - P2);
    }
};


struct BSplineQuadratic {
    glm::vec3 P0;
    glm::vec3 P1;
    glm::vec3 P2;

    glm::vec3 evaluate(float t) const {
        float u = 1.0f - t;
        float b0 = u * u;
        float b1 = 2.0f * u * t;
        float b2 = t * t;

        return b0 * P0 + b1 * P1 + b2 * P2;
    }
};

class VFHPlus3D {
public:
    struct Config {
        float sensorMaxRadius = 100.0f;
        float safetyRadius = 2.0f;
        float sectorSizeDeg = 3.0f;        
        float horizontalFovDeg = 90.0f;    
        float verticalFovDeg = 90.0f;      
        float densityThreshold = 0.2f;
    };

    VFHPlus3D() : VFHPlus3D(Config()) {}

    void bindPub(ZmqNode& _publisher) {
        publisher = &_publisher;
    }

    VFHPlus3D(Config config)
        : m_cfg(config),
        m_numAzimuth(static_cast<int>(config.horizontalFovDeg / config.sectorSizeDeg)),
        m_numElevation(static_cast<int>(config.verticalFovDeg / config.sectorSizeDeg))
    {
        m_histogram.resize(m_numAzimuth * m_numElevation, 0.0f);
    }
   

    glm::vec3 computeHybridAPF_EGOPlanner(
        const glm::vec3& agentPos,
        const glm::vec3& targetPos,
        float& targetYaw,
        LidarVoxelGrid &lidarVoxelGrid,
        std::vector<glm::vec3>& splineSegment,
        float actualSpeed = 4.0f,
        float k_att = 5.0f,
        float learningRate = 5.0f,
        float d_max = 5.0f,
        float k_rep = 20.0f,
        float smoothness = 0.05f
    ) {
        const float a_max = actualSpeed;
        const float stopThreshold = 4.0f;

        glm::vec3 dirToGlobalTarget = targetPos - agentPos;
        float distToGlobalTarget = glm::length(dirToGlobalTarget);

        if (distToGlobalTarget < stopThreshold) {
            return glm::vec3(0.0f);
        }

        std::vector<glm::vec3> foundedPoints = lidarVoxelGrid.getUniqueCenters();

        dirToGlobalTarget = glm::normalize(dirToGlobalTarget);

        float d_thresh = d_max + (actualSpeed * actualSpeed) / (2.0f * a_max);

        float localHorizon = glm::clamp(d_thresh + 2.0f, 2.0f, 10.0f);

        float minHitDist = localHorizon;
        const float voxelRadius = 0.0f;

        for (const auto& obsPt : foundedPoints)
        {
            glm::vec3 toObs = obsPt - agentPos;

            float projDist = glm::dot(toObs, dirToGlobalTarget);

            if (projDist > 0.0f && projDist < localHorizon)
            {
                glm::vec3 closestPointOnRay = agentPos + dirToGlobalTarget * projDist;
                float perpendicularDist = glm::length(obsPt - closestPointOnRay);

                if (perpendicularDist < voxelRadius)
                {
                    float hitDist = projDist - voxelRadius;

                    if (hitDist < minHitDist) {
                        minHitDist = hitDist;
                    }
                }
            }
        }

        if (minHitDist < localHorizon) {
            const float d_margin = 0.8f;
            localHorizon = std::max(0.5f, minHitDist - d_margin);
        }

        glm::vec3 localTarget = agentPos + dirToGlobalTarget * localHorizon;

        glm::vec3 defaultP1 = agentPos + (localTarget - agentPos) * (1.0f / 3.0f);
        glm::vec3 defaultP2 = agentPos + (localTarget - agentPos) * (2.0f / 3.0f);

        glm::vec3 currentP1 = defaultP1;
        glm::vec3 currentP2 = defaultP2;
        glm::vec3 currentP3 = localTarget;

        BSplineCubic spline = { agentPos, currentP1, currentP2, currentP3 };

        glm::vec3 forceP1(0.0f);
        glm::vec3 forceP2(0.0f);
        glm::vec3 forceP3(0.0f);
        float weightP1Sum = 0.0f;
        float weightP2Sum = 0.0f;
        float weightP3Sum = 0.0f;       

        const int sampleCount = 20;
        for (int i = 0; i <= sampleCount; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(sampleCount);
           

            glm::vec3 ptOnCurve = spline.evaluate(t);
            float w1 = 3.0f * (1.0f - t) * (1.0f - t) * t;
            float w2 = 3.0f * (1.0f - t) * t * t;
            float w3 = t* t* t;

            for (const auto& obsPt : foundedPoints)
            {
                glm::vec3 diff = ptOnCurve - obsPt;
                float dist = glm::length(diff);

                if (dist < d_thresh && dist > 0.001f)
                {
                    glm::vec3 dir = glm::normalize(diff);
                    float penetration = d_thresh - dist;

                    glm::vec3 egoForce = dir * penetration * penetration;

                    forceP1 += egoForce * w1;
                    forceP2 += egoForce * w2;
                    forceP3 += egoForce * w3;
                    
                    weightP1Sum += w1;
                    weightP2Sum += w2;
                    weightP3Sum += w3;
                }
            }
        }

        
        if (weightP1Sum > 0.001f) {
            currentP1 += (forceP1 / weightP1Sum) * learningRate;
        }
        if (weightP2Sum > 0.001f) {
            currentP2 += (forceP2 / weightP2Sum) * learningRate;
        }
        if (weightP3Sum > 0.001f) {
            currentP3 += (forceP3 / weightP3Sum) * learningRate;
        }

        spline.P1 = currentP1;
        spline.P2 = currentP2;
        spline.P3 = currentP3;

        float t_lookahead = 0.25f;
        glm::vec3 targetWayPoint = spline.evaluate(t_lookahead);

        glm::vec3 moveDirection = targetWayPoint - agentPos;
        float moveLen = glm::length(moveDirection);

        if (moveLen > 0.001f) {
            moveDirection = glm::normalize(moveDirection) * k_att;
            targetYaw = std::atan2(moveDirection.x, moveDirection.z);
        }
        else {
            moveDirection = glm::vec3(0.0f);
        }

        splineSegment = generateSplineVertices(spline);

        return moveDirection;
    }

    std::vector<glm::vec3> generateSplineVertices(BSplineCubic& spline)
    {
        std::vector<glm::vec3> segmentPoints;
        for (int i = 0; i < 10; i++)
        {
            float t = static_cast<float>(i) / static_cast<float>(10);
            segmentPoints.push_back(spline.evaluate(t));
        }

        return segmentPoints;
    }
    std::vector<glm::vec3> generateSplineVertices(BSplineQuadratic& spline)
    {
        std::vector<glm::vec3> segmentPoints;
        for (int i = 0; i < 50; i++)
        {
            float t = static_cast<float>(i) / static_cast<float>(50);
            segmentPoints.push_back(spline.evaluate(t));
        }

        return segmentPoints;
    }

private:
    ZmqNode* publisher;
    Config m_cfg;
    int m_numAzimuth;
    int m_numElevation;
    glm::vec3 m_lastSelectedDir = glm::vec3(0.0f, 0.0f, 1.0f);
    std::vector<float> m_histogram;
};

#endif //_VFH_