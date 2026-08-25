#ifndef PHYSICSENGINE_H
#define PHYSICSENGINE_H


#include <glad/glad.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>
#include <memory>
#include <vector>
#include <stdint.h>
#include <random>

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Renderer/DebugRenderer.h>


JPH_SUPPRESS_WARNINGS

#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, unsigned int inLine)
{
	// Print to the TTY
	std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;

	// Breakpoint
	return true;
};
#endif // JPH_ENABLE_ASSERTS

namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
	virtual bool					ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr JPH::uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
	BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual JPH::uint					GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual JPH::BroadPhaseLayer			GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
	{
		switch ((JPH::BroadPhaseLayer::Type)inLayer)
		{
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	JPH::BroadPhaseLayer					mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool				ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

class MyBatchImpl : public JPH::RefTargetVirtual {
public:
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	unsigned int EBO = 0;
	GLsizei indexCount = 0;

	MyBatchImpl(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) {
		indexCount = inIndexCount;

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		// VBO: Folosim structura de Vertex nativă Jolt
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, inVertexCount * sizeof(JPH::DebugRenderer::Vertex), inVertices, GL_STATIC_DRAW);

		// EBO: Indicii
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, inIndexCount * sizeof(JPH::uint32), inIndices, GL_STATIC_DRAW);

		// Atribut 0: Position (JPH::Float3)
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(JPH::DebugRenderer::Vertex), (void*)offsetof(JPH::DebugRenderer::Vertex, mPosition));

		// Atribut 1: Normal (JPH::Float3) - util dacă vrei shading
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(JPH::DebugRenderer::Vertex), (void*)offsetof(JPH::DebugRenderer::Vertex, mNormal));

		// Atribut 2: UV (JPH::Float2)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(JPH::DebugRenderer::Vertex), (void*)offsetof(JPH::DebugRenderer::Vertex, mUV));

		// Atribut 3: Color (JPH::Color) - 4 octeți unorm (0-255)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(JPH::DebugRenderer::Vertex), (void*)offsetof(JPH::DebugRenderer::Vertex, mColor));

		glBindVertexArray(0);
	}

	~MyBatchImpl() {
		if (VAO) glDeleteVertexArrays(1, &VAO);
		if (VBO) glDeleteBuffers(1, &VBO);
		if (EBO) glDeleteBuffers(1, &EBO);
	}

	virtual void AddRef() override { mRefCount++; }
	virtual void Release() override { if (--mRefCount == 0) delete this; }

private:
	std::atomic<uint32_t> mRefCount = 0;
};

class MyJoltDebugRenderer : public JPH::DebugRenderer {
private:
	struct Vertex {
		glm::vec3 position;
		glm::vec4 color;
	};

	std::vector<Vertex> m_LineVertices;
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	GLsizei indexCount = 0;
	Shader debug = Shader("bbVisual.vert", "bbVisual.frag");

public:
	JPH_OVERRIDE_NEW_DELETE
	MyJoltDebugRenderer() {
		Initialize(); // Inițializarea Jolt

		// Generăm VAO și VBO dinamic pentru linii
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		// Alocăm spațiu inițial gol, îl vom popula la fiecare cadru cu glBufferSubData
		glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

		// Atributul 1: Culoare
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

		glBindVertexArray(0);
	}

	~MyJoltDebugRenderer() {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}

	void setShader(glm::mat4& view, glm::mat4& projection)
	{
		debug.use();
		debug.setMat4("view", view);
		debug.setMat4("projection", projection);
	}

	// Jolt apelează asta pentru fiecare linie din scenă
	virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
		JPH::Color c = inColor;
		glm::vec4 color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);

		m_LineVertices.push_back({ glm::vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ()), color });
		m_LineVertices.push_back({ glm::vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ()), color });
	}

	// Funcția apelată la finalul cadru-lui pentru a trimite totul la GPU și a randa
	void Render(const glm::mat4& view, const glm::mat4& projection) {
		if (m_LineVertices.empty())
		{
			return;
		}

		glUseProgram(debug.ID);

		// Trimitem doar matricile globale (punctele Jolt sunt deja în World Space)
		glUniformMatrix4fv(glGetUniformLocation(debug.ID, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(debug.ID, "projection"), 1, GL_FALSE, &projection[0][0]);

		glm::mat4 model = glm::mat4(1.0f); // Identity
		glUniformMatrix4fv(glGetUniformLocation(debug.ID, "model"), 1, GL_FALSE, &model[0][0]);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		// Încărcăm toate liniile acumulate în acest cadru direct în GPU
		glBufferData(GL_ARRAY_BUFFER, m_LineVertices.size() * sizeof(Vertex), m_LineVertices.data(), GL_DYNAMIC_DRAW);
		// Desenăm liniile
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_LineVertices.size()));

		glBindVertexArray(0);

		// !! CRUCIAL: Golim vectorul pentru cadrul următor, altfel se acumulează la infinit
		m_LineVertices.clear();
	}

	// 1. DrawTriangle
	virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, JPH::DebugRenderer::ECastShadow inCastShadow) override 
	{
		DrawLine(inV1, inV2, inColor);
		DrawLine(inV2, inV3, inColor);
		DrawLine(inV3, inV1, inColor);
	}

	// 2. CreateTriangleBatch (Varianta cu Vertex brut)
	virtual JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) override {
		return new MyBatchImpl(inVertices, inVertexCount, inIndices, inIndexCount);
	}

	// 3. CreateTriangleBatch (Varianta cu Triangle structure)
	virtual JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount) override {
		std::vector<JPH::DebugRenderer::Vertex> vertices;
		std::vector<JPH::uint32> indices;

		vertices.reserve(inTriangleCount * 3);
		indices.reserve(inTriangleCount * 3);

		for (int i = 0; i < inTriangleCount; ++i) {
			const auto& tri = inTriangles[i];

			// Fiecare triunghi are 3 noduri (mV[0], mV[1], mV[2])
			for (int j = 0; j < 3; ++j) {
				vertices.push_back(tri.mV[j]);
				indices.push_back(static_cast<JPH::uint32>(vertices.size() - 1));
			}
		}

		return CreateTriangleBatch(vertices.data(), static_cast<int>(vertices.size()), indices.data(), static_cast<int>(indices.size()));
	}

	// 4. DrawGeometry (Aici era problema principală: GeometryRef în loc de Batch)
	virtual void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const JPH::DebugRenderer::GeometryRef& inGeometry, JPH::DebugRenderer::ECullMode inCullMode, JPH::DebugRenderer::ECastShadow inCastShadow, JPH::DebugRenderer::EDrawMode inDrawMode) override 
	{
		// Culegem LOD-ul potrivit (LOD 0 este cel mai detaliat)
		const JPH::DebugRenderer::LOD& lod = inGeometry->mLODs[0];
		MyBatchImpl* batch = static_cast<MyBatchImpl*>(lod.mTriangleBatch.GetPtr());

		if (!batch || batch->VAO == 0) return;

		// Convertim matricea din format Jolt (Column-Major) în glm::mat4
		glm::mat4 model;
		JPH::Mat44 m44 = inModelMatrix.ToMat44();
		for (int r = 0; r < 4; ++r) {
			JPH::Vec4 col = m44.GetColumn4(r);
			model[r] = glm::vec4(col.GetX(), col.GetY(), col.GetZ(), col.GetW());
		}

		// Setăm Culling-ul cerut de Jolt
		if (inCullMode == JPH::DebugRenderer::ECullMode::Off) {
			glDisable(GL_CULL_FACE);
		}
		else {
			glEnable(GL_CULL_FACE);
			glCullFace(inCullMode == JPH::DebugRenderer::ECullMode::CullBackFace ? GL_BACK : GL_FRONT);
		}

		// Dacă vreți Wireframe pentru a vedea triunghiurile clar:
		if (inDrawMode == JPH::DebugRenderer::EDrawMode::Wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		// Aici folosești Shader-ul tău de geometrie/debug
		// Pasăm matricea `model` și culoarea `inModelColor`
		glUniformMatrix4fv(glGetUniformLocation(debug.ID, "model"), 1, GL_FALSE, &model[0][0]);


		glBindVertexArray(batch->VAO);
		glDrawElements(GL_TRIANGLES, batch->indexCount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Resetăm starea la normal
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}

	// 5. DrawText3D
	virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight = 0.5f) override {}
};

class NoiseForceGenerator {
private:
	std::mt19937 m_RNG;
	std::uniform_real_distribution<float> m_Distribution;
	float m_MaxForce;

public:
	NoiseForceGenerator() {
		std::random_device rd;
		m_RNG.seed(rd());

		m_Distribution = std::uniform_real_distribution<float>(1.0f, 1.0f);
	}

	float GetNextNoise(float force) {
		return m_Distribution(m_RNG) * force;
	}
};

//struct PIDController {
//	float kp, ki, kd;
//	float integral = 0.0f;
//	float prevError = 0.0f;
//
//	float Update(float error, float dt, float force = 3.0f) {
//		integral += error * dt;
//		integral = glm::clamp(integral, -force, force);
//
//		float derivative = (error - prevError) / dt;
//		prevError = error;
//
//		return (error * kp) + (integral * ki) + (derivative * kd);
//	}
//
//	void Reset() {
//		integral = 0.0f;
//		prevError = 0.0f;
//	}
//};

struct PIDController {
	float kp = 0.0f;
	float ki = 0.0f;
	float kd = 0.0f;

	float integral = 0.0f;

	// varianta optimizată pentru D bazat pe Gyro/AngVel
	float Update(float error, float velocity, float dt, float maxTorque = 50.0f) {
		// 1. Protecție împotriva dt invalid (dacă framerate-ul pică sau e pasul 0)
		if (dt <= 0.00001f) return 0.0f;

		// 2. Integrator cu Anti-Windup
		integral += error * dt;
		integral = glm::clamp(integral, -maxTorque, maxTorque);

		// 3. Calcul P, I, D
		float P = error * kp;
		float I = integral * ki;

		// D se opune vitezei de rotație (se pune cu MINUS)
		float D = -velocity * kd;

		float output = P + I + D;

		// 4. Clamping pe ieșirea totală pentru siguranță în Jolt
		return glm::clamp(output, -maxTorque, maxTorque);
	}

	void Reset() {
		integral = 0.0f;
	}
};

PIDController pidPitch{ 3.1f, 0.2f, 0.62f };
PIDController pidRoll{ 1.8f, 0.2f, 0.30f };
PIDController pidYaw{ 2.0f, 0.1f, 1.2f };
PIDController pidHeight{ 11.2f, 2.5f, 18.4f };
PIDController pidX{ 1.1f, 0.0f, 2.0f };
PIDController pidZ{ 1.1f, 0.0f, 2.0f };


class PhysicsEngine
{
public:
	static PhysicsEngine& getInstance()
	{
		static PhysicsEngine instance;
		return instance;
	}

	PhysicsEngine(const PhysicsEngine&) = delete;
	void operator=(const PhysicsEngine&) = delete;

	void init()
	{
		temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024); // 10 MB buffer temporar
		job_system = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		physics_system.Init(1024, 0, 1024, 1024, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

		JPH::BoxShapeSettings floor_settings(JPH::Vec3(50.0f, 0.5f, 50.0f));
		JPH::BodyCreationSettings floor_cube(floor_settings.Create().Get(), JPH::RVec3(0.0f, -0.5f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
		floor_id = body_interface.CreateBody(floor_cube)->GetID();

		body_interface.AddBody(floor_id, JPH::EActivation::DontActivate);

		physics_system.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

		jolt_debug_renderer = std::make_unique<MyJoltDebugRenderer>();
		JPH::DebugRenderer::sInstance = jolt_debug_renderer.get();
	}

	JPH::BodyID createBodyDynamic(glm::vec3 size, glm::vec3 positioning, glm::quat rotation)
	{
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

		JPH::BoxShapeSettings box_settings(JPH::Vec3(size.x, size.y, size.z));
		JPH::ShapeSettings::ShapeResult box_shape_result = box_settings.Create();
		JPH::Ref<JPH::Shape> box_shape = box_shape_result.Get();

		JPH::RVec3 position(positioning.x, positioning.y, positioning.z);
		//glm::quat quater(glm::vec3(glm::radians(30.0f), glm::radians(45.0f), glm::radians(0.0f)));
		//JPH::Quat rotation(quater.x, quater.y, quater.z, quater.w);
		JPH::Quat quat(rotation.x, rotation.y, rotation.z, rotation.w);

		JPH::BodyCreationSettings cube_settings(
			box_shape,
			position,
			quat,
			JPH::EMotionType::Dynamic,
			Layers::MOVING
		);

		cube_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		cube_settings.mMassPropertiesOverride.mMass = 1.0f;

		JPH::BodyID body_id;
		body_id = body_interface.CreateBody(cube_settings)->GetID();
		body_interface.AddBody(body_id, JPH::EActivation::Activate);

		return body_id;
	}

	JPH::BodyID createBodyStatic(glm::vec3 size, glm::vec3 positioning, glm::quat rotation)
	{
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

		JPH::BoxShapeSettings box_settings(JPH::Vec3(size.x, size.y, size.z));
		JPH::ShapeSettings::ShapeResult box_shape_result = box_settings.Create();
		JPH::Ref<JPH::Shape> box_shape = box_shape_result.Get();

		JPH::RVec3 position(positioning.x, positioning.y, positioning.z);
		//glm::quat quater(glm::vec3(glm::radians(30.0f), glm::radians(45.0f), glm::radians(0.0f)));
		//JPH::Quat rotation(quater.x, quater.y, quater.z, quater.w);
		JPH::Quat quat(rotation.x, rotation.y, rotation.z, rotation.w);

		JPH::BodyCreationSettings cube_settings(
			box_shape,
			position,
			quat,
			JPH::EMotionType::Static,
			Layers::NON_MOVING
		);

		cube_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		cube_settings.mMassPropertiesOverride.mMass = 1.0f;

		JPH::BodyID body_id;
		body_id = body_interface.CreateBody(cube_settings)->GetID();
		body_interface.AddBody(body_id, JPH::EActivation::DontActivate);

		return body_id;
	}

	JPH::BodyID createBodyConvexHull(std::vector<JPH::Vec3> &joltVertices, glm::vec3 size, glm::vec3 positioning, glm::quat rotation)
	{
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		for (int i = 0; i < joltVertices.size(); i++)
		{
			JPH::Vec3 boxSize(size.x, size.y, size.z);
			joltVertices[i] = joltVertices[i] * boxSize;
		}
		JPH::ConvexHullShapeSettings convexHullSettings(joltVertices.data(), joltVertices.size());
		JPH::Shape::ShapeResult result = convexHullSettings.Create();

		if (result.IsValid())
		{
			JPH::Ref<JPH::Shape> convexCollider = result.Get();
		}
		else
		{
			std::cout << "Eroare Jolt: " << result.GetError() << std::endl;
			exit(1);
		}
		JPH::Ref<JPH::Shape> body_shape = result.Get();

		JPH::RVec3 position(positioning.x, positioning.y, positioning.z);
		JPH::Quat quat(rotation.x, rotation.y, rotation.z, rotation.w);

		JPH::BodyCreationSettings bodySettings(
			body_shape,
			position,
			quat,
			JPH::EMotionType::Dynamic,
			Layers::MOVING
		);

		JPH::MassProperties massProperties;
		massProperties.mMass = 0.6f; // 1 kg
		massProperties.mInertia = JPH::Mat44::sScale(JPH::Vec3(0.005f, 0.005f, 0.009f));

		bodySettings.mMassPropertiesOverride = massProperties;
		bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;

		JPH::BodyID body_id;
		body_id = body_interface.CreateBody(bodySettings)->GetID();
		body_interface.AddBody(body_id, JPH::EActivation::Activate);

		return body_id;

	}

	void convertJPHtoGLM(JPH::BodyID body_id, glm::vec3 &position, glm::quat &quaternion)
	{
		JPH::RVec3 currentPos;
		JPH::Quat currentRot;
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		body_interface.GetPositionAndRotation(body_id, currentPos, currentRot);

		position = glm::vec3(currentPos.GetX(), currentPos.GetY(), currentPos.GetZ());
		quaternion =  glm::quat(currentRot.GetW(), currentRot.GetX(), currentRot.GetY(), currentRot.GetZ());
	}

	void ApplyRocketForce(JPH::BodyID physics_id, glm::vec3& localOffset, const glm::vec3& localForceDirection, float forceMagnitude, float torqueDirectionSign) {
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		glm::vec3 glmPos;
		glm::quat glmRot;
		convertJPHtoGLM(physics_id, glmPos, glmRot);

		glm::vec3 worldForcePoint = glmPos + (glmRot * localOffset); // Offset from the main object applied for rotation, snipped to corner
		//glm::vec3 worldForcePointApplied = glmPos + (glmRot * localOffset * (1.0f + (0.2f * noise.GetNextNoise(1.0f))));

		localOffset = worldForcePoint;
		float forceMagnitudeNOISE = noise.GetNextNoise(forceMagnitude);
		glm::vec3 worldForceVector = (glmRot * localForceDirection) * forceMagnitude; // Direction of the power related to offset + how much power

		JPH::RVec3 joltWorldPoint(worldForcePoint.x, worldForcePoint.y, worldForcePoint.z);
		JPH::Vec3 joltWorldForce(worldForceVector.x, worldForceVector.y, worldForceVector.z);

		body_interface.AddForce(physics_id, joltWorldForce, joltWorldPoint);

		float torqueRelationFactor = 0.02f;
		float torqueMagnitude = forceMagnitudeNOISE * torqueRelationFactor * torqueDirectionSign;

		glm::vec3 worldTorqueVector = (glmRot * localForceDirection) * torqueMagnitude;
		JPH::Vec3 joltWorldTorque(worldTorqueVector.x, worldTorqueVector.y, worldTorqueVector.z);

		body_interface.AddTorque(physics_id, joltWorldTorque);
	}

	void GetAngularVelocity(JPH::BodyID physics_id, glm::vec3 &angularVelocity) 
	{
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		JPH::Vec3 joltAngVel = body_interface.GetAngularVelocity(physics_id);
		angularVelocity = glm::vec3(joltAngVel.GetX(), joltAngVel.GetY(), joltAngVel.GetZ());
	}

	void GetLinearVelocity(JPH::BodyID physics_id, glm::vec3 &linearVelocity)
	{
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		JPH::Vec3 joltLinearVel = body_interface.GetLinearVelocity(physics_id);
		linearVelocity = glm::vec3(joltLinearVel.GetX(), joltLinearVel.GetY(), joltLinearVel.GetZ());
	}

	void getModelMatrix(JPH::BodyID bodyID, glm::vec3& pos, glm::quat& quat)
	{
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		JPH::RVec3 position;
		JPH::Quat rotation;
		body_interface.GetPositionAndRotation(bodyID, position, rotation);

		pos = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
		quat = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
	}

	void resetBody(JPH::BodyID bodyID, const JPH::RVec3& startPosition, const JPH::Quat& startRotation) 
	{
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		body_interface.SetPositionAndRotation(bodyID, startPosition, startRotation, JPH::EActivation::Activate);
		{
			JPH::BodyLockWrite lock(physics_system.GetBodyLockInterface(), bodyID);
			if (lock.Succeeded())
			{
				JPH::Body& body = lock.GetBody();
				body.ResetForce();
				body.ResetTorque();
				body.ResetMotion(); // Pune vitezele pe 0 în noua locatie
			}
		}
	}

	void showBodyInfo(JPH::BodyID bodyID)
	{
		if (!bodyID.IsInvalid())
		{
			JPH::BodyLockRead lock(physics_system.GetBodyLockInterfaceNoLock(), bodyID);
			if (lock.Succeeded())
			{
				const JPH::Body& dronaBody = lock.GetBody();
				const JPH::Shape* shape = dronaBody.GetShape();
				JPH::AABox bounds = shape->GetLocalBounds();
				JPH::Vec3 extents = bounds.GetExtent();

				const JPH::MotionProperties* prop = dronaBody.GetMotionProperties();

				std::cout << "--- DIAGNOSTIC JOLT DRONA (VIA BODY ID) ---" << std::endl;
				std::cout << "Dimensiune Box Local (Jumatati de Axe X, Y, Z): "
					<< extents.GetX() << ", " << extents.GetY() << ", " << extents.GetZ() << std::endl;

				if (prop != nullptr)
				{
					std::cout << "Masa totala înregistrata: " << 1.0f / prop->GetInverseMass() << " kg" << std::endl;

					JPH::Mat44 invInertiaLocalMat = prop->GetLocalSpaceInverseInertia();

					float invIxx = invInertiaLocalMat.GetColumn4(0).GetX(); // Coloana 0, Elementul X (linia 0)
					float invIyy = invInertiaLocalMat.GetColumn4(1).GetY(); // Coloana 1, Elementul Y (linia 1)
					float invIzz = invInertiaLocalMat.GetColumn4(2).GetZ(); // Coloana 2, Elementul Z (linia 2)

					std::cout << "Inversa Inertiei Locale (1/I) pe X, Y, Z: "
						<< invIxx << ", " << invIyy << ", " << invIzz << std::endl;

					// 3. Calculăm Tensorul de Inerție Real (I) prin inversare
					std::cout << "Tensorul de Inertie Real (I) local pe X, Y, Z: "
						<< (invIxx > 0.0f ? 1.0f / invIxx : 0.0f) << ", "
						<< (invIyy > 0.0f ? 1.0f / invIyy : 0.0f) << ", "
						<< (invIzz > 0.0f ? 1.0f / invIzz : 0.0f) << std::endl;
				}
				else
				{
					std::cout << "Eroare: Corpul este marcat ca Static/Kinematic! Nu are proprietăți de mișcare." << std::endl;
				}
				std::cout << "------------------------------------------" << std::endl;
			}
			else
			{
				std::cout << "Eroare: Nu s-a putut bloca corpul. ID-ul ar putea fi invalid sau corpul a fost șters." << std::endl;
			}
		}
	}

	void drawDebug(glm::mat4& view, glm::mat4& projection) 
	{
		jolt_debug_renderer->setShader(view, projection);

		JPH::BodyManager::DrawSettings settings;
		settings.mDrawShape = true;
		settings.mDrawShapeWireframe = true;
		settings.mDrawBoundingBox = true; // Va desena linii pentru Box-ul corpului!

		// 1. Îi cerem lui Jolt să populeze vectorul m_LineVertices prin functia DrawLine
		physics_system.DrawBodies(settings, jolt_debug_renderer.get());

		// 2. Randăm bufferul acumulat pe ecran
		jolt_debug_renderer->Render(view, projection);
	}

	const float getPhysicsStep()
	{
		return physicsTimeStep;
	}

	void run()
	{
		physics_system.Update(physicsTimeStep, 1, temp_allocator.get(), job_system.get());
	}
	
	NoiseForceGenerator noise;
private:
	PhysicsEngine(){
		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();
	}
	
	const float physicsTimeStep = 1.0f / 400.0f;
	
	// System
	JPH::PhysicsSystem physics_system;
	std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator;
	std::unique_ptr<JPH::JobSystemThreadPool> job_system;
	std::unique_ptr<MyJoltDebugRenderer> jolt_debug_renderer;

	BPLayerInterfaceImpl broad_phase_layer_interface;
	ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
	ObjectLayerPairFilterImpl object_vs_object_layer_filter;
	float timeAccumulator = 0.0f;


	// Objects
	JPH::BodyID cube_id;
	JPH::BodyID floor_id;
};


#endif // PHYSICSENGINE_H