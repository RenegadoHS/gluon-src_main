// Copyright Valve Corporation, All rights reserved.
// Bullet integration by Triang3l, derivative work, in public domain if detached from Valve's work.

#include "physics_object.h"
#include "physics_collide.h"
#include "physics_environment.h"
#include "physics_friction.h"
#include "physics_material.h"
#include "physics_motioncontroller.h"
#include "physics_shadow.h"
#include "physics_vehicle.h"
#include "bspflags.h"
#include "tier0/dbg.h"

CPhysicsObject::CPhysicsObject(IPhysicsEnvironment *environment,
		const CPhysCollide *collide, int materialIndex,
		const Vector &position, const QAngle &angles,
		const objectparams_t *params, bool isStatic) :
		m_Environment(environment),
		m_CollideObjectNext(this), m_CollideObjectPrevious(this),
		m_MassCenterOverride(0.0f, 0.0f, 0.0f),
		m_Mass((!isStatic && !collide->GetShape()->isNonMoving()) ? params->mass : 0.0f),
		m_HingeHLAxis(-1),
		m_MotionEnabled(true),
		m_ShadowTempGravityDisable(false),
		m_LinearDamping(params->damping), m_AngularDamping(params->rotdamping),
		m_MaterialIndex(materialIndex), m_RealMaterialIndex(-1),
		m_ContentsMask(CONTENTS_SOLID),
		m_Shadow(nullptr), m_Player(nullptr),
		m_BodyOfVehicle(nullptr), m_WheelOfVehicle(nullptr),
		m_CollisionEnabled(params->enableCollisions),
		m_ConstraintObjectCount(0), m_ValidConstraintCount(0),
		m_GameData(params->pGameData), m_GameFlags(0), m_GameIndex(0),
		m_Callbacks(CALLBACK_GLOBAL_COLLISION | CALLBACK_GLOBAL_FRICTION |
				CALLBACK_FLUID_TOUCH | CALLBACK_GLOBAL_TOUCH |
				CALLBACK_GLOBAL_COLLIDE_STATIC | CALLBACK_DO_FLUID_SIMULATION),
		m_WasAsleep(true),
		m_LinearVelocityChange(0.0f, 0.0f, 0.0f),
		m_LocalAngularVelocityChange(0.0f, 0.0f, 0.0f),
		m_TouchingTriggers(0),
		m_InterPSILinearVelocity(0.0f, 0.0f, 0.0f),
		m_InterPSIAngularVelocity(0.0f, 0.0f, 0.0f) {
	if (params->pName != nullptr) {
		V_strncpy(m_Name, params->pName, sizeof(m_Name));
	} else {
		m_Name[0] = '\0';
	}

	btCollisionShape *shape = const_cast<CPhysCollide *>(collide)->GetShape();

	btRigidBody::btRigidBodyConstructionInfo constructionInfo(m_Mass, nullptr, shape,
			(m_Mass > 0.0f) ? collide->GetInertia() : btVector3(0.0f, 0.0f, 0.0f));

	btVector3 massCenter = collide->GetMassCenter();
	const Vector *massCenterOverride = params->massCenterOverride;
	if (massCenterOverride != nullptr) {
		ConvertPositionToBullet(*massCenterOverride, m_MassCenterOverride);
		btCompoundShape *massCenterOverrideShape = VPhysicsNew(btCompoundShape, false, 1);
		btVector3 massCenterOffset = m_MassCenterOverride - massCenter;
		massCenterOverrideShape->addChildShape(btTransform(btMatrix3x3::getIdentity(),
				-massCenterOffset), shape);
		constructionInfo.m_collisionShape = massCenterOverrideShape;
		massCenter = m_MassCenterOverride;
		if (m_Mass > 0.0f) {
			constructionInfo.m_localInertia = CPhysicsCollision::OffsetInertia(
					constructionInfo.m_localInertia, massCenterOffset);
		}
	}
	if (m_Mass > 0.0f) {
		constructionInfo.m_localInertia *= params->inertia * m_Mass;
		if (params->rotInertiaLimit > 0.0f) {
			btScalar minInertia = constructionInfo.m_localInertia.length() * params->rotInertiaLimit;
			constructionInfo.m_localInertia.setMax(btVector3(minInertia, minInertia, minInertia));
		}
	}
	ConvertInertiaToHL(constructionInfo.m_localInertia, m_Inertia);

	matrix3x4_t startMatrix;
	AngleMatrix(angles, position, startMatrix);
	btTransform &startWorldTransform = constructionInfo.m_startWorldTransform;
	ConvertMatrixToBullet(startMatrix, startWorldTransform);
	startWorldTransform.getOrigin() += startWorldTransform.getBasis() * massCenter;
	m_InterPSIWorldTransform = startWorldTransform;

	m_RigidBody = VPhysicsNew(btRigidBody, constructionInfo);
	m_RigidBody->setUserPointer(this);

	m_RigidBody->setSleepingThresholds(0.2f, 0.4f); // 0.1 and 0.2 in IVP, but that's too low.

	if (!IsStatic()) {
		m_GravityEnabled = true;
		m_LinearDragCoefficient = m_AngularDragCoefficient = params->dragCoefficient;
		ComputeDragBases();
		m_DragEnabled = (m_LinearDragCoefficient != 0.0f);
	} else {
		const btVector3 zero(0.0f, 0.0f, 0.0f);
		m_GravityEnabled = false;
		m_LinearDragCoefficient = m_AngularDragCoefficient = 0.0f;
		m_LinearDragBasis.setZero();
		m_AngularDragBasis.setZero();
		m_DragEnabled = false;
	}

	AddReferenceToCollide();

	UpdateMaterial();

	Sleep();
}

CPhysicsObject::~CPhysicsObject() {
	btCollisionShape *shape = m_RigidBody->getCollisionShape();
	if (shape->getUserPointer() == nullptr) {
		// Delete the mass override shape.
		m_RigidBody->setCollisionShape(static_cast<btCompoundShape *>(shape)->getChildShape(0));
		VPhysicsDelete(btCompoundShape, shape);
	}

	RemoveReferenceFromCollide();
	VPhysicsDelete(btRigidBody, m_RigidBody);
}

void CPhysicsObject::NotifyQueuedForRemoval() {
	if (m_BodyOfVehicle != nullptr) {
		static_cast<CPhysicsVehicleController *>(m_BodyOfVehicle)->SetBodyObject(nullptr);
		Assert(m_BodyOfVehicle == nullptr);
	}
}

void CPhysicsObject::Release() {
	// Prevent callbacks to the game code and unlink from this object.
	m_Callbacks = 0;
	m_GameData = nullptr;

	RemovePlayerController();
	RemoveShadowController();
	DetachFromMotionControllers();
	// Really, REALLY bad if still a vehicle - constraints queued for removal won't be removed correctly.
	Assert(m_BodyOfVehicle == nullptr);

	static_cast<CPhysicsEnvironment *>(m_Environment)->NotifyObjectRemoving(this);

	VPhysicsDelete(CPhysicsObject, this);
}

CPhysCollide *CPhysicsObject::GetCollide() {
	btCollisionShape *shape = m_RigidBody->getCollisionShape();
	if (shape->getUserPointer() == nullptr) {
		// Overriding mass center.
		shape = static_cast<btCompoundShape *>(shape)->getChildShape(0);
	}
	return reinterpret_cast<CPhysCollide *>(shape->getUserPointer());
}

const CPhysCollide *CPhysicsObject::GetCollide() const {
	const btCollisionShape *shape = m_RigidBody->getCollisionShape();
	if (shape->getUserPointer() == nullptr) {
		// Overriding mass center.
		shape = static_cast<const btCompoundShape *>(shape)->getChildShape(0);
	}
	return reinterpret_cast<const CPhysCollide *>(shape->getUserPointer());
}

float CPhysicsObject::GetSphereRadius() const {
	const CPhysCollide *collide = GetCollide();
	if (CPhysCollide_Sphere::IsSphere(collide)) {
		return BULLET2HL(static_cast<const CPhysCollide_Sphere *>(collide)->GetRadius());
	}
	return 0.0f;
}

void CPhysicsObject::NotifyTransferred(IPhysicsEnvironment *newEnvironment) {
	m_Environment = newEnvironment;
	Assert(!IsTouchingTriggers());
}

/*******************
 * Mass and inertia
 *******************/

bool CPhysicsObject::IsStatic() const {
	return m_RigidBody->isStaticObject();
}

void CPhysicsObject::UpdateMassProps() {
	// GetMass and GetInertia handle the overrides (shadows, hinge).
	btVector3 bulletInertia;
	ConvertInertiaToBullet(GetInertia(), bulletInertia);
	m_RigidBody->setMassProps(GetMass(), bulletInertia);
	m_RigidBody->updateInertiaTensor();
}

void CPhysicsObject::SetMass(float mass) {
	Assert(mass > 0.0f);
	if (IsStatic()) {
		return;
	}
	m_Inertia *= mass / m_Mass;
	m_Mass = mass;
	UpdateMassProps();
}

float CPhysicsObject::GetMass() const {
	// Must handle all the overrides here because UpdateMassProps calls this.
	if (!m_MotionEnabled || (m_Shadow != nullptr && !m_Shadow->AllowsTranslation())) {
		return BT_LARGE_FLOAT;
	}
	return m_Mass;
}

float CPhysicsObject::GetInvMass() const {
	return m_RigidBody->getInvMass();
}

Vector CPhysicsObject::GetInertia() const {
	// Must handle all the overrides here because UpdateMassProps calls this.
	if (!m_MotionEnabled || (m_Shadow != nullptr && !m_Shadow->AllowsRotation())) {
		return Vector(BT_LARGE_FLOAT, BT_LARGE_FLOAT, BT_LARGE_FLOAT);
	}
	Vector inertia = m_Inertia;
	if (m_HingeHLAxis >= 0) {
		inertia[m_HingeHLAxis] = BT_LARGE_FLOAT;
	}
	return m_Inertia;
}

Vector CPhysicsObject::GetInvInertia() const {
	Vector inertia;
	ConvertInertiaToHL(m_RigidBody->getInvInertiaDiagLocal(), inertia);
	return inertia;
}

void CPhysicsObject::SetInertia(const Vector &inertia) {
	if (!IsStatic()) {
		return;
	}
	m_Inertia = inertia;
	UpdateMassProps();
}

bool CPhysicsObject::IsHinged() const {
	return m_HingeHLAxis >= 0;
}

void CPhysicsObject::BecomeHinged(int localAxis) {
	Assert(localAxis >= 0 && localAxis <= 2);
	if (IsStatic()) {
		return;
	}
	if (localAxis < 0 || localAxis > 2) {
		RemoveHinged();
		return;
	}
	m_HingeHLAxis = localAxis;
	UpdateMassProps();
}

void CPhysicsObject::RemoveHinged() {
	if (!IsHinged()) {
		return;
	}
	m_HingeHLAxis = -1;
	UpdateMassProps();
}

bool CPhysicsObject::IsMotionEnabled() const {
	return m_MotionEnabled;
}

bool CPhysicsObject::IsMoveable() const {
	return !IsStatic() && IsMotionEnabled();
}

void CPhysicsObject::EnableMotion(bool enable) {
	if (IsMotionEnabled() == enable) {
		return;
	}

	m_MotionEnabled = enable;

	// IVP clears speed when both pinning and unpinning.
	btVector3 zero(0.0f, 0.0f, 0.0f);
	m_RigidBody->setLinearVelocity(zero);
	m_RigidBody->setAngularVelocity(zero);
	m_LinearVelocityChange.setZero();
	m_LocalAngularVelocityChange.setZero();
	// Freeze in place if called externally, don't wait for the next PSI.
	m_InterPSILinearVelocity.setZero();
	m_InterPSIAngularVelocity.setZero();

	UpdateMassProps();
}

/*******************
 * Activation state
 *******************/

bool CPhysicsObject::IsAsleep() const {
	return !m_RigidBody->isActive();
}

void CPhysicsObject::Wake() {
	if (!IsStatic() && m_RigidBody->getActivationState() != DISABLE_DEACTIVATION) {
		// Forcing because it may be used for external forces without contacts.
		// Also waking up from DISABLE_SIMULATION, which is not possible with setActivationState.
		m_RigidBody->forceActivationState(ACTIVE_TAG);
		m_RigidBody->setDeactivationTime(0.0f);
	}
}

void CPhysicsObject::Sleep() {
	if (!IsStatic() && m_RigidBody->getActivationState() != DISABLE_DEACTIVATION) {
		m_RigidBody->setActivationState(DISABLE_SIMULATION);
	}
}

/**********************
 * Gravity and damping
 **********************/

bool CPhysicsObject::IsGravityEnabled() const {
	if (m_ShadowTempGravityDisable ||
			(m_Shadow != nullptr && !m_Shadow->AllowsTranslation()) || IsTrigger()) {
		return false;
	}
	return m_GravityEnabled;
}

void CPhysicsObject::EnableGravity(bool enable) {
	if (IsStatic()) {
		return;
	}
	// IsGravityEnabled comparison shouldn't be done because of shadow and trigger overrides.
	m_GravityEnabled = enable;
}

void CPhysicsObject::SetDamping(const float *speed, const float *rot) {
	if (speed != nullptr) {
		m_LinearDamping = *speed;
	}
	if (rot != nullptr) {
		m_AngularDamping = *rot;
	}
}

void CPhysicsObject::GetDamping(float *speed, float *rot) const {
	if (speed != nullptr) {
		*speed = m_LinearDamping;
	}
	if (rot != nullptr) {
		*rot = m_AngularDamping;
	}
}

void CPhysicsObject::ApplyDamping(btScalar timeStep) {
	if (!IsMoveable()) {
		return;
	}

	if (m_Shadow != nullptr || m_Player != nullptr) {
		// Only single-tick forces for shadows.
		m_RigidBody->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
	}

	if (IsAsleep() || !IsGravityEnabled()) {
		return;
	}

	const btVector3 &linearVelocity = m_RigidBody->getLinearVelocity();
	btScalar linearDamping = m_LinearDamping * timeStep;
	btScalar linearSleepingThreshold = m_RigidBody->getLinearSleepingThreshold();
	if (linearVelocity.length2() < linearSleepingThreshold * linearSleepingThreshold) {
		linearDamping += 0.1f;
	}
	if (linearDamping < 0.25f) {
		linearDamping = btScalar(1.0f) - linearDamping;
	} else {
		linearDamping = btExp(-linearDamping);
	}
	m_RigidBody->setLinearVelocity(linearVelocity * linearDamping);

	if (m_Shadow == nullptr) {
		const btVector3 &angularVelocity = m_RigidBody->getAngularVelocity();
		btScalar angularDamping = m_AngularDamping * timeStep;
		btScalar angularSleepingThreshold = m_RigidBody->getAngularSleepingThreshold();
		if (angularVelocity.length2() < angularSleepingThreshold * angularSleepingThreshold) {
			angularDamping += 0.1f;
		}
		if (angularDamping < 0.4f) {
			angularDamping = btScalar(1.0f) - angularDamping;
		} else {
			angularDamping = btExp(-angularDamping);
		}
		m_RigidBody->setAngularVelocity(angularVelocity * angularDamping);
	}
}

void CPhysicsObject::ApplyGravity(btScalar timeStep) {
	if (!IsMoveable() || IsAsleep() || !IsGravityEnabled()) {
		return;
	}
	btVector3 gravity = static_cast<const CPhysicsEnvironment *>(m_Environment)->GetBulletGravity();
	if (m_BodyOfVehicle != nullptr) {
		static_cast<CPhysicsVehicleController *>(m_BodyOfVehicle)->ModifyGravity(gravity);
	}
	m_RigidBody->setLinearVelocity(m_RigidBody->getLinearVelocity() + gravity * timeStep);
}

/*******
 * Drag
 *******/

btScalar CPhysicsObject::AngularDragIntegral(btScalar l, btScalar w, btScalar h) {
	btScalar w2 = w * w, l2 = l * l, h2 = h * h;
	return (1.0f / 3.0f) * w2 * l * l2 + 0.5f * w2 * w2 * l + l * w2 * h2;
}

void CPhysicsObject::ComputeDragBases() {
	const CPhysCollide *collide = GetCollide();
	btVector3 aabbMin, aabbMax;
	collide->GetShape()->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
	btVector3 extents = aabbMax - aabbMin;
	const btVector3 &areas = collide->GetOrthographicAreas();
	m_LinearDragBasis.setValue(
			extents.getY() * extents.getZ(),
			extents.getX() * extents.getZ(),
			extents.getX() * extents.getY());
	m_LinearDragBasis *= areas;
	extents *= 0.5f;
	m_AngularDragBasis.setValue(
			AngularDragIntegral(extents.getX(), extents.getY(), extents.getZ()) +
					AngularDragIntegral(extents.getX(), extents.getZ(), extents.getY()),
			AngularDragIntegral(extents.getY(), extents.getX(), extents.getZ()) +
					AngularDragIntegral(extents.getY(), extents.getZ(), extents.getX()),
			AngularDragIntegral(extents.getZ(), extents.getX(), extents.getY()) +
					AngularDragIntegral(extents.getZ(), extents.getY(), extents.getX()));
	m_AngularDragBasis *= areas;
}

bool CPhysicsObject::IsDragEnabled() const {
	if (m_Shadow != nullptr || IsTrigger()) {
		return false;
	}
	return m_DragEnabled;
}

void CPhysicsObject::EnableDrag(bool enable) {
	if (IsStatic()) {
		return;
	}
	// IsDragEnabled comparison shouldn't be done because of shadow and trigger overrides.
	m_DragEnabled = enable;
}

void CPhysicsObject::SetDragCoefficient(float *pDrag, float *pAngularDrag) {
	if (pDrag != nullptr) {
		m_LinearDragCoefficient = *pDrag;
	}
	if (pAngularDrag != nullptr) {
		m_AngularDragCoefficient = *pAngularDrag;
	}
}

btScalar CPhysicsObject::CalculateLinearDrag(const btVector3 &velocity) const {
	btVector3 drag = ((velocity * m_RigidBody->getWorldTransform().getBasis()) * m_LinearDragBasis).absolute();
	return m_LinearDragCoefficient * m_RigidBody->getInvMass() * (drag.getX() + drag.getY() + drag.getZ());
}

float CPhysicsObject::CalculateLinearDrag(const Vector &unitDirection) const {
	btVector3 bulletUnitDirection;
	ConvertDirectionToBullet(unitDirection, bulletUnitDirection);
	return CalculateLinearDrag(bulletUnitDirection);
}

btScalar CPhysicsObject::CalculateAngularDrag(const btVector3 &objectSpaceRotationAxis) const {
	btVector3 drag = (objectSpaceRotationAxis * m_AngularDragBasis *
			m_RigidBody->getInvInertiaDiagLocal()).absolute();
	return m_AngularDragCoefficient * (drag.getX() + drag.getY() + drag.getZ());
}

float CPhysicsObject::CalculateAngularDrag(const Vector &objectSpaceRotationAxis) const {
	btVector3 bulletAxis;
	ConvertDirectionToBullet(objectSpaceRotationAxis, bulletAxis);
	return DEG2RAD(CalculateAngularDrag(bulletAxis));
}

void CPhysicsObject::ApplyDrag(btScalar timeStep) {
	if (!IsMoveable() || IsAsleep() || !IsDragEnabled()) {
		return;
	}

	btScalar dragForceScale = m_Environment->GetAirDensity() * timeStep;

	const btVector3 &linearVelocity = m_RigidBody->getLinearVelocity();
	btScalar dragForce = -0.5f * CalculateLinearDrag(linearVelocity) * dragForceScale;
	if (dragForce < 0.0f) {
		btSetMax(dragForce, btScalar(-1.0f));
		m_RigidBody->setLinearVelocity(linearVelocity + (linearVelocity * dragForce));
	}

	const btVector3 &angularVelocity = m_RigidBody->getAngularVelocity();
	float angularDragForce = -CalculateAngularDrag(angularVelocity *
			m_RigidBody->getWorldTransform().getBasis()) * dragForceScale;
	if (angularDragForce < 0.0f) {
		btSetMax(angularDragForce, btScalar(-1.0f));
		m_RigidBody->setAngularVelocity(angularVelocity + (angularVelocity * angularDragForce));
	}
}

void CPhysicsObject::NotifyOrthographicAreasChanged() {
	if (!IsStatic()) {
		ComputeDragBases();
	}
}

/************
 * Materials
 ************/

int CPhysicsObject::GetMaterialIndex() const {
	return m_RealMaterialIndex;
}

void CPhysicsObject::SetMaterialIndex(int materialIndex) {
	m_MaterialIndex = materialIndex;
	UpdateMaterial();
}

void CPhysicsObject::UpdateMaterial() {
	int materialIndex = m_MaterialIndex;
	if (m_Shadow != nullptr && static_cast<CPhysicsShadowController *>(m_Shadow)->IsUsingShadowMaterial()) {
		materialIndex = MATERIAL_INDEX_SHADOW;
	} else {
		const CPhysCollide *collide = GetCollide();
		if (CPhysCollide_TriangleMesh::IsTriangleMesh(collide)) {
			int triangleMeshMaterialIndex =
					static_cast<const CPhysCollide_TriangleMesh *>(collide)->GetSurfacePropsIndex();
			if (triangleMeshMaterialIndex != 0) {
				materialIndex = triangleMeshMaterialIndex;
			}
		}
	}
	if (materialIndex != m_RealMaterialIndex) {
		m_RealMaterialIndex = materialIndex;
		float friction, elasticity;
		g_pPhysSurfaceProps->GetPhysicsProperties(materialIndex, nullptr, nullptr, &friction, &elasticity);
		m_RigidBody->setFriction(friction);
		if (friction > 0.0f) {
			// Stability.
			m_RigidBody->setCollisionFlags(m_RigidBody->getCollisionFlags() | btCollisionObject::CF_HAS_FRICTION_ANCHOR);
		} else {
			m_RigidBody->setCollisionFlags(m_RigidBody->getCollisionFlags() & ~btCollisionObject::CF_HAS_FRICTION_ANCHOR);
		}
		m_RigidBody->setRestitution(elasticity);
	}
}

unsigned int CPhysicsObject::GetContents() const {
	return m_ContentsMask;
}

void CPhysicsObject::SetContents(unsigned int contents) {
	m_ContentsMask = contents;
}

/************
 * Game data
 ************/

void CPhysicsObject::SetGameData(void *pGameData) {
	m_GameData = pGameData;
}

void *CPhysicsObject::GetGameData() const {
	return m_GameData;
}

void CPhysicsObject::SetGameFlags(unsigned short userFlags) {
	m_GameFlags = userFlags;
}

unsigned short CPhysicsObject::GetGameFlags() const {
	return m_GameFlags;
}

void CPhysicsObject::SetGameIndex(unsigned short gameIndex) {
	m_GameIndex = gameIndex;
}

unsigned short CPhysicsObject::GetGameIndex() const {
	return m_GameIndex;
}

void CPhysicsObject::SetCallbackFlags(unsigned short callbackflags) {
	m_Callbacks = callbackflags;
}

unsigned short CPhysicsObject::GetCallbackFlags() const {
	return m_Callbacks;
}

const char *CPhysicsObject::GetName() const {
	return m_Name;
}

/**********************
 * Position and forces
 **********************/

const btVector3 &CPhysicsObject::GetBulletMassCenter() const {
	if (m_RigidBody->getCollisionShape()->getUserPointer() == nullptr) {
		return m_MassCenterOverride;
	}
	return GetCollide()->GetMassCenter();
}

Vector CPhysicsObject::GetMassCenterLocalSpace() const {
	Vector massCenter;
	ConvertPositionToHL(GetBulletMassCenter(), massCenter);
	return massCenter;
}

void CPhysicsObject::ProceedToTransform(const btTransform &transform) {
	btTransform oldTransform = m_RigidBody->getWorldTransform();
	m_RigidBody->proceedToTransform(transform);

	if (!IsStatic()) {
		m_RigidBody->setAngularVelocity(transform.getBasis() *
				(m_RigidBody->getAngularVelocity() * oldTransform.getBasis()));

		// TODO: Properly handle interpolation values of the rigid body.

		if (!m_Environment->IsInSimulation()) {
			m_InterPSIAngularVelocity = transform.getBasis() *
					(m_InterPSIAngularVelocity * oldTransform.getBasis());
			InterpolateBetweenPSIs();
		}
	}

	if (m_BodyOfVehicle != nullptr) {
		// TODO: This multiplication is TOTALLY UNTESTED!
		static_cast<CPhysicsVehicleController *>(m_BodyOfVehicle)->ShiftWheelTransforms(
				oldTransform.inverseTimes(transform));
	}
}

void CPhysicsObject::SetPosition(const Vector &worldPosition, const QAngle &angles, bool isTeleport) {
	if (m_Shadow != nullptr) {
		UpdateShadow(worldPosition, angles, false, 0.0f);
	}
	matrix3x4_t matrix;
	AngleMatrix(angles, worldPosition, matrix);
	btTransform transform;
	ConvertMatrixToBullet(matrix, transform);
	transform.getOrigin() += transform.getBasis() * GetBulletMassCenter();
	ProceedToTransform(transform);
}

void CPhysicsObject::SetPositionMatrix(const matrix3x4_t &matrix, bool isTeleport) {
	if (m_Shadow != nullptr) {
		Vector worldPosition;
		MatrixGetColumn(matrix, 3, worldPosition);
		QAngle angles;
		MatrixAngles(matrix, angles);
		UpdateShadow(worldPosition, angles, false, 0.0f);
	}
	btTransform transform;
	ConvertMatrixToBullet(matrix, transform);
	transform.getOrigin() += transform.getBasis() * GetBulletMassCenter();
	ProceedToTransform(transform);
}

void CPhysicsObject::GetPosition(Vector *worldPosition, QAngle *angles) const {
	const btTransform &transform = GetInterPSIWorldTransform();
	const btMatrix3x3 &basis = transform.getBasis();
	if (worldPosition != nullptr) {
		ConvertPositionToHL(transform.getOrigin() - (basis * GetBulletMassCenter()), *worldPosition);
	}
	if (angles != nullptr) {
		ConvertRotationToHL(basis, *angles);
	}
}

void CPhysicsObject::GetPositionAtPSI(Vector *worldPosition, QAngle *angles) const {
	const btTransform &transform = m_RigidBody->getWorldTransform();
	const btMatrix3x3 &basis = transform.getBasis();
	if (worldPosition != nullptr) {
		ConvertPositionToHL(transform.getOrigin() - (basis * GetBulletMassCenter()), *worldPosition);
	}
	if (angles != nullptr) {
		ConvertRotationToHL(basis, *angles);
	}
}

void CPhysicsObject::GetPositionMatrix(matrix3x4_t *positionMatrix) const {
	const btTransform &transform = GetInterPSIWorldTransform();
	const btMatrix3x3 &basis = transform.getBasis();
	btVector3 origin = transform.getOrigin() - (basis * GetBulletMassCenter());
	ConvertMatrixToHL(basis, origin, *positionMatrix);
}

void CPhysicsObject::LocalToWorld(Vector *worldPosition, const Vector &localPosition) const {
	matrix3x4_t matrix;
	GetPositionMatrix(&matrix);
	// Copy in case src == dest.
	VectorTransform(Vector(localPosition), matrix, *worldPosition);
}

void CPhysicsObject::WorldToLocal(Vector *localPosition, const Vector &worldPosition) const {
	matrix3x4_t matrix;
	GetPositionMatrix(&matrix);
	// Copy in case src == dest.
	VectorITransform(Vector(worldPosition), matrix, *localPosition);
}

void CPhysicsObject::LocalToWorldVector(Vector *worldVector, const Vector &localVector) const {
	matrix3x4_t matrix;
	GetPositionMatrix(&matrix);
	// Copy in case src == dest.
	VectorRotate(Vector(localVector), matrix, *worldVector);
}

void CPhysicsObject::WorldToLocalVector(Vector *localVector, const Vector &worldVector) const {
	matrix3x4_t matrix;
	GetPositionMatrix(&matrix);
	// Copy in case src == dest.
	VectorIRotate(Vector(worldVector), matrix, *localVector);
}

void CPhysicsObject::UpdateAfterPSI() {
	m_InterPSIWorldTransform = m_RigidBody->getWorldTransform();
	m_InterPSILinearVelocity = m_RigidBody->getLinearVelocity();
	m_InterPSIAngularVelocity = m_RigidBody->getAngularVelocity();
}

void CPhysicsObject::InterpolateBetweenPSIs() {
	// For non-moving objects, the transform was already updated at the end of the PSI.
	if (!m_InterPSILinearVelocity.isZero() || !m_InterPSIAngularVelocity.isZero()) {
		btTransformUtil::integrateTransform(m_RigidBody->getWorldTransform(),
				m_InterPSILinearVelocity, m_InterPSIAngularVelocity,
				static_cast<const CPhysicsEnvironment *>(m_Environment)->GetTimeSinceLastPSI(),
				m_InterPSIWorldTransform);
	}
}

void CPhysicsObject::ApplyForcesAndSpeedLimit(btScalar timeStep) {
	Assert(!IsStatic());
	// Still need to clamp things like shadow impulses even if motion is disabled, so it doesn't affect this.
	if (!IsAsleep()) {
		const CPhysicsEnvironment *environment = static_cast<const CPhysicsEnvironment *>(m_Environment);
		btVector3 linearVelocity = m_RigidBody->getLinearVelocity() + m_LinearVelocityChange;
		btScalar maxSpeed = environment->GetMaxSpeed();
		btClamp(linearVelocity[0], -maxSpeed, maxSpeed);
		btClamp(linearVelocity[1], -maxSpeed, maxSpeed);
		btClamp(linearVelocity[2], -maxSpeed, maxSpeed);
		m_RigidBody->setLinearVelocity(linearVelocity);

		const btMatrix3x3 &worldTransformBasis = m_RigidBody->getWorldTransform().getBasis();
		btVector3 localAngularVelocity = (m_RigidBody->getAngularVelocity() * worldTransformBasis) +
				m_LocalAngularVelocityChange;
		btScalar maxAngularSpeed = environment->GetMaxAngularSpeed();
		btClamp(localAngularVelocity[0], -maxAngularSpeed, maxAngularSpeed);
		btClamp(localAngularVelocity[1], -maxAngularSpeed, maxAngularSpeed);
		btClamp(localAngularVelocity[2], -maxAngularSpeed, maxAngularSpeed);
		m_RigidBody->setAngularVelocity(worldTransformBasis * localAngularVelocity);
	}
	m_LinearVelocityChange.setZero();
	m_LocalAngularVelocityChange.setZero();
}

void CPhysicsObject::CheckAndClearBulletForces() {
	Assert(m_RigidBody->getTotalForce().isZero() && m_RigidBody->getTotalTorque().isZero());
	m_RigidBody->clearForces();
}

void CPhysicsObject::SetVelocity(const Vector *velocity, const AngularImpulse *angularVelocity) {
	if (!IsMoveable()) {
		return;
	}
	btVector3 zero(0.0f, 0.0f, 0.0f);
	bool wake = false;
	if (velocity != nullptr) {
		ConvertPositionToBullet(*velocity, m_LinearVelocityChange);
		m_RigidBody->setLinearVelocity(zero);
		wake = (wake || !m_LinearVelocityChange.isZero());
	}
	if (angularVelocity != nullptr) {
		ConvertAngularImpulseToBullet(*angularVelocity, m_LocalAngularVelocityChange);
		m_RigidBody->setAngularVelocity(zero);
		wake = (wake || !m_LocalAngularVelocityChange.isZero());
	}
	if (wake) {
		Wake();
	}
}

void CPhysicsObject::SetVelocityInstantaneous(const Vector *velocity, const AngularImpulse *angularVelocity) {
	if (!IsMoveable()) {
		return;
	}
	const CPhysicsEnvironment *environment = static_cast<const CPhysicsEnvironment *>(m_Environment);
	bool wake = false;
	if (velocity != nullptr) {
		btVector3 bulletVelocity;
		ConvertPositionToBullet(*velocity, bulletVelocity);
		btScalar maxSpeed = environment->GetMaxSpeed();
		btClamp(bulletVelocity[0], -maxSpeed, maxSpeed);
		btClamp(bulletVelocity[1], -maxSpeed, maxSpeed);
		btClamp(bulletVelocity[2], -maxSpeed, maxSpeed);
		m_RigidBody->setLinearVelocity(bulletVelocity);
		m_LinearVelocityChange.setZero();
		m_InterPSILinearVelocity = bulletVelocity;
		wake = (wake || !bulletVelocity.isZero());
	}
	if (angularVelocity != nullptr) {
		btVector3 bulletAngularVelocity;
		ConvertAngularImpulseToBullet(*angularVelocity, bulletAngularVelocity);
		btScalar maxAngularSpeed = environment->GetMaxAngularSpeed();
		btClamp(bulletAngularVelocity[0], -maxAngularSpeed, maxAngularSpeed);
		btClamp(bulletAngularVelocity[1], -maxAngularSpeed, maxAngularSpeed);
		btClamp(bulletAngularVelocity[2], -maxAngularSpeed, maxAngularSpeed);
		bulletAngularVelocity = m_RigidBody->getWorldTransform().getBasis() * bulletAngularVelocity;
		m_RigidBody->setAngularVelocity(bulletAngularVelocity);
		m_LocalAngularVelocityChange.setZero();
		m_InterPSIAngularVelocity = bulletAngularVelocity;
		wake = (wake || !bulletAngularVelocity.isZero());
	}
	if (wake) {
		Wake();
	}
}

void CPhysicsObject::GetVelocity(Vector *velocity, AngularImpulse *angularVelocity) const {
	if (velocity != nullptr) {
		ConvertPositionToHL(m_RigidBody->getLinearVelocity() + m_LinearVelocityChange, *velocity);
	}
	if (angularVelocity != nullptr) {
		AngularImpulse worldAngularVelocity;
		ConvertAngularImpulseToHL((m_RigidBody->getAngularVelocity() * 
				m_RigidBody->getWorldTransform().getBasis()) +
				m_LocalAngularVelocityChange, *angularVelocity);
	}
}

void CPhysicsObject::GetVelocityAtPoint(const Vector &worldPosition, Vector *pVelocity) const {
	const btTransform &worldTransform = m_RigidBody->getWorldTransform();
	btVector3 angularVelocity = m_RigidBody->getAngularVelocity() +
			(worldTransform.getBasis() * m_LocalAngularVelocityChange);
	// Position is relative to the center of mass (the rigid body's position).
	btVector3 bulletWorldPosition;
	ConvertPositionToBullet(worldPosition, bulletWorldPosition);
	btVector3 linearVelocity = m_RigidBody->getLinearVelocity() + angularVelocity.cross(
			bulletWorldPosition - worldTransform.getOrigin()) + m_LinearVelocityChange;
	ConvertPositionToHL(linearVelocity, *pVelocity);
}

void CPhysicsObject::AddVelocity(const Vector *velocity, const AngularImpulse *angularVelocity) {
	if (!IsMoveable()) {
		return;
	}
	if (velocity != nullptr) {
		btVector3 bulletVelocity;
		ConvertPositionToBullet(*velocity, bulletVelocity);
		m_LinearVelocityChange += bulletVelocity;
	}
	if (angularVelocity != nullptr) {
		btVector3 bulletAngularVelocity;
		ConvertAngularImpulseToBullet(*angularVelocity, bulletAngularVelocity);
		m_LocalAngularVelocityChange += bulletAngularVelocity;
	}
	Wake();
}

float CPhysicsObject::GetEnergy() const {
	btVector3 angularVelocity = m_RigidBody->getAngularVelocity() *
			m_RigidBody->getWorldTransform().getBasis();
	btVector3 inertia;
	ConvertInertiaToBullet(GetInertia(), inertia);
	// 1/2mv^2 + 1/2Iw^2
	return ConvertEnergyToHL(0.5f * (
			btScalar(GetMass()) * m_RigidBody->getLinearVelocity().length2() +
			(angularVelocity * inertia).dot(angularVelocity)));
}

void CPhysicsObject::ApplyForceCenter(const Vector &forceVector) {
	if (!IsMoveable()) {
		return;
	}
	btVector3 bulletForce;
	ConvertForceImpulseToBullet(forceVector, bulletForce);
	m_LinearVelocityChange += bulletForce * m_RigidBody->getInvMass();
	Wake();
}

void CPhysicsObject::ApplyForceOffset(const Vector &forceVector, const Vector &worldPosition) {
	if (!IsMoveable()) {
		return;
	}
	btVector3 bulletWorldForce;
	ConvertForceImpulseToBullet(forceVector, bulletWorldForce);
	m_LinearVelocityChange += bulletWorldForce * m_RigidBody->getInvMass();
	btVector3 bulletWorldPosition;
	ConvertPositionToBullet(worldPosition, bulletWorldPosition);
	const btTransform &worldTransform = m_RigidBody->getWorldTransform();
	m_LocalAngularVelocityChange += ((bulletWorldPosition - worldTransform.getOrigin()).cross(
			bulletWorldForce) * worldTransform.getBasis()) * m_RigidBody->getInvInertiaDiagLocal();
	Wake();
}

void CPhysicsObject::ApplyTorqueCenter(const AngularImpulse &torque) {
	if (!IsMoveable()) {
		return;
	}
	btVector3 bulletWorldTorque;
	ConvertAngularImpulseToBullet(torque, bulletWorldTorque);
	m_LocalAngularVelocityChange += (bulletWorldTorque *
			m_RigidBody->getWorldTransform().getBasis()) * m_RigidBody->getInvInertiaDiagLocal();
	Wake();
}

void CPhysicsObject::CalculateForceOffset(const Vector &forceVector, const Vector &worldPosition,
		Vector *centerForce, AngularImpulse *centerTorque) const {
	if (centerForce != nullptr) {
		*centerForce = forceVector;
	}
	if (centerTorque != nullptr) {
		btVector3 bulletWorldForce, bulletWorldPosition;
		ConvertPositionToBullet(forceVector, bulletWorldForce);
		ConvertPositionToBullet(worldPosition, bulletWorldPosition);
		// Center torque is in mass center-relative space (for motion controllers, not ApplyTorqueCenter).
		const btTransform &worldTransform = m_RigidBody->getWorldTransform();
		btVector3 bulletCenterTorque = (bulletWorldPosition - worldTransform.getOrigin()).cross(
				bulletWorldForce) * worldTransform.getBasis();
		ConvertAngularImpulseToHL(bulletCenterTorque, *centerTorque);
	}
}

void CPhysicsObject::CalculateVelocityOffset(const Vector &forceVector, const Vector &worldPosition,
		Vector *centerVelocity, AngularImpulse *centerAngularVelocity) const {
	if (centerVelocity != nullptr) {
		*centerVelocity = forceVector * (float) m_RigidBody->getInvMass();
	}
	if (centerAngularVelocity != nullptr) {
		btVector3 bulletWorldForce, bulletWorldPosition;
		ConvertPositionToBullet(forceVector, bulletWorldForce);
		ConvertPositionToBullet(worldPosition, bulletWorldPosition);
		// Center angular velocity is in mass center-relative space.
		const btTransform &worldTransform = m_RigidBody->getWorldTransform();
		btVector3 bulletCenterAngularVelocity = ((bulletWorldPosition - worldTransform.getOrigin()).cross(
				bulletWorldForce) * worldTransform.getBasis()) * m_RigidBody->getInvInertiaDiagLocal();
		ConvertAngularImpulseToHL(bulletCenterAngularVelocity, *centerAngularVelocity);
	}
}

void CPhysicsObject::NotifyAttachedToMotionController(IPhysicsMotionController *controller) {
	m_MotionControllers.AddToTail(controller);
}

void CPhysicsObject::NotifyDetachedFromMotionController(IPhysicsMotionController *controller) {
	// Not using FastRemove not to change order in case a controller reads the current velocity.
	m_MotionControllers.FindAndRemove(controller);
}

void CPhysicsObject::DetachFromMotionControllers() {
	int controllerCount = m_MotionControllers.Count();
	for (int controllerIndex = 0; controllerIndex < controllerCount; ++controllerIndex) {
		static_cast<CPhysicsMotionController *>(
				m_MotionControllers[controllerIndex])->DetachObjectInternal(this, false);
	}
	m_MotionControllers.RemoveAll();
}

void CPhysicsObject::SimulateMotionControllers(
		IPhysicsMotionController::priority_t priority, btScalar timeStep) {
	// Simulating even if not moveable because the controller may toggle moveability.
	int controllerCount = m_MotionControllers.Count();
	for (int controllerIndex = 0; controllerIndex < controllerCount; ++controllerIndex) {
		CPhysicsMotionController *controller = static_cast<CPhysicsMotionController *>(
				m_MotionControllers[controllerIndex]);
		if (controller->GetPriority() == priority) {
			controller->Simulate(this, timeStep);
		}
	}
}

void CPhysicsObject::ApplyEventMotion(bool isWorld, bool isForce,
		const btVector3 &linear, const btVector3 &angular) {
	if (isForce && !IsMoveable()) {
		return;
	}
	const btMatrix3x3 &worldTransformBasis = m_RigidBody->getWorldTransform().getBasis();
	bool wake = false;
	if (!linear.isZero()) {
		btVector3 worldLinearAcceleration = linear;
		if (!isWorld) {
			worldLinearAcceleration = worldTransformBasis * worldLinearAcceleration;
		}
		if (isForce) {
			worldLinearAcceleration *= m_RigidBody->getInvMass();
		}
		m_RigidBody->setLinearVelocity(m_RigidBody->getLinearVelocity() +
				worldLinearAcceleration);
		wake = true;
	}
	if (!angular.isZero()) {
		btVector3 localAngularAcceleration = angular;
		if (isWorld) {
			localAngularAcceleration = localAngularAcceleration * worldTransformBasis;
		}
		if (isForce) {
			localAngularAcceleration *= m_RigidBody->getInvInertiaDiagLocal();
		}
		m_RigidBody->setAngularVelocity(m_RigidBody->getAngularVelocity() +
				(worldTransformBasis * localAngularAcceleration));
		wake = true;
	}
	if (wake) {
		Wake();
	}
}

void CPhysicsObject::SetShadow(float maxSpeed, float maxAngularSpeed,
		bool allowPhysicsMovement, bool allowPhysicsRotation) {
	if (m_Shadow == nullptr) {
		m_ShadowTempGravityDisable = false;
		SetCallbackFlags((GetCallbackFlags() | CALLBACK_SHADOW_COLLISION) &
				~(CALLBACK_GLOBAL_FRICTION | CALLBACK_GLOBAL_COLLIDE_STATIC));
		m_Environment->CreateShadowController(this, allowPhysicsMovement, allowPhysicsRotation);
	}
	m_Shadow->MaxSpeed(maxSpeed, maxAngularSpeed);
}

void CPhysicsObject::UpdateShadow(const Vector &targetPosition, const QAngle &targetAngles,
		bool tempDisableGravity, float timeOffset) {
	m_ShadowTempGravityDisable = tempDisableGravity;
	if (m_Shadow != nullptr) {
		m_Shadow->Update(targetPosition, targetAngles, timeOffset);
	}
}

int CPhysicsObject::GetShadowPosition(Vector *position, QAngle *angles) const {
	btTransform transform;
	btTransformUtil::integrateTransform(m_RigidBody->getWorldTransform(),
			m_RigidBody->getLinearVelocity(), m_RigidBody->getAngularVelocity(),
			m_Environment->GetSimulationTimestep(), transform);
	if (position != nullptr) {
		ConvertPositionToHL(transform.getOrigin() -
				(transform.getBasis() * GetBulletMassCenter()), *position);
	}
	if (angles != nullptr) {
		ConvertRotationToHL(transform.getBasis(), *angles);
	}
	return 1; // Used to return the PSI count last simulation (whether a tick happened), but always 1 in v31.
}

IPhysicsShadowController *CPhysicsObject::GetShadowController() const {
	return m_Shadow;
}

void CPhysicsObject::RemoveShadowController() {
	if (m_Shadow != nullptr) {
		m_Environment->DestroyShadowController(m_Shadow);
		Assert(m_Shadow == nullptr);
	}
}

void CPhysicsObject::RemovePlayerController() {
	if (m_Player != nullptr) {
		m_Environment->DestroyPlayerController(m_Player);
		Assert(m_Player == nullptr);
	}
}

bool CPhysicsObject::IsControlledByGame() const {
	if (m_Shadow != nullptr && !m_Shadow->IsPhysicallyControlled()) {
		return true;
	}
	if (GetCallbackFlags() & CALLBACK_IS_PLAYER_CONTROLLER) {
		return true;
	}
	return false;
}

float CPhysicsObject::ComputeShadowControl(const hlshadowcontrol_params_t &params,
		float secondsToArrival, float dt) {
	return ComputeBulletShadowControl(ShadowControlBulletParameters_t(params), secondsToArrival, dt);
}

void CPhysicsObject::NotifyAttachedToShadowController(IPhysicsShadowController *shadow) {
	m_Shadow = shadow;
	UpdateMassProps();
	UpdateMaterial();
}

void CPhysicsObject::NotifyAttachedToPlayerController(
		IPhysicsPlayerController *player, bool notifyEnvironment) {
	Assert((m_Player == nullptr && player != nullptr) || (m_Player != nullptr && player == nullptr));
	CPhysicsEnvironment *environment = static_cast<CPhysicsEnvironment *>(m_Environment);
	if (notifyEnvironment && m_Player != nullptr) {
		environment->NotifyPlayerControllerDetached(m_Player);
	}
	m_Player = player;
	unsigned short callbacks = GetCallbackFlags();
	if (m_Player != nullptr) {
		callbacks |= CALLBACK_IS_PLAYER_CONTROLLER;
	} else {
		callbacks &= ~CALLBACK_IS_PLAYER_CONTROLLER;
	}
	SetCallbackFlags(callbacks);
	if (notifyEnvironment && m_Player != nullptr) {
		environment->NotifyPlayerControllerAttached(m_Player);
	}
}

btScalar CPhysicsObject::ComputeBulletShadowControl(ShadowControlBulletParameters_t &params,
		btScalar secondsToArrival, btScalar timeStep) {
	Assert(m_Environment->IsInSimulation()); // Not going to touch interpolated values here.

	// Resample fraction.
	// This allows us to arrive at the target at the requested time.
	btScalar fraction = 1.0f;
	if (secondsToArrival > 0.0f) {
		fraction = btMin(timeStep / secondsToArrival, btScalar(1.0f));
	}
	secondsToArrival = btMax(secondsToArrival - timeStep, btScalar(0.0f));
	if (fraction <= 0.0f) {
		return secondsToArrival;
	}
	fraction *= 1.0f / timeStep;

	// Not a reference because it may be modified by ProceedToTransform.
	const btTransform &worldTransform = m_RigidBody->getWorldTransform();
	const btVector3 &massCenter = GetBulletMassCenter();
	btVector3 objectPosition = worldTransform.getOrigin() - (worldTransform.getBasis() * massCenter);
	btVector3 localAngularVelocity = m_RigidBody->getAngularVelocity() * worldTransform.getBasis();

	btVector3 deltaPosition = params.m_TargetObjectTransform.getOrigin() - objectPosition;

	if (params.m_TeleportDistance > 0.0f) {
		btScalar dist2;
		if (!params.m_LastObjectPosition.isZero()) {
			dist2 = objectPosition.distance2(params.m_LastObjectPosition);
		} else {
			// UNDONE: This is totally bogus!
			// Measure error using last known estimate, not current position!
			dist2 = deltaPosition.length2();
		}
		if (dist2 > params.m_TeleportDistance * params.m_TeleportDistance) {
			const btMatrix3x3 &targetBasis = params.m_TargetObjectTransform.getBasis();
			ProceedToTransform(btTransform(targetBasis,
					params.m_TargetObjectTransform.getOrigin() + (targetBasis * massCenter)));
			deltaPosition.setZero();
		}
	}

	btVector3 linearVelocity = m_RigidBody->getLinearVelocity();
	CPhysicsShadowController::ComputeVelocity(linearVelocity, deltaPosition,
			params.m_MaxSpeed, params.m_MaxDampSpeed, fraction, params.m_DampFactor, &params.m_LastImpulse);
	m_RigidBody->setLinearVelocity(linearVelocity);

	btVector3 axis;
	btScalar angle;
	btTransformUtil::calculateDiffAxisAngleQuaternion(worldTransform.getRotation(),
			params.m_TargetObjectTransform.getRotation(), axis, angle);
	if (angle > SIMD_PI) {
		angle -= SIMD_2_PI; // Take the shortest path.
	}
	CPhysicsShadowController::ComputeVelocity(localAngularVelocity, (axis * worldTransform.getBasis()) * angle,
			params.m_MaxAngular, params.m_MaxDampAngular, fraction, params.m_DampFactor, nullptr);
	m_RigidBody->setAngularVelocity(worldTransform.getBasis() * localAngularVelocity);

	return secondsToArrival;
}

void CPhysicsObject::SimulateShadowAndPlayer(btScalar timeStep) {
	if (m_Player != nullptr) {
		static_cast<CPhysicsPlayerController *>(m_Player)->Simulate(timeStep);
	}
	if (m_Shadow != nullptr) {
		static_cast<CPhysicsShadowController *>(m_Shadow)->Simulate(timeStep);
	}
}

void CPhysicsObject::NotifyAttachedToVehicleController(IPhysicsVehicleController *vehicle, bool isWheel) {
	if (isWheel) {
		m_WheelOfVehicle = vehicle;
	} else {
		m_BodyOfVehicle = vehicle;
	}
}

bool CPhysicsObject::IsPartOfSameVehicle(const IPhysicsObject *otherObject) const {
	const CPhysicsObject *otherPhysicsObject = static_cast<const CPhysicsObject *>(otherObject);
	bool same = false;
	if (m_BodyOfVehicle != nullptr) {
		same = (m_BodyOfVehicle == otherPhysicsObject->m_WheelOfVehicle) ||
				(m_BodyOfVehicle == otherPhysicsObject->m_BodyOfVehicle);
	}
	if (!same && m_WheelOfVehicle != nullptr) {
		same = (m_WheelOfVehicle == otherPhysicsObject->m_WheelOfVehicle) ||
				(m_WheelOfVehicle == otherPhysicsObject->m_BodyOfVehicle);
	}
	return same;
}

void CPhysicsObject::SimulateVehicle(btScalar timeStep) {
	if (m_BodyOfVehicle != nullptr) {
		static_cast<CPhysicsVehicleController *>(m_BodyOfVehicle)->Simulate(timeStep);
	}
}

/************
 * Collision
 ************/

bool CPhysicsObject::IsCollisionEnabled() const {
	return m_CollisionEnabled;
}

void CPhysicsObject::EnableCollisions(bool enable) {
	if (IsCollisionEnabled() == enable) {
		return;
	}
	m_CollisionEnabled = enable;
	if (!enable) {
		static_cast<CPhysicsEnvironment *>(m_Environment)->RemoveObjectCollisionPairs(m_RigidBody);
	}
}

void CPhysicsObject::RecheckCollisionFilter() {
	static_cast<CPhysicsEnvironment *>(m_Environment)->RecheckObjectCollisionFilter(m_RigidBody);
}

void CPhysicsObject::RecheckContactPoints() {
	// The game calls this only after doing things that change collision rules,
	// so RecheckCollisionFilter is always called prior to this.
	// RecheckCollisionFilter removes obsolete contact points, so this isn't needed.
	// There also were plans to make it recheck contact points in IVP VPhysics.
}

bool CPhysicsObject::GetContactPoint(Vector *contactPoint, IPhysicsObject **contactObject) const {
	const btCollisionDispatcher *dispatcher =
			static_cast<CPhysicsEnvironment *>(m_Environment)->GetCollisionDispatcher();
	int manifoldCount = dispatcher->getNumManifolds();
	for (int manifoldIndex = 0; manifoldIndex < manifoldCount; ++manifoldIndex) {
		const btPersistentManifold *manifold = dispatcher->getManifoldByIndexInternal(manifoldIndex);
		if (manifold->getNumContacts() == 0) {
			continue;
		}
		// Contact point 0 is considered the best by Bullet.
		const btManifoldPoint &manifoldPoint = manifold->getContactPoint(0);
		const btCollisionObject *body0 = manifold->getBody0(), *body1 = manifold->getBody1();
		if (body0 == m_RigidBody) {
			if (!body1->hasContactResponse()) {
				continue;
			}
			if (contactPoint != nullptr) {
				ConvertPositionToHL(manifoldPoint.getPositionWorldOnA(), *contactPoint);
			}
			if (contactObject != nullptr) {
				*contactObject = reinterpret_cast<IPhysicsObject *>(body1->getUserPointer());
			}
			return true;
		}
		if (body1 == m_RigidBody) {
			if (!body0->hasContactResponse()) {
				continue;
			}
			if (contactPoint != nullptr) {
				ConvertPositionToHL(manifoldPoint.getPositionWorldOnB(), *contactPoint);
			}
			if (contactObject != nullptr) {
				*contactObject = reinterpret_cast<IPhysicsObject *>(body0->getUserPointer());
			}
			return true;
		}
	}
	return false;
}

IPhysicsFrictionSnapshot *CPhysicsObject::CreateFrictionSnapshot() {
	return static_cast<CPhysicsEnvironment *>(m_Environment)->CreateFrictionSnapshot(this);
}

void CPhysicsObject::DestroyFrictionSnapshot(IPhysicsFrictionSnapshot *pSnapshot) {
	static_cast<CPhysicsEnvironment *>(m_Environment)->DestroyFrictionSnapshot(pSnapshot);
}

/***********
 * Triggers
 ***********/

bool CPhysicsObject::IsTrigger() const {
	return (m_RigidBody->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0;
}

void CPhysicsObject::BecomeTrigger() {
	if (IsTrigger()) {
		return;
	}
	m_RigidBody->setCollisionFlags(m_RigidBody->getCollisionFlags() |
			btCollisionObject::CF_NO_CONTACT_RESPONSE);
}

void CPhysicsObject::RemoveTrigger() {
	if (!IsTrigger()) {
		return;
	}
	m_RigidBody->setCollisionFlags(m_RigidBody->getCollisionFlags() &
			~btCollisionObject::CF_NO_CONTACT_RESPONSE);
	static_cast<CPhysicsEnvironment *>(m_Environment)->NotifyTriggerRemoved(this);
}

/***************************************
 * Collide object reference linked list
 ***************************************/

void CPhysicsObject::AddReferenceToCollide() {
	IPhysicsObject *next = GetCollide()->AddObjectReference(this);
	if (next != nullptr) {
		m_CollideObjectNext = static_cast<CPhysicsObject *>(next);
		m_CollideObjectPrevious = m_CollideObjectNext->m_CollideObjectPrevious;
		m_CollideObjectNext->m_CollideObjectPrevious = this;
		m_CollideObjectPrevious->m_CollideObjectNext = this;
	} else {
		m_CollideObjectNext = m_CollideObjectPrevious = this;
	}
}

void CPhysicsObject::RemoveReferenceFromCollide() {
	GetCollide()->RemoveObjectReference(this);
	m_CollideObjectNext->m_CollideObjectPrevious = m_CollideObjectPrevious;
	m_CollideObjectPrevious->m_CollideObjectNext = m_CollideObjectNext;
}

/**************
 * Constraints
 **************/

bool CPhysicsObject::IsAttachedToConstraint(bool bExternalOnly) const {
	// TODO: bExternalOnly.
	return m_ValidConstraintCount > 0;
}
