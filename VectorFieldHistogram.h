#define _VFH_
#ifdef _VFH_

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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

    glm::vec3 computeSteeringDirection(
        const glm::vec3& currentPos,
        const glm::vec3& forwardDir,
        const glm::vec3& upDir,
        const glm::vec3& targetPos,
        const std::vector<glm::vec3>& lidarCloud)
    {
        std::fill(m_histogram.begin(), m_histogram.end(), 0.0f);

        float sectorRad = glm::radians(m_cfg.sectorSizeDeg);
        float halfHFOV = glm::radians(m_cfg.horizontalFovDeg * 0.5f);
        float halfVFOV = glm::radians(m_cfg.verticalFovDeg * 0.5f);

        glm::vec3 fwd = glm::normalize(forwardDir);
        glm::vec3 right = glm::normalize(glm::cross(fwd, glm::normalize(upDir)));
        glm::vec3 up = glm::cross(right, fwd);

        for (const auto& ptWorld : lidarCloud) {
            glm::vec3 relPt = ptWorld - currentPos;
            float dist = glm::length(relPt);

            if (dist <= 0.001f || dist > m_cfg.sensorMaxRadius) continue;

            glm::vec3 normDir = relPt / dist;

            float localZ = glm::dot(normDir, fwd);
            float localX = glm::dot(normDir, right);
            float localY = glm::dot(normDir, up);

            if (localZ <= 0.0f) continue;

            float azimuth = std::atan2(localX, localZ);
            float elevation = std::asin(glm::clamp(localY, -1.0f, 1.0f));

            if (std::abs(azimuth) > halfHFOV || std::abs(elevation) > halfVFOV) continue;

            int azIdx = static_cast<int>((azimuth + halfHFOV) / sectorRad);
            int elIdx = static_cast<int>((elevation + halfVFOV) / sectorRad);

            azIdx = glm::clamp(azIdx, 0, m_numAzimuth - 1);
            elIdx = glm::clamp(elIdx, 0, m_numElevation - 1);

            float effectiveDist = std::max(0.1f, dist - m_cfg.safetyRadius);
            float weight = 1.0f - (effectiveDist / m_cfg.sensorMaxRadius);
            weight = std::max(0.0f, weight * weight);

            int histIdx = elIdx * m_numAzimuth + azIdx;
            m_histogram[histIdx] += weight;
        }

        glm::vec3 targetDirWorld = targetPos - currentPos;
        float targetDist = glm::length(targetDirWorld);
        if (targetDist < 0.001f) return fwd;

        glm::vec3 targetDirNorm = targetDirWorld / targetDist;

        float bestCost = std::numeric_limits<float>::max();
        glm::vec3 bestDirectionWorld = fwd;
        bool foundValidSector = false;

        const float wTarget = 1.0f;
        const float wForward = 0.4f;
        const float wPrevDir = 0.3f;

        for (int el = 0; el < m_numElevation; ++el) {
            for (int az = 0; az < m_numAzimuth; ++az) {
                int idx = el * m_numAzimuth + az;

                if (m_histogram[idx] < m_cfg.densityThreshold) {
                    float sectorAz = -halfHFOV + (az + 0.5f) * sectorRad;
                    float sectorEl = -halfVFOV + (el + 0.5f) * sectorRad;

                    glm::vec3 candidateDirLocal(
                        std::sin(sectorAz) * std::cos(sectorEl),
                        std::sin(sectorEl),
                        std::cos(sectorAz) * std::cos(sectorEl)
                    );

                    glm::vec3 candidateDirWorld = glm::normalize(
                        candidateDirLocal.x * right +
                        candidateDirLocal.y * up +
                        candidateDirLocal.z * fwd
                    );

                    float targetCost = 1.0f - glm::dot(candidateDirWorld, targetDirNorm);
                    float forwardCost = 1.0f - glm::dot(candidateDirWorld, fwd);
                    float prevDirCost = 1.0f - glm::dot(candidateDirWorld, m_lastSelectedDir);

                    float cost = wTarget * targetCost + wForward * forwardCost + wPrevDir * prevDirCost;

                    if (cost < bestCost) {
                        bestCost = cost;
                        bestDirectionWorld = candidateDirWorld;
                        foundValidSector = true;
                    }
                }
            }
        }

        if (!foundValidSector) {
            m_lastSelectedDir = bestDirectionWorld;
        }

        //publisher->sendHistogram(m_histogram); // This will be rechecked later

        return bestDirectionWorld;
    }

    glm::vec3 computeAPFSteering(
        const glm::vec3& agentPos,         
        const glm::vec3& targetPos,       
        const std::vector<glm::vec3>& lidarPointsWorld, 
        float d_max = 2.5f,                      
        float k_att = 2.0f,                      
        float k_rep = 5.0f                      
    ) {
        glm::vec3 f_att = k_att * (targetPos - agentPos);

        float max_att_force = 5.0f;
        if (glm::length(f_att) > max_att_force) {
            f_att = glm::normalize(f_att) * max_att_force;
        }

        glm::vec3 f_rep = glm::vec3(0.0f);

        for (const auto& pointWorld : lidarPointsWorld) {
            glm::vec3 diff = agentPos - pointWorld;
            float dist = glm::length(diff);

            if (dist < d_max && dist > 0.001f) {
                glm::vec3 dir = glm::normalize(diff);
                float factor = k_rep * ((1.0f / dist) - (1.0f / d_max)) * (1.0f / (dist * dist));
                f_rep += factor * dir;
            }
        }

        glm::vec3 f_total = f_att + f_rep;

        return f_total;
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