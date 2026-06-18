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

class MyJoltDebugRenderer : public JPH::DebugRenderer {
private:
	struct Vertex {
		glm::vec3 position;
		glm::vec4 color;
	};

	std::vector<Vertex> m_LineVertices;
	unsigned int m_VAO = 0, m_VBO = 0;

public:
	JPH_OVERRIDE_NEW_DELETE
	MyJoltDebugRenderer() {
		Initialize(); // Inițializarea Jolt

		// Generăm VAO și VBO dinamic pentru linii
		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);

		glBindVertexArray(m_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

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
		glDeleteVertexArrays(1, &m_VAO);
		glDeleteBuffers(1, &m_VBO);
	}

	// Jolt apelează asta pentru fiecare linie din scenă
	virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
		JPH::Color c = inColor;
		glm::vec4 color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);

		m_LineVertices.push_back({ glm::vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ()), color });
		m_LineVertices.push_back({ glm::vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ()), color });
	}

	// Funcția apelată la finalul cadru-lui pentru a trimite totul la GPU și a randa
	void Render(unsigned int shaderID, const glm::mat4& view, const glm::mat4& projection) {
		if (m_LineVertices.empty())
		{
			return;
		}

		glUseProgram(shaderID);

		// Trimitem doar matricile globale (punctele Jolt sunt deja în World Space)
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, &projection[0][0]);

		glm::mat4 model = glm::mat4(1.0f); // Identity
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, &model[0][0]);

		glBindVertexArray(m_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

		// Încărcăm toate liniile acumulate în acest cadru direct în GPU
		glBufferData(GL_ARRAY_BUFFER, m_LineVertices.size() * sizeof(Vertex), m_LineVertices.data(), GL_DYNAMIC_DRAW);
		// Desenăm liniile
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_LineVertices.size()));

		glBindVertexArray(0);

		// !! CRUCIAL: Golim vectorul pentru cadrul următor, altfel se acumulează la infinit
		m_LineVertices.clear();
	}

	// 1. DrawTriangle
	virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, JPH::DebugRenderer::ECastShadow inCastShadow) override {}

	// 2. CreateTriangleBatch (Varianta cu Vertex brut)
	virtual JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) override {
		return nullptr;
	}

	// 3. CreateTriangleBatch (Varianta cu Triangle structure)
	virtual JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount) override {
		return nullptr;
	}

	// 4. DrawGeometry (Aici era problema principală: GeometryRef în loc de Batch)
	virtual void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const JPH::DebugRenderer::GeometryRef& inGeometry, JPH::DebugRenderer::ECullMode inCullMode, JPH::DebugRenderer::ECastShadow inCastShadow, JPH::DebugRenderer::EDrawMode inDrawMode) override {}

	// 5. DrawText3D
	virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight = 0.5f) override {}
};

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
		
		JPH::BoxShapeSettings box_settings(JPH::Vec3(0.5f, 0.5f, 0.5f));
		JPH::ShapeSettings::ShapeResult box_shape_result = box_settings.Create();
		JPH::Ref<JPH::Shape> box_shape = box_shape_result.Get();

		JPH::RVec3 position(0.0f, 2.0f, 0.0f); // Îl punem la înălțimea de 10 unități
		//glm::quat quater(glm::vec3(glm::radians(30.0f), glm::radians(45.0f), glm::radians(0.0f)));
		//JPH::Quat rotation(quater.x, quater.y, quater.z, quater.w);
		JPH::Quat rotation = JPH::Quat::sIdentity();

		JPH::BodyCreationSettings cube_settings(
			box_shape,
			position,
			rotation,
			JPH::EMotionType::Dynamic,
			Layers::MOVING
		);

		cube_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		cube_settings.mMassPropertiesOverride.mMass = 1.0f;

		cube_id = body_interface.CreateBody(cube_settings)->GetID();

		JPH::BoxShapeSettings floor_settings(JPH::Vec3(50.0f, 0.5f, 50.0f));
		JPH::BodyCreationSettings floor_cube(floor_settings.Create().Get(), JPH::RVec3(0.0f, -0.5f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
		floor_id = body_interface.CreateBody(floor_cube)->GetID();

		body_interface.AddBody(cube_id, JPH::EActivation::Activate);
		body_interface.AddBody(floor_id, JPH::EActivation::DontActivate);

		physics_system.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

		jolt_debug_renderer = std::make_unique<MyJoltDebugRenderer>();
		JPH::DebugRenderer::sInstance = jolt_debug_renderer.get();
	}

	void bindTimeAccumulator(float _timeAccumulator)
	{
		timeAccumulator = _timeAccumulator;
	}

	void getModelMatrix(JPH::BodyID bodyID, glm::vec3 &pos, glm::quat &quat)
	{
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		JPH::RVec3 position;
		JPH::Quat rotation;
		body_interface.GetPositionAndRotation(bodyID, position, rotation);

		pos = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
		quat = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
	}

	void getModelMatrixCube(glm::vec3 &pos, glm::quat &quat)
	{
		getModelMatrix(cube_id, pos, quat);
	}

	void run()
	{
		physics_system.Update(physicsTimeStep, 1, temp_allocator.get(), job_system.get());
	}

	void ApplyRocketForce(const glm::vec3& localOffset, const glm::vec3& localForceDirection, float forceMagnitude) {
		JPH::RVec3 currentPos;
		JPH::Quat currentRot;
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		body_interface.GetPositionAndRotation(cube_id, currentPos, currentRot);

		glm::vec3 glmPos(currentPos.GetX(), currentPos.GetY(), currentPos.GetZ());
		glm::quat glmRot(currentRot.GetW(), currentRot.GetX(), currentRot.GetY(), currentRot.GetZ());

		glm::vec3 worldForcePoint = glmPos + (glmRot * localOffset); // Offset from the main object applied for rotation, snipped to corner

		glm::vec3 worldForceVector = (glmRot * localForceDirection) * forceMagnitude; // Direction of the power related to offset + how much power

		JPH::RVec3 joltWorldPoint(worldForcePoint.x, worldForcePoint.y, worldForcePoint.z);
		JPH::Vec3 joltWorldForce(worldForceVector.x, worldForceVector.y, worldForceVector.z);

		body_interface.AddForce(cube_id, joltWorldForce, joltWorldPoint);
	}

	void ApplyRocketForce()
	{
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

		body_interface.ActivateBody(cube_id);

		JPH::Vec3 joltForce(0.0f, 10.0f, 0.0f);
		JPH::Vec3 joltLoc(0.5f, 0.5f, 0.5f);

		body_interface.AddForce(cube_id, joltForce, joltLoc);

		JPH::RVec3 currentPos;
		JPH::Quat currentRot;
		body_interface.GetPositionAndRotation(cube_id, currentPos, currentRot);
		std::cout << currentPos.GetX() << " " << currentPos.GetY() << " " << currentPos.GetZ() << "\n";
	}

	void drawDebug(unsigned int shaderID, const glm::mat4& view, const glm::mat4& projection) 
	{
		JPH::BodyManager::DrawSettings settings;
		settings.mDrawShape = true;
		settings.mDrawShapeWireframe = true;
		settings.mDrawBoundingBox = true; // Va desena linii pentru Box-ul corpului!

		// 1. Îi cerem lui Jolt să populeze vectorul m_LineVertices prin functia DrawLine
		physics_system.DrawBodies(settings, jolt_debug_renderer.get());

		// 2. Randăm bufferul acumulat pe ecran
		jolt_debug_renderer->Render(shaderID, view, projection);
	}


private:
	PhysicsEngine(){
		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();
	}
	
	const float physicsTimeStep = 1.0f / 60.0f;
	
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