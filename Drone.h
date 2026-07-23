#define _DRONE_
#ifdef _DRONE_

#include <vector>

class Drone
{
public:
    Window* myWindow;
    Model* drone;
    Model* propeler;
    PrimitiveObj* primObj;

    DebuggerClass* imgui_helper;

    struct SampledDepth {
        float ndcX;
        float ndcY;
        float value;
    };
    std::vector<SampledDepth> points, pointsPrevious;
    float middlePoint = 0.0f, previous_middlePoint = 0.0f;

	Drone(Window* _myWindow, Model* droneModel, Model* _propeler, PrimitiveObj* _primObj, DebuggerClass* _imgui_helper)
    {
        myWindow = _myWindow;
        drone = droneModel;
        propeler = _propeler;
        primObj = _primObj;
        imgui_helper = _imgui_helper;
        points.reserve(64);
        pointsPrevious.reserve(64);
    }

    bool resetPosition = false, resetPID = false;
    float currentYaw = 0.0f;
    float targetHeight = 0.0f;

    
    void gamepadControl(Gamepad &gamepad)
    {
        float pitchAvoid = 0.0f;
        float rollAvoid = 0.0f;

        calculateAvoidanceVector(pitchAvoid, rollAvoid);
        imgui_helper->rollValue = rollAvoid;
        imgui_helper->pitchValue= pitchAvoid;
        imgui_helper->middlePointDebug = middlePoint;

        float pitchInput = gamepad.getRightStickY(); // Valoare între -1.0 și 1.0
        float frontDistance = 1.0f - middlePoint;
        pitchInput = pitchInput - ((glm::pow(2.71828182845904523536, 30*(frontDistance-1.0f))) * imgui_helper->SENSITIVITY);
        glm::clamp(pitchInput, -1.2f, 1.2f);

        float rollInput = gamepad.getRightStickX();  // Valoare între -1.0 și 1.0
        if (std::abs(rollAvoid) > 0.008f)
        {
            rollInput = rollInput - (30.0f * rollAvoid * rollAvoid * rollAvoid * imgui_helper->SENSITIVITY);
            glm::clamp(rollInput, -1.2f, 1.2f);
        }

        float yawInput = gamepad.getLeftStickX();
        float yawSpeed = 0.02f;
        currentYaw -= yawInput * yawSpeed;

        imgui_helper->outputRoll = rollInput;
        imgui_helper->outputPitch = pitchInput;

        float maxAngleRadian = glm::radians(30.0f);
        if(gamepad.isButtonPressed(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER))
            maxAngleRadian = glm::radians(60.0f);

        float currentPitch = pitchInput * maxAngleRadian;
        float currentRoll = rollInput * maxAngleRadian;

        glm::quat yawQuat = glm::angleAxis(currentYaw, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::quat pitchQuat = glm::angleAxis(currentPitch, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat rollQuat = glm::angleAxis(currentRoll, glm::vec3(0.0f, 0.0f, 1.0f));

        
        glm::quat combinedQuat = yawQuat * pitchQuat * rollQuat;
        glm::vec3 correctedEuler = glm::eulerAngles(combinedQuat);

        imgui_helper->quatDummyTest.x = glm::degrees(correctedEuler.x);
        imgui_helper->quatDummyTest.y = glm::degrees(correctedEuler.y);
        imgui_helper->quatDummyTest.z = glm::degrees(correctedEuler.z);
        
        targetHeight += 0.1f * (gamepad.getRightTrigger());
        targetHeight -= 0.1f * (gamepad.getLeftTrigger());
        if (targetHeight > 30.0f)
            targetHeight = 30.0f;
        imgui_helper->droneTargetHeight = targetHeight;

        resetPosition = gamepad.isButtonPressed(GLFW_GAMEPAD_BUTTON_A);
        resetPID = gamepad.isButtonPressed(GLFW_GAMEPAD_BUTTON_X);
    }

    void depthProc(std::vector<unsigned char> &depthProc)
    {
        get36UniformNDCPoints(depthProc);
    }
    void get36UniformNDCPoints(std::vector<unsigned char>& depthData) {
        const int width = 1280;
        const int height = 720;
        const int count = 8;

        if (points.size() != 64) {
            points.resize(64);
        }

        if (pointsPrevious.size() != 64) {
            pointsPrevious.resize(64);
        }

        middlePoint = depthData[(360 * 1280 + 640) * 4] / 255.0f;

        static bool isInitialized = false;
        bool firstFrame = !isInitialized;

        pointsPrevious = points;

        const float stepX = static_cast<float>(width) / (count + 1);
        const float stepY = static_cast<float>(height) / (count + 1);

        int pointIndex = 0;

        std::vector<unsigned char> neighborhood(25);

        for (int r = 1; r <= count; ++r)
        {
            int y = static_cast<int>(r * stepY);
            if (y >= height) y = height - 1;

            for (int c = 1; c <= count; ++c)
            {
                int x = static_cast<int>(c * stepX);
                if (x >= width) x = width - 1;

                size_t index = (static_cast<size_t>(y) * width + x) * 4;

                if (index + 3 < depthData.size()) {

                    // --- MEDIANA 3x3 ---
                    int countValid = 0;
                    for (int dy = -2; dy <= 2; ++dy) {
                        int py = std::clamp(y + dy, 0, height - 1);
                        for (int dx = -2; dx <= 2; ++dx) {
                            int px = std::clamp(x + dx, 0, width - 1);
                            size_t nIdx = (static_cast<size_t>(py) * width + px) * 4;
                            neighborhood[countValid++] = depthData[nIdx];
                        }
                    }

                    // std::nth_element găsește mediana în O(N) fără să sorteze tot vectorul
                    std::nth_element(neighborhood.begin(), neighborhood.begin() + 4, neighborhood.end());
                    unsigned char medianRawValue = neighborhood[4]; // Valoarea mediană din ferestra 3x3

                    // --- CONVERSIE NDC & ADÂNCIME ---
                    float ndcX = (static_cast<float>(x) / (width - 1)) * 2.0f - 1.0f;
                    float ndcY = (static_cast<float>(y) / (height - 1)) * 2.0f - 1.0f;
                    float depthVal = medianRawValue / 255.0f;
                    float finalDepth = depthVal;

                    if (!firstFrame) {
                        float prevDepth = pointsPrevious[pointIndex].value;
                        finalDepth = applyAdaptiveFilter(depthVal, prevDepth);
                    }

                    // Salvăm punctul filtrat
                    points[pointIndex++] = { ndcX, ndcY, finalDepth };

                    // --- MARCARE VIZUALĂ A ZONEI (Debug Grid) ---
                    for (int dy = -4; dy < 4; ++dy) {
                        int py = y + dy;
                        if (py < 0 || py >= height) continue;

                        for (int dx = -4; dx < 4; ++dx) {
                            int px = x + dx;
                            if (px < 0 || px >= width) continue;

                            size_t pIndex = (static_cast<size_t>(py) * width + px) * 4;
                            depthData[pIndex + 2] = 255; // Marcare pe canalul Albastru (dacă e RGBA)
                        }
                    }
                }
            }
        }

        isInitialized = true; // După primul cadru rulat complet, trecem pe false
    }

    void calculateAvoidanceVector(float& pitch, float& roll) 
    {
        static float lastPitch = 0.0f;
        static float lastRoll = 0.0f;
        float totalPitchRepulsion = 0.0f;
        float totalRollRepulsion = 0.0f;
        float totalWeight = 0.0f;

        for (int i = 0; i < points.size(); i++) 
        {
            float speedDifference = pointsPrevious[i].value - points[i].value; //difference between first and second frame depth for checking ~speed increase
            if (std::abs(speedDifference) < 0.004f) { // ignoră fluctuația de exact 1 LSB (1/255)
                speedDifference = 0.0f;
            }
            float depthWeight = points[i].value - (1.0f * speedDifference);
            float spatialWeight = getCenterWeight(points[i].ndcX, points[i].ndcY);
            float combinedWeight = depthWeight * 1.5 * spatialWeight;

            totalRollRepulsion += -points[i].ndcX * depthWeight;
            totalPitchRepulsion += -points[i].ndcY * depthWeight;

            totalWeight += combinedWeight;
        }

        if (totalWeight > 0.0001f) {
            roll = totalRollRepulsion / totalWeight;
            pitch = totalPitchRepulsion / totalWeight;
        }
        else {
            roll = 0.0f;
            pitch = 0.0f;
        }
        roll = applyAdaptiveFilter(roll, lastRoll);
        pitch = applyAdaptiveFilter(pitch, lastPitch);
        middlePoint = applyAdaptiveFilter(middlePoint, previous_middlePoint);
        lastRoll = roll;
        lastPitch = pitch;
        previous_middlePoint = middlePoint;

        MIN_ALPHA = imgui_helper->MIN_ALPHA;
        MAX_ALPHA = imgui_helper->MAX_ALPHA;
        NOISE_THRESHOLD = imgui_helper->NOISE_THRESHOLD;
        JUMP_THRESHOLD = imgui_helper->JUMP_THRESHOLD;
    }

	void flyDrone(Shader pbrShader)
	{
        bool inteligenta_artificiala = imgui_helper->inteligenta_artificiala;
        float dt = 1.0f / 480.0f;
        glm::vec3 targetPosition = imgui_helper->targetPosition;
        float targetYawAngle = 0.0f;

        if (imgui_helper->maxPower < 0.01f)
            imgui_helper->maxPower = 0.0f;
        if (imgui_helper->minPower >= imgui_helper->maxPower)
            imgui_helper->minPower = imgui_helper->maxPower;

        if (glfwGetKey(myWindow->window, GLFW_KEY_R) == GLFW_PRESS || (resetPID == true))
        {
            pidPitch.Reset();
            pidRoll.Reset();
            pidYaw.Reset();
            pidHeight.Reset();
            pidX.Reset();
            pidZ.Reset();
        }

        if (glfwGetKey(myWindow->window, GLFW_KEY_T) == GLFW_PRESS || (resetPosition == true))
        {
            PhysicsEngine::getInstance().resetBody(drone->physics_id, JPH::Vec3(-1.0f, 2.0f, 2.0f), JPH::Quat::sIdentity());
            targetPosition = glm::vec3(-1.0f, 2.0f, 2.0f);
            imgui_helper->quatDummyTest.x = imgui_helper->quatDummyTest.y = imgui_helper->quatDummyTest.z = 0;
        }

        glm::vec3 directieRacheta(0.0f, 1.0f, 0.0f);
        glm::vec3 positionPropeler(1.65f, -0.45, 1.4f);
        glm::vec3 posPropeller_FRONTLEFT = imgui_helper->posPropeller_FRONTLEFT;
        glm::vec3 posPropeller_BACKLEFT = imgui_helper->posPropeller_BACKLEFT;
        glm::vec3 posPropeller_FRONTRIGHT = imgui_helper->posPropeller_FRONTRIGHT;
        glm::vec3 posPropeller_BACKRIGHT = imgui_helper->posPropeller_BACKRIGHT;

        glm::vec3 coltLocal0(posPropeller_FRONTLEFT);  // FRONT LEFT
        glm::vec3 coltLocal1(posPropeller_BACKLEFT); // BACK LEFT
        glm::vec3 coltLocal2(posPropeller_FRONTRIGHT); // FRONT RIGHT
        glm::vec3 coltLocal3(posPropeller_BACKRIGHT);// BACK RIGHT


        glm::vec3 currentPosition;
        glm::quat currentQuat;
        glm::vec3 currentAngVel, worldLinearVel;
        PhysicsEngine::getInstance().getModelMatrix(drone->physics_id, currentPosition, currentQuat);
        PhysicsEngine::getInstance().GetAngularVelocity(drone->physics_id, currentAngVel);
        PhysicsEngine::getInstance().GetLinearVelocity(drone->physics_id, worldLinearVel);
        glm::vec3 localAngVel = glm::inverse(currentQuat) * currentAngVel;

        imgui_helper->currentPos = currentPosition;

        // --- A. CONTROL ALTITUDINE (Axa Y) ---
        float heightError;
        if (inteligenta_artificiala) {
            heightError = targetPosition.y - currentPosition.y;
        }
        else
            heightError = targetHeight - currentPosition.y;

        // PID pe Y: P folosește heightError, D folosește viteza verticală curentă (worldLinearVel.y)
        float heightCorrection = pidHeight.Update(heightError, worldLinearVel.y, dt, 10.0f);

        float masaDronei = 1.0f; // Asigură-te că coincide cu masa setată în BodyCreationSettings în Jolt
        float hoverThrust = masaDronei * 9.81f;
        float fortaRacheta = hoverThrust + heightCorrection;

        // Compensation for Tilt (Când drona e înclinată, pierde din forța de ridicare pe verticală)
        glm::vec3 droneUpWorld = currentQuat * glm::vec3(0.0f, 1.0f, 0.0f); // Axul local Y în World Space
        float cosinusInclinare = glm::dot(droneUpWorld, glm::vec3(0.0f, 1.0f, 0.0f));
        cosinusInclinare = std::clamp(cosinusInclinare, 0.3f, 1.0f); // Evităm împărțirea la 0 dacă se răstoarnă
        fortaRacheta = fortaRacheta / cosinusInclinare;

        // Limitare Thrust Total
        fortaRacheta = std::clamp(fortaRacheta, imgui_helper->minPower, imgui_helper->getMaxPower());

        float errorX = targetPosition.x - currentPosition.x;
        float errorZ = targetPosition.z - currentPosition.z;

        float worldAccelX = pidX.Update(errorX, worldLinearVel.x, dt, 5.0f);
        float worldAccelZ = pidZ.Update(errorZ, worldLinearVel.z, dt, 5.0f);

        glm::vec3 forwardDir = currentQuat * glm::vec3(0.0f, 0.0f, -1.0f);
        float currentYaw = atan2f(-forwardDir.x, -forwardDir.z); // Unghiul Yaw în radiani
        float cosYaw = cosf(currentYaw);
        float sinYaw = sinf(currentYaw);

        float localAccelRight = worldAccelX * cosYaw - worldAccelZ * sinYaw; // Pe axa X locală
        float localAccelForward = -worldAccelX * sinYaw - worldAccelZ * cosYaw; // Pe axa Z locală

        float MAX_TILT = glm::radians(60.0f);
        float targetPitch = std::clamp(-localAccelForward / 9.81f, -MAX_TILT, MAX_TILT);
        float targetRoll = std::clamp(-localAccelRight / 9.81f, -MAX_TILT, MAX_TILT);
        float targetYaw = targetYawAngle;

        
        // TEST !!!!!
        glm::vec3 convert;
        glm::quat quaternionDum; // Test or drone movementx=
        if (inteligenta_artificiala) {
            glm::quat qPitch = glm::angleAxis(targetPitch, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::quat qYaw = glm::angleAxis(targetYawAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat qRoll = glm::angleAxis(targetRoll, glm::vec3(0.0f, 0.0f, 1.0f));
            quaternionDum = qYaw * qPitch * qRoll;
        }
        else {
            convert = glm::vec3(glm::radians(imgui_helper->quatDummyTest.x), glm::radians(imgui_helper->quatDummyTest.y), glm::radians(imgui_helper->quatDummyTest.z));
            quaternionDum = glm::quat(convert);
        }

        glm::quat targetRot = quaternionDum;
        glm::quat errorQuat = glm::inverse(currentQuat) * targetRot;
        if (errorQuat.w < 0.0f) errorQuat = -errorQuat;

        float angle = 2.0f * acosf(std::clamp(errorQuat.w, -1.0f, 1.0f));
        glm::vec3 rotationError(0.0f);
        if (angle > 0.0001f) {
            // Evităm împărțirea la 0 când suntem perfect aliniați
            float sinHalfAngle = sqrtf(1.0f - errorQuat.w * errorQuat.w);
            if (sinHalfAngle > 0.0001f) {
                rotationError = glm::vec3(errorQuat.x, errorQuat.y, errorQuat.z) / sinHalfAngle * angle;
            }
        }

        float pitchCorectie = pidPitch.Update(rotationError.x, localAngVel.x, dt, fortaRacheta);
        float yawCorectie = pidYaw.Update(rotationError.y, localAngVel.y, dt, fortaRacheta);
        float rollCorectie = pidRoll.Update(rotationError.z, localAngVel.z, dt, fortaRacheta);

        float throttle = fortaRacheta;

        float fortaM0 = throttle - pitchCorectie + rollCorectie - yawCorectie;
        float fortaM1 = throttle + pitchCorectie - rollCorectie - yawCorectie;
        float fortaM2 = throttle - pitchCorectie - rollCorectie + yawCorectie;
        float fortaM3 = throttle + pitchCorectie + rollCorectie + yawCorectie;

        float MAX_FORCE = imgui_helper->getMaxPower();
        fortaM0 = std::clamp(fortaM0, 0.0f, MAX_FORCE);
        fortaM1 = std::clamp(fortaM1, 0.0f, MAX_FORCE);
        fortaM2 = std::clamp(fortaM2, 0.0f, MAX_FORCE);
        fortaM3 = std::clamp(fortaM3, 0.0f, MAX_FORCE);

        PhysicsEngine::getInstance().ApplyRocketForce(drone->physics_id, coltLocal0, directieRacheta, fortaM0, -1.0f); // Front Left
        PhysicsEngine::getInstance().ApplyRocketForce(drone->physics_id, coltLocal2, directieRacheta, fortaM2, 1.0f);  // Front Right
        PhysicsEngine::getInstance().ApplyRocketForce(drone->physics_id, coltLocal1, directieRacheta, fortaM3, 1.0f);  // Back Left
        PhysicsEngine::getInstance().ApplyRocketForce(drone->physics_id, coltLocal3, directieRacheta, fortaM1, -1.0f); // Back Right

        propeler->move(coltLocal0); // M0
        propeler->rotate_Q(drone->quaternion);
        propeler->scale(0.5);
        pbrShader.setVec3("u_albedo", glm::vec3(fortaM0 / MAX_FORCE, 0.0f, 0.0f));
        propeler->draw(pbrShader);

        propeler->move(coltLocal1); // M3
        propeler->rotate_Q(drone->quaternion);
        propeler->scale(0.5f);
        pbrShader.setVec3("u_albedo", glm::vec3(0.0f, fortaM3 / MAX_FORCE, 0.0f));
        propeler->draw(pbrShader);

        propeler->move(coltLocal2); // M2 
        propeler->rotate_Q(drone->quaternion);
        propeler->scale(0.5f);
        pbrShader.setVec3("u_albedo", glm::vec3(0.0f, 0.0f, fortaM2 / MAX_FORCE));
        propeler->draw(pbrShader);

        propeler->move(coltLocal3); // M1 
        propeler->rotate_Q(drone->quaternion);
        propeler->scale(0.5f);
        pbrShader.setVec3("u_albedo", glm::vec3(fortaM1 / MAX_FORCE, fortaM1 / MAX_FORCE, 0.0f));
        propeler->draw(pbrShader);

        primObj->move(glm::vec3(2.0f, 2.0f, -1.0f));
        primObj->rotate_Q(quaternionDum);
        primObj->renderCube_shader(pbrShader);

        imgui_helper->quatDebug = drone->quaternion;

        PhysicsEngine::getInstance().run();
	}

private:
    float MIN_ALPHA = 0.05f;  
    float MAX_ALPHA = 1.00f;  
    float NOISE_THRESHOLD = 0.05f;  
    float JUMP_THRESHOLD = 0.15f; 

    float applyAdaptiveFilter(float currentRaw, float previousFiltered) {
        float delta = std::abs(currentRaw - previousFiltered);

        if (delta < NOISE_THRESHOLD) {
            return previousFiltered;
        }

        float factor = (delta - NOISE_THRESHOLD) / (JUMP_THRESHOLD - NOISE_THRESHOLD);
        factor = std::clamp(factor, 0.0f, 1.0f);

        float dynamicAlpha = MIN_ALPHA + factor * (MAX_ALPHA - MIN_ALPHA);

        return dynamicAlpha * currentRaw + (1.0f - dynamicAlpha) * previousFiltered;
    }

    float dampenVibration(float current, float target, float& velocity, float damping, float springK, float dt) {
        float force = (target - current) * springK;
        float dampingForce = velocity * damping;

        float acceleration = force - dampingForce;
        velocity += acceleration * dt;
        current += velocity * dt;

        return current;
    }


    float getCenterWeight(float ndcX, float ndcY) {
        float distFromCenter = std::sqrt(ndcX * ndcX + ndcY * ndcY);
        float t = 1.0f - distFromCenter;
        t = std::clamp(t, 0.0f, 1.0f);
        return getGaussianWeight(t);
    }
    float getGaussianWeight(float x, float a = 2.6f, float b = 6.0f) {
        return glm::exp(-a * glm::pow(glm::abs(x), b));
    }

};

#endif // !_DRONE_
