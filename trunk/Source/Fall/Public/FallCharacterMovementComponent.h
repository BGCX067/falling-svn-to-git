// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Fall.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FallCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class FALL_API UFallCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
		UFallCharacterMovementComponent(const class FObjectInitializer& PCIP);
	enum EFallMovementMode : uint8
	{
		CUSTOM_MoonJumping,
		CUSTOM_MoonRunning,
		CUSTOM_MoonWalking
	};
protected:
	FVector GravityDirection, GravityDistanceVector, GravityVector;
	float GravityMagnitude, GravityDistance;

	virtual void CalculateGravity();
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void ApplyAccumulatedForces(float DeltaSeconds) override;
	virtual void SetMovementMode(EMovementMode NewMovementMode);
	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode) override;
	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void PhysWalking(float deltaTime, int32 Iterations);
	virtual void PhysMoonWalking(float deltaTime, int32 Iterations);
	virtual bool IsMoonWalking() const;
	virtual void AdjustFloorHeight() override;
	virtual bool IsWalkable(const FHitResult& Hit) const override;
	virtual void FindFloor(const FVector& CapsuleLocation, FFindFloorResult& OutFloorResult, bool bZeroDelta, const FHitResult* DownwardSweepResult) const override;
	virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const override;
	virtual void MaintainHorizontalGroundVelocity() override;
	virtual bool DoJump(bool bReplayingMoves) override;
	//virtual void SimulateMovement(float DeltaSeconds) override;
	virtual bool IsMovingOnGround() const override;//fixed not jumping , but now can't move
	virtual void OnTeleported() override;
	virtual void SetPostLandedPhysics(const FHitResult& Hit) override;
	virtual void CalcAvoidanceVelocity(float DeltaTime) override;
	virtual void SetDefaultMovementMode() override;
	virtual void PhysFalling(float deltaTime, int32 Iterations) override;
	virtual FVector GetFallingLateralAcceleration(float DeltaTime) override;
	virtual float GetMaxJumpHeight() const override;
	virtual bool SafeMoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult& OutHit);
	virtual void StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc) override;
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;
	virtual bool UFallCharacterMovementComponent::IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const override;

	virtual bool FloorSweepTest(
		FHitResult& OutHit,
		const FVector& Start,
		const FVector& End,
		ECollisionChannel TraceChannel,
		const struct FCollisionShape& CollisionShape,
		const struct FCollisionQueryParams& Params,
		const struct FCollisionResponseParams& ResponseParam
		) const override;
	
};
