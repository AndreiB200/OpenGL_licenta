#ifndef FRAMECAPTURER_H
#define FRAMECAPTURER_H

#include <vector>
#include <cstdint>
#include <algorithm>

class FrameCapturer {
public:
    FrameCapturer(int width, int height)
        : m_width(width),
        m_height(height),
        m_bufferSizeColor(width* height * 4 * sizeof(uint8_t)), 
        m_bufferSizeDepth(width* height * 1 * sizeof(float))
    {
        glGenBuffers(2, m_pboColor);
        glGenBuffers(2, m_pboDepth);

        for (int i = 0; i < 2; ++i) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboColor[i]);
            glBufferData(GL_PIXEL_PACK_BUFFER, m_bufferSizeColor, nullptr, GL_STREAM_READ);

            glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboDepth[i]);
            glBufferData(GL_PIXEL_PACK_BUFFER, m_bufferSizeDepth, nullptr, GL_STREAM_READ);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        m_colorBuffer.resize(m_width * m_height * 4); 
        m_depthBuffer.resize(m_width * m_height);     
    }

    ~FrameCapturer() {
        glDeleteBuffers(2, m_pboColor);
        glDeleteBuffers(2, m_pboDepth);
    }

    template <typename DroneSim, typename Streamer>
    void CaptureAndProcess(DroneSim& droneSim, Streamer& streamer) {
        int writeIdx = m_pboIndex;
        int readIdx = (m_pboIndex + 1) % 2;

        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboColor[writeIdx]);
        glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboDepth[writeIdx]);
        glReadPixels(0, 0, m_width, m_height, GL_RED, GL_FLOAT, nullptr);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboColor[readIdx]);
        uint8_t* colorPtr = static_cast<uint8_t*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
        if (colorPtr) {
            std::copy(colorPtr, colorPtr + m_colorBuffer.size(), m_colorBuffer.begin());
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboDepth[readIdx]);
        float* depthPtr = static_cast<float*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
        if (depthPtr) {
            std::copy(depthPtr, depthPtr + m_depthBuffer.size(), m_depthBuffer.begin());
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        if (m_isReady) {
            droneSim.depthProc(m_depthBuffer);
            streamer.PushFrame(m_colorBuffer, m_depthBuffer);
        }
        else {
            m_isReady = true;
        }

        m_pboIndex = readIdx;
    }

private:
    int m_width;
    int m_height;
    size_t m_bufferSizeColor; 
    size_t m_bufferSizeDepth;

    GLuint m_pboColor[2];
    GLuint m_pboDepth[2];
    int m_pboIndex = 0;
    bool m_isReady = false;

    std::vector<uint8_t> m_colorBuffer;
    std::vector<float> m_depthBuffer;
};

#endif