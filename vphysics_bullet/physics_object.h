// Copyright Valve Corporation, All rights reserved.
// Bullet integration by Triang3l, derivative work, in public domain if detached from Valve's work.

#ifndef PHYSICS_OBJECT_H
#define PHYSICS_OBJECT_H

#include "physics_internal.h"
#include "tier1/utlvector.h"

class CPhysicsObject : public IPhysicsObject {
public:
	CPhysicsObject(IPhysicsEnvironment *environment,
			const CPhysCollide *collide, int materialIndex,
			const Vector &position, const QAngle &angles,
			const objectparams_t *params, bool isStatic);
	virtual ~CPhysicsObject(); // Must be deleted via Release!

	// IPhysicsObject methods.

	virtual bool IsStatic() const;
	virtual bool IsAsleep() const;
	virtual bool IsTrigger() const;
	/* DUMMY */ virtual bool IsFluid() const { return false; }
	virtual bool IsHinged() const;
	virtual bool IsCollisionEnabled() const;
	virtual bool IsGravityEnabled() const;
	virtual bool IsDragEnabled() const;
	virtual bool IsMotionEnabled() const;
	virtual bool IsMoveable() const;
	virtual bool IsAttachedToConstraint(bool bExternalOnly) const;

	virtual void EnableCollisions(bool enable);
	virtual void EnableGravity(bool enable);
	virtual void EnableDrag(bool enable);
	virtual void EnableMotion(bool enable);

	virtual void SetGameData(void *pGameData);
	virtual void *GetGameData() const;
	virtual void SetGameFlags(unsigned short userFlags);
	virtual unsigned short GetGameFlags() const;
	virtual void SetGameIndex(unsigned short gameIndex);
	virtual unsigned short GetGameIndex() const;

	virtual void SetCallbackFlags(unsigned short callbackflags);
	virtual unsigned short GetCallbackFlags() const;

	virtual void Wake();
	virtual void Sleep();

	virtual void RecheckCollisionFilter();
	virtual void RecheckContactPoints();

	virtual void SetMass(float mass);
	virtual float GetMass() const;
	virtual float GetInvMass() const;
	virtual Vector GetInertia() const;
	virtual Vector GetInvInertia() const;
	virtual void SetInertia(const Vector &inertia);

	virtual void SetDamping(const float *speed, const float *rot);
	virtual void GetDamping(float *speed, float *rot) const;

	virtual void SetDragCoefficient(float *pDrag, float *pAngularDrag);

	/* DUMMY */ virtual void SetBuoyancyRatio(float ratio) {}

	virtual int GetMaterialIndex() const;
	virtual void SetMaterialIndex(int materialIndex);

	virtual unsigned int GetContents() const;
	virtual void SetContents(unsigned int contents);

	virtual float GetSphereRadius() const;
	virtual float GetEnergy() const;
	virtual Vector GetMassCenterLocalSpace() const;

	virtual void SetPosition(const Vector &worldPosition, const QAngle &angles, bool isTeleport);
	virtual void SetPositionMatrix(const matrix3x4_t &matrix, bool isTeleport);
	virtual void GetPosition(Vector *worldPosition, QAngle *angles) const;
	virtual void GetPositionMatrix(matrix3x4_t *positionMatrix) const;

	virtual void SetVelocity(const Vector *velocity, const AngularImpulse *angularVelocity);
	virtual void SetVelocityInstantaneous(const Vector *velocity, const AngularImpulse *angularVelocity);
	virtual void GetVelocity(Vector *velocity, AngularImpulse *angularVelocity) const;
	virtual void AddVelocity(const Vector *velocity, const AngularImpulse *angularVelocity);
	virtual void GetVelocityAtPoint(const Vector &worldPosition, Vector *pVelocity) const;
	/* DUMMY */ virtual void GetImplicitVelocity(Vector *velocity, AngularImpulse *angularVelocity) const {
		if (velocity != nullptr) {
			velocity->Zero();
		}
		if (angularVelocity != nullptr) {
			angularVelocity->Zero();
		}
	}

	virtual void LocalToWorld(Vector *worldPosition, const Vector &localPosition) const;
	virtual void WorldToLocal(Vector *localPosition, const Vector &worldPosition) const;
	virtual void LocalToWorldVector(Vector *worldVector, const Vector &localVector) const;
	virtual void WorldToLocalVector(Vector *localVector, const Vector &worldVector) const;

	virtual void ApplyForceCenter(const Vector &forceVector);
	virtual void ApplyForceOffset(const Vector &forceVector, const Vector &worldPosition);
	virtual void ApplyTorqueCenter(const AngularImpulse &torque);

	virtual void CalculateForceOffset(const Vector &forceVector, const Vector &worldPosition,
			Vector *centerForce, AngularImpulse *centerTorque) const;
	virtual void CalculateVelocityOffset(const Vector &forceVector, const Vector &worldPosition,
			Vector *centerVelocity, AngularImpulse *centerAngularVelocity) const;

	virtual float CalculateLinearDrag(const Vector &unitDirection) const;
	virtual float CalculateAngularDrag(const Vector &objectSpaceRotationAxis) const;

	virtual bool GetContactPoint(Vector *contactPoint, IPhysicsObject **contactObject) const;

	virtual void SetShadow(float maxSpeed, float maxAngularSpeed,
			bool allowPhysicsMovement, bool allowPhysicsRotation);
	virtual void UpdateShadow(const Vector &targetPosition, const QAngle &targetAngles,
			bool tempDisableGravity, float timeOffset);
	virtual int GetShadowPosition(Vector *position, QAngle *angles) const;
	virtual IPhysicsShadowController *GetShadowController() const;
	virtual void RemoveShadowController();
	virtual float ComputeShadowControl(const hlshadowcontrol_params_t &params,
			float secondsToArrival, float dt);

	virtual const CPhysCollide *GetCollide() const;

	virtual const char *GetName() const;

	virtual void BecomeTrigger();
	virtual void RemoveTrigger();

	virtual void BecomeHinged(int localAxis);
	virtual void RemoveHinged();

	virtual IPhysicsFrictionSnapshot *CreateFrictionSnapshot();
	virtual void DestroyFrictionSnapshot(IPhysicsFrictionSnapshot *pSnapshot);

	/* DUMMY */ virtual void OutputDebugInfo() const {}

	// Internal methods.

	FORCEINLINE btRigidBody *GetRigidBody() const { return m_RigidBody; }

	FORCEINLINE IPhysicsEnvironment *GetEnvironment() const { return m_Environment; }

	inline bool WasAsleep() const { return m_WasAsleep; }
	inline bool UpdateEventSleepState() {
		bool wasAsleep = m_WasAsleep;
		m_WasAsleep = IsAsleep();
		return wasAsleep;
	}

	const btVector3 &GetBulletMassCenter() const;
	void GetPositionAtPSI(Vector *worldPosition, QAngle *angles) const;
	void ProceedToTransform(const btTransform &transform);

	// Bullet doesn't allow damping factors over 1, so it has to be done manually.
	// Also applies damping in a way more similar to how IVP VPhysics does it.
	void ApplyDamping(btScalar timeStep);
	void ApplyGravity(btScalar timeStep);

	btScalar CalculateLinearDrag(const btVector3 &velocity) const;
	btScalar CalculateAngularDrag(const btVector3 &objectSpaceRotationAxis) const;
	void ApplyDrag(btScalar timeStep);
	void NotifyOrthographicAreasChanged();

	void UpdateMaterial();

	FORCEINLINE const btVector3 &GetLinearVelocityChange() const {
		return m_LinearVelocityChange;
	}
	FORCEINLINE const btVector3 &GetLocalAngularVelocityChange() const {
		return m_LocalAngularVelocityChange;
	}
	// Moveability not checked - async impulses (not forces) are still okay for internal use.
	FORCEINLINE void SetLinearVelocityChange(const btVector3 &linearVelocityChange) {
		m_LinearVelocityChange = linearVelocityChange;
	}
	FORCEINLINE void SetLocalAngularVelocityChange(const btVector3 &localAngularVelocityChange) {
		m_LocalAngularVelocityChange = localAngularVelocityChange;
	}

	// Bullet integrates forces and torques over time, in IVP async pushes are applied fully.
	void ApplyForcesAndSpeedLimit(btScalar timeStep);

	// No force should EVER be applied with applyForce/applyCentralForce/applyTorque,
	// as we take over force application. Bullet may only apply gravity, but we set it to 0.
	void CheckAndClearBulletForces();

	void NotifyAttachedToMotionController(IPhysicsMotionController *controller);
	void NotifyDetachedFromMotionController(IPhysicsMotionController *controller);
	void SimulateMotionControllers(
			IPhysicsMotionController::priority_t priority, btScalar timeStep);
	void ApplyEventMotion(bool isWorld, bool isForce,
			const btVector3 &linear, const btVector3 &angular);

	bool IsControlledByGame() const;
	void NotifyAttachedToShadowController(IPhysicsShadowController *shadow);
	void NotifyAttachedToPlayerController(
			IPhysicsPlayerController *player, bool notifyEnvironment);
	btScalar ComputeBulletShadowControl(struct ShadowControlBulletParameters_t &params,
			btScalar secondsToArrival, btScalar timeStep);
	void SimulateShadowAndPlayer(btScalar timeStep);
	void RemovePlayerController();

	void NotifyAttachedToVehicleController(IPhysicsVehicleController *vehicle, bool isWheel);
	bool IsPartOfSameVehicle(const IPhysicsObject *otherObject) const;
	void SimulateVehicle(btScalar timeStep);

	FORCEINLINE CPhysicsObject *GetNextCollideObject() const {
		return m_CollideObjectNext;
	}

	FORCEINLINE void AddTriggerTouchReference() {
		++m_TouchingTriggers;
	}
	FORCEINLINE void RemoveTriggerTouchReference() {
		Assert(m_TouchingTriggers > 0);
		m_TouchingTriggers = btMax(m_TouchingTriggers - 1, 0);
	}
	FORCEINLINE bool IsTouchingTriggers() const {
		return m_TouchingTriggers > 0;
	}

	FORCEINLINE bool IsAttachedToConstraintObjects() const {
		return m_ConstraintObjectCount > 0;
	}
	FORCEINLINE void NotifyConstraintAdded(bool valid) {
		++m_ConstraintObjectCount;
		if (valid) {
			++m_ValidConstraintCount;
		}
	}
	FORCEINLINE void NotifyConstraintRemoved(bool valid) {
		if (valid) {
			Assert(m_ValidConstraintCount > 0);
			m_ValidConstraintCount = btMax(m_ValidConstraintCount - 1, 0);
		}
		Assert(m_ConstraintObjectCount > 0);
		m_ConstraintObjectCount = btMax(m_ConstraintObjectCount - 1, 0);
	}
	FORCEINLINE void NotifyAllConstraintsRemoved() {
		m_ValidConstraintCount = m_ConstraintObjectCount = 0;
	}

	void UpdateAfterPSI(); // Only called for non-static objects.

	void InterpolateBetweenPSIs();
	inline const btTransform &GetInterPSIWorldTransform() const {
		return ((IsStatic() || m_Environment->IsInSimulation()) ?
				m_RigidBody->getWorldTransform() : m_InterPSIWorldTransform);
	}

	void NotifyTransferred(IPhysicsEnvironment *newEnvironment);

	// Safe removal of child objects.
	void NotifyQueuedForRemoval();

	// Destruction permitting calling back through virtual functions.
	// NotifyQueuedForRemoval must be called before a call to Release happens!!!
	void Release();

private:
	/***********************************
	 * Properties and persistent values
	 ***********************************/

	IPhysicsEnvironment *m_Environment;

	btRigidBody *m_RigidBody;

	CPhysCollide *GetCollide();
	CPhysicsObject *m_CollideObjectNext, *m_CollideObjectPrevious;
	void AddReferenceToCollide();
	void RemoveReferenceFromCollide();

	btVector3 m_MassCenterOverride;

	float m_Mass;
	Vector m_Inertia;
	int m_HingeHLAxis;
	bool m_MotionEnabled;
	void UpdateMassProps();

	bool m_GravityEnabled;
	bool m_ShadowTempGravityDisable;
	float m_LinearDamping, m_AngularDamping;

	int m_MaterialIndex, m_RealMaterialIndex;
	unsigned int m_ContentsMask;

	btScalar m_LinearDragCoefficient, m_AngularDragCoefficient;
	btVector3 m_LinearDragBasis, m_AngularDragBasis;
	bool m_DragEnabled;
	static btScalar AngularDragIntegral(btScalar l, btScalar w, btScalar h);
	void ComputeDragBases();

	CUtlVector<IPhysicsMotionController *> m_MotionControllers;
	void DetachFromMotionControllers();

	IPhysicsShadowController *m_Shadow;
	IPhysicsPlayerController *m_Player;

	IPhysicsVehicleController *m_BodyOfVehicle;
	IPhysicsVehicleController *m_WheelOfVehicle;

	bool m_CollisionEnabled;

	int m_ConstraintObjectCount, m_ValidConstraintCount;

	void *m_GameData;
	unsigned short m_GameFlags;
	unsigned short m_GameIndex;
	char m_Name[128];

	unsigned short m_Callbacks;

	/******************
	 * Transient state
	 ******************/

	// Was the object active in the previous PSI - used to trigger sleep events.
	bool m_WasAsleep;

	btVector3 m_LinearVelocityChange, m_LocalAngularVelocityChange;

	int m_TouchingTriggers;

	btTransform m_InterPSIWorldTransform;
	btVector3 m_InterPSILinearVelocity, m_InterPSIAngularVelocity;
};

#endif
