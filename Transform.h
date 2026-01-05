#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum axis
{
    X, Y, Z,
};

class Transform
{
public:
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 size = glm::vec3(1.0f);
    float axisX, axisY, axisZ, degrees;
    glm::vec3 rotation = glm::vec3(0.0f);

    void move(float x, float y, float z)
    {
        position.x = x; position.y = y; position.z = z;
        model = glm::translate(model, position);
    }
    void move(glm::vec3 v_position)
    {
        position = v_position;
        model = glm::translate(model, position);
    }
    void move()
    {
        model = glm::translate(model, position);
    }

    void scale(float x, float y, float z)
    {
        size.x = x; size.y = y; size.z = z;
        model = glm::scale(model, size);
    }
    void scale(float x)
    {
        size = glm::vec3(x);
        model = glm::scale(model, size);
    }

    void rotate(float m_degrees, axis axisR)
    {
        degrees = m_degrees;
        if (axisR == X)
            rotation = glm::vec3(1.0f, 0.0f, 0.0f);
        if (axisR == Y)
            rotation = glm::vec3(0.0f, 1.0f, 0.0f);
        if (axisR == Z)
            rotation = glm::vec3(0.0f, 0.0f, 1.0f);

        model = glm::rotate(model, glm::radians(m_degrees), rotation);
    }

};