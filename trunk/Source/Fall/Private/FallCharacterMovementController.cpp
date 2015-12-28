// Fill out your copyright notice in the Description page of Project Settings.

#include "Fall.h"
#include "FallCharacterMovementComponent.h"



UFallCharacterMovementComponent::UFallCharacterMovementComponent(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	RotationRate = FRotator(360.0f, 360.0f, 360.0f);
	bMaintainHorizontalGroundVelocity = false;
	GravityDistance = 0.f;
	GravityMagnitude = 0.f;
	GravityVector = FVector::ZeroVector;
	GravityDistanceVector = FVector::ZeroVector;
}

void UFallCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	CalculateGravity();
}
void UFallCharacterMovementComponent::TickComponent( float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction )
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CalculateGravity();//FIXME: probably won't have to do the complete calculation every tick
	//GetOwner->ClientSetRotation(GravityDirection);
	FRotator gravRot = GravityDirection.Rotation();
	GetOwner()->SetActorRotation( FRotator(gravRot.Pitch+120.f, gravRot.Yaw, gravRot.Roll ));//better
	//GetOwner()->SetActorRotation( FRotator(gravRot.Pitch, gravRot.Yaw, gravRot.Roll ));//about 90deg off
}
void UFallCharacterMovementComponent::CalculateGravity()
{
	//FIXME Need to iterate over massive things to get vector sum of direction and force
	const float gravBodyMass = 90000000.f;
	GravityDistanceVector = FVector(0.f, 0.f, 5000.f) - GetOwner()->GetActorLocation();
	//GravityDistanceVector = FVector(0.f, 0.f, 5000.f) -  GetOwner()->GetTransform().GetLocation();
	GravityDistance = GravityDistanceVector.Size();
	GravityDirection = GravityDistanceVector.GetSafeNormal();
	//	GravityDirection = GravityDistanceVector/GravityDistance;
	GravityMagnitude = ((Mass * gravBodyMass) / FMath::Square(GravityDistance));
	GravityVector = GravityDirection * GravityMagnitude;
	AddImpulse(GravityVector);
	
	//UE_LOG(LogTemp, Warning, TEXT("CalGrav:%d GDir:%s GM:%f GV:%s"), __LINE__,*GravityDirection.ToString(), GravityMagnitude, *GravityVector.ToString());
}
void UFallCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode){
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	
	UE_LOG(LogTemp, Warning, TEXT("%s %d"), *GetMovementName(), CustomMovementMode);

}

void UFallCharacterMovementComponent::ApplyAccumulatedForces(float DeltaSeconds)
{
	//if (PendingImpulseToApply.Z != 0.f || PendingForceToApply.Z != 0.f)
	if (PendingImpulseToApply.ProjectOnTo(GravityDirection).Size() != 0.f || PendingForceToApply.ProjectOnTo(GravityDirection).Size() != 0.f)//gdg
	{
		// check to see if applied momentum is enough to overcome gravity
			//(PendingImpulseToApply.Z + (PendingForceToApply.Z * DeltaSeconds) + (GetGravityZ() * DeltaSeconds) >  SMALL_NUMBER))
		if (IsMovingOnGround() && ( ((PendingImpulseToApply.ProjectOnTo(GravityDirection)  + (PendingForceToApply * DeltaSeconds).ProjectOnTo(GravityDirection)).Size() - (GravityVector * DeltaSeconds).Size()) >SMALL_NUMBER) )
		{
	UE_LOG(LogTemp, Warning, TEXT("%d AppForce Fly IMP:%s F:%s, applied:%f,  SMALL:%f"), __LINE__, 
		*PendingImpulseToApply.ToString(), *PendingForceToApply.ToString(), 
		(PendingImpulseToApply.ProjectOnTo(GravityDirection)  + (PendingForceToApply * DeltaSeconds).ProjectOnTo(GravityDirection)).Size() - (GravityVector * DeltaSeconds).Size(),
		SMALL_NUMBER
		);
			SetMovementMode(MOVE_Falling,0);
		}
	}

	Velocity += PendingImpulseToApply + (PendingForceToApply * DeltaSeconds);

	PendingImpulseToApply = FVector::ZeroVector;
	PendingForceToApply = FVector::ZeroVector;
}

//probably doing something wrong that I need this to prevent errors about missing 2nd param
void UFallCharacterMovementComponent::SetMovementMode(EMovementMode NewMovementMode)
{
	Super::SetMovementMode(NewMovementMode, 0);
}

void UFallCharacterMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
{
	Super::SetMovementMode(NewMovementMode, NewCustomMode);
}

void UFallCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if (CharacterOwner)
	{
		CharacterOwner->K2_UpdateCustomMovement(deltaTime);//not sure why I'd want to do this?
		switch (CustomMovementMode){
		case CUSTOM_MoonWalking:
			PhysMoonWalking(deltaTime, Iterations);
			break;
		}
	}
}

bool UFallCharacterMovementComponent::IsMoonWalking() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == CUSTOM_MoonWalking;//fart
}


void UFallCharacterMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	//UE_LOG(LogTemp, Warning, TEXT("Physics Walking"));
//	SetMovementMode(MOVE_Custom, CUSTOM_MoonWalking);//fart
	Super::PhysWalking(deltaTime, Iterations);
	//all of the above is probably going away
	//end
}
void UFallCharacterMovementComponent::PhysMoonWalking(float deltaTime, int32 Iterations)
{
	UE_LOG(LogTemp, Warning, TEXT("Dang"));
	if (deltaTime < MIN_TICK_TIME)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d Postpoone tick"), __LINE__);
		return;
	}

	if( (!CharacterOwner || !CharacterOwner->Controller) && !bRunPhysicsWithNoController && !HasRootMotion() )
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		UE_LOG(LogTemp, Warning, TEXT("%d Zeroed movement "), __LINE__);
		return;
	}

	if (!UpdatedComponent->IsCollisionEnabled())
	{
		SetMovementMode(MOVE_Custom, CUSTOM_MoonWalking);//fart
		return;
	}

	checkf(!Velocity.ContainsNaN(), TEXT("PhysMoonWalking: Velocity contains NaN before Iteration (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Velocity.ToString());
	
	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	// Perform the move
	while ( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasRootMotion()) )
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		UPrimitiveComponent * const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		Velocity.Z = 0.f;
		const FVector OldVelocity = Velocity;

		// Apply acceleration
		Acceleration.Z = 0.f;
		if( !HasRootMotion() )
		{
			CalcVelocity(timeTick, GroundFriction, false, BrakingDecelerationWalking);
		}
		checkf(!Velocity.ContainsNaN(), TEXT("PhysMoonWalking: Velocity contains NaN after CalcVelocity (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Velocity.ToString());

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if ( bZeroDelta )
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if ( IsFalling() )
			{
			UE_LOG(LogTemp, Warning, TEXT("%d DesiredDist"), __LINE__);
				// pawn decided to jump up
				const float DesiredDist = Delta.Size();
			UE_LOG(LogTemp, Warning, TEXT("%d DesiredDist:%f"), __LINE__, DesiredDist);
				if (DesiredDist > KINDA_SMALL_NUMBER)
				{
					//@todo gdg
					const float ActualDist = (CharacterOwner->GetActorLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f,ActualDist/DesiredDist));
				}
				StartNewPhysics(remainingTime, Iterations);
				return;
			}
			else if ( IsSwimming() ) //just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if ( bCheckLedges && !CurrentFloor.IsWalkableFloor() )
		{
			// calculate possible alternate movement
			//const FVector GravDir = FVector(0.f,0.f,-1.f);
			const FVector GravDir = GravityDirection;//gdg
			UE_LOG(LogTemp, Warning, TEXT("%d GravityDirection:%s"), __LINE__, *GravityDirection.ToString());
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
			if ( !NewDelta.IsZero() )
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;

				// Try new movement direction
				Velocity = NewDelta/timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on
				bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ( (bMustJump || !bCheckedFall) && CheckFall(CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump) )
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}

		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("you don't have a boner, to penetrate surface"));
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				if (ShouldCatchAir(OldFloor, CurrentFloor))
				{
					CharacterOwner->OnWalkingOffLedge();
					if (IsMovingOnGround())
					{
						// If still walking, then fall. If not, assume the user set a different mode they want to keep.
						StartFalling(Iterations, remainingTime, timeTick, Delta, OldLocation);//not problem
					}
					return;
				}

				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				//Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				Hit.TraceEnd = Hit.TraceStart + (-GravityDirection*MAX_FLOOR_DIST);//gdg
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, CharacterOwner->GetActorRotation());
			}

			// check if just entered water
			if ( IsSwimming() )
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}
			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || 
					(!OldBase->IsCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if (
					(bMustJump || !bCheckedFall) 
					&& CheckFall(CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump) 
					) {
					return;
				}
				bCheckedFall = true;
			}
		}

		//GETS to here normally
	//UE_LOG(LogTemp, Warning, TEXT("And the cow jumped over the Moon %d"), __LINE__);

		// Allow overlap events and such to change physics state and velocity
		if (IsMovingOnGround())
		{
			// Make velocity reflect actual move
			if( !bJustTeleported && !HasRootMotion() && timeTick >= MIN_TICK_TIME)
			{
	UE_LOG(LogTemp, Warning, TEXT("And the cow jumped over the Moon %d"), __LINE__);
				Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick;
			}
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (CharacterOwner->GetActorLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}	
	}

	if (IsMovingOnGround())
	{
		MaintainHorizontalGroundVelocity();
	}
}
//things that sweep
//ComputeFloorDist
void UFallCharacterMovementComponent::AdjustFloorHeight()
{

	// If we have a floor check that hasn't hit anything, don't adjust height.
	if (!CurrentFloor.bBlockingHit)
	{
			UE_LOG(LogTemp, Warning, TEXT("And the cow jumped over the Moon %d"), __LINE__);
		return;
	}

	const float OldFloorDist = CurrentFloor.FloorDist;
	if (CurrentFloor.bLineTrace && OldFloorDist < MIN_FLOOR_DIST)
	{
			UE_LOG(LogTemp, Warning, TEXT("And the cow jumped over the Moon %d"), __LINE__);
		// This would cause us to scale unwalkable walls
		return;
	}

UE_LOG(LogTemp, Warning, TEXT("%d OldFloorDist:%f, MIN:%f, MAX:%f"),
	__LINE__, OldFloorDist,MIN_FLOOR_DIST,MAX_FLOOR_DIST);
	// Move up or down to maintain floor height.
	if (OldFloorDist < MIN_FLOOR_DIST || OldFloorDist > MAX_FLOOR_DIST)
	{
		FHitResult AdjustHit(1.f);
		const float InitialZ = UpdatedComponent->GetComponentLocation().Z;
		//const FVector InitialZ = UpdatedComponent->GetComponentLocation();//gdg
		const float AvgFloorDist = (MIN_FLOOR_DIST + MAX_FLOOR_DIST) * 0.5f;
		const float MoveDist = AvgFloorDist - OldFloorDist;
		//const float MoveDist = FMath::Abs(AvgFloorDist - OldFloorDist);//gdg
		//SafeMoveUpdatedComponent( FVector(0.f,0.f,MoveDist), CharacterOwner->GetActorRotation(), true, AdjustHit );
		SafeMoveUpdatedComponent( GravityDirection * MoveDist, CharacterOwner->GetActorRotation(), true, AdjustHit );//gdg
		UE_LOG(LogTemp, Warning, TEXT("Adjust floor height %.3f (Hit = %d)"), MoveDist, AdjustHit.bBlockingHit);

		//	UE_LOG(LogTemp, Warning, TEXT("Mine %d Floor:%.3f BlockingHit:%d"), __LINE__, (GravityDirection * MoveDist).Size() , AdjustHit.bBlockingHit);
		//	UE_LOG(LogTemp, Warning, TEXT("Orig %d Floor:%.3f"), __LINE__, FVector(0.f,0.f,MoveDist).Size() );
		if (!AdjustHit.IsValidBlockingHit())
		{
			UE_LOG(LogTemp, Warning, TEXT("And the cow jumped over the Moon %d"), __LINE__);//here
			CurrentFloor.FloorDist += MoveDist;
			//CurrentFloor.FloorDist -= MoveDist;
		}
		else if (MoveDist > 0.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("And the cow jumped over the Moon %d"), __LINE__);
			const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
			//const FVector CurrentZ = UpdatedComponent->GetComponentLocation();//gdg
			CurrentFloor.FloorDist += CurrentZ - InitialZ;
			//CurrentFloor.FloorDist += FVector::Dist(InitialZ, CurrentZ );//gdg

			//UE_LOG(LogTemp, Warning, TEXT("Mine %d Floor:%.3f "), __LINE__,FVector::Dist(InitialZ, CurrentZ ) );
			//UE_LOG(LogTemp, Warning, TEXT("Orig %d Floor:%.3f"), __LINE__, CurrentZ.Z - InitialZ.Z );
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("And the cow jumped over the Moon %d"), __LINE__);
			checkSlow(MoveDist < 0.f);
			const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
			//const FVector CurrentZ = UpdatedComponent->GetComponentLocation();//gdg
			CurrentFloor.FloorDist = CurrentZ - AdjustHit.Location.Z;
			//CurrentFloor.FloorDist = FVector::Dist(AdjustHit.Location, CurrentZ );//gdg
			
			//UE_LOG(LogTemp, Warning, TEXT("Mine %d Floor:%.3f "), __LINE__,FVector::Dist(InitialZ, CurrentZ ) );
			//UE_LOG(LogTemp, Warning, TEXT("Orig %d Floor:%.3f"), __LINE__, CurrentZ.Z - AdjustHit.Location.Z );

			if (IsWalkable(AdjustHit))
			{
			UE_LOG(LogTemp, Warning, TEXT("And the cow jumped over the Moon %d"), __LINE__);
			CurrentFloor.SetFromSweep(AdjustHit, CurrentFloor.FloorDist, true);
			}
		}

		// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
		// Also avoid it if we moved out of penetration
		bJustTeleported |= !bMaintainHorizontalGroundVelocity || (OldFloorDist < 0.f);

	}
		UE_LOG(LogTemp, Warning, TEXT("%d GravityDirection:%s, Teleported:%d"), __LINE__, *GravityDirection.ToString(),bJustTeleported);
		UE_LOG(LogTemp, Warning, TEXT("%d Floor Distance OLD DIST:%f, NEW:%f DIFF:%f"), __LINE__, 
			OldFloorDist, CurrentFloor.FloorDist, (OldFloorDist - CurrentFloor.FloorDist));
}




















bool UFallCharacterMovementComponent::IsWalkable(const FHitResult& Hit) const
{
	if (!Hit.IsValidBlockingHit())
	{
			UE_LOG(LogTemp, Warning, TEXT("%d IsWalkable"), __LINE__);
		// No hit, or starting in penetration
		return false;
	}

	// Never walk up vertical surfaces. bah humbug
	/*
			FVector tmp = Velocity.ProjectOnTo( GravityVector);//gdg this one is smaller than one above
*/
	//if (Hit.ImpactNormal.Z < KINDA_SMALL_NUMBER)
	if (Hit.ImpactNormal.ProjectOnTo( GravityVector).Size() < KINDA_SMALL_NUMBER)//gdg
	{
			UE_LOG(LogTemp, Warning, TEXT("%d IsWalkable NOT"), __LINE__);
		return false;
	}
	//float TestWalkableZ = GetWalkableFloorZ();
	FVector TestWalkable = -GravityDirection;//gdg

	// See if this component overrides the walkable floor z.
	const UPrimitiveComponent* HitComponent = Hit.Component.Get();
	/*
	if (HitComponent)
	{
			UE_LOG(LogTemp, Warning, TEXT("%d IsWalkable"), __LINE__);
			//gdg
		//const FWalkableSlopeOverride& SlopeOverride = HitComponent->GetWalkableSlopeOverride();
		//TestWalkableZ = SlopeOverride.ModifyWalkableFloorZ(TestWalkableZ);
			//end gdg
	}
	*/

	// Can't walk on this surface if it is too steep.
	//if (Hit.ImpactNormal.Z < TestWalkableZ)
	//gdg, goofed up a lot here. 
	//TODO move player to north pole and see if I'm getting the same results as orginal
	//TODO I suspect there are other places where I should have been using dot product instead of projection. 
	const FVector bert = TestWalkable;
	const FVector earnie = Hit.ImpactNormal;
	//if (Hit.ImpactNormal.ProjectOnTo( GravityVector).Size() < TestWalkable.ProjectOnTo( GravityVector).Size())//gdg
	if ( FVector::DotProduct( bert, earnie)<GetWalkableFloorZ())
	{
			UE_LOG(LogTemp, Warning, TEXT("%d IsWalkable"), __LINE__);
		//UE_LOG(LogTemp, Warning, TEXT("ImpZ:%f, WalkZ:%f"), Hit.ImpactNormal.Z, TestWalkableZ);
		return false;
	}

	return true;
}


void UFallCharacterMovementComponent::FindFloor(const FVector& CapsuleLocation, FFindFloorResult& OutFloorResult, bool bZeroDelta, const FHitResult* DownwardSweepResult) const
{
	// No collision, no floor...
	if (!UpdatedComponent->IsCollisionEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
		OutFloorResult.Clear();
		return;
	}

	// Increase height check slightly if walking, to prevent floor height adjustment from later invalidating the floor result.
	const float HeightCheckAdjust = (IsMovingOnGround() ? MAX_FLOOR_DIST + KINDA_SMALL_NUMBER : -MAX_FLOOR_DIST);

	float FloorSweepTraceDist = FMath::Max(MAX_FLOOR_DIST, MaxStepHeight + HeightCheckAdjust);
	float FloorLineTraceDist = FloorSweepTraceDist;
	bool bNeedToValidateFloor = true;
	
	// Sweep floor
	if (FloorLineTraceDist > 0.f || FloorSweepTraceDist > 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
		UCharacterMovementComponent* MutableThis = const_cast<UFallCharacterMovementComponent*>(this);

		if ( bAlwaysCheckFloor || !bZeroDelta || bForceNextFloorCheck || bJustTeleported )
		{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
			MutableThis->bForceNextFloorCheck = false;
			ComputeFloorDist(CapsuleLocation, FloorLineTraceDist, FloorSweepTraceDist, OutFloorResult, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), DownwardSweepResult);
		}
		else
		{
			// Force floor check if base has collision disabled or if it does not block us.
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
			UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
			const AActor* BaseActor = MovementBase ? MovementBase->GetOwner() : NULL;
			const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

			if (MovementBase != NULL)
			{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
				MutableThis->bForceNextFloorCheck = !MovementBase->IsCollisionEnabled()
				|| MovementBase->GetCollisionResponseToChannel(CollisionChannel) != ECR_Block
				|| (MovementBase->Mobility == EComponentMobility::Movable)
				|| MovementBaseUtility::IsDynamicBase(MovementBase)
				|| (Cast<const ADestructibleActor>(BaseActor) != NULL);
			}

			const bool IsActorBasePendingKill = BaseActor && BaseActor->IsPendingKill();

			if ( !bForceNextFloorCheck && !IsActorBasePendingKill && MovementBase )
			{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
				//UE_LOG(LogCharacterMovement, Log, TEXT("%s SKIP check for floor"), *CharacterOwner->GetName());
				OutFloorResult = CurrentFloor;
				bNeedToValidateFloor = false;
			}
			else
			{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
				MutableThis->bForceNextFloorCheck = false;
				ComputeFloorDist(CapsuleLocation, FloorLineTraceDist, FloorSweepTraceDist, OutFloorResult, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), DownwardSweepResult);
			}
		}
	}
		UE_LOG(LogTemp, Warning, TEXT("Line:%d bNeedValidateFloor:%d bBlockingHit:%d bLineTrace:%d"),
			__LINE__, bNeedToValidateFloor, OutFloorResult.bBlockingHit, OutFloorResult.bLineTrace);
		//The OutFloorResult.bBlockingHit goes from 1 to 0 when fall off and none of the code below executes

	// OutFloorResult.HitResult is now the result of the vertical floor check.
		UE_LOG(LogTemp, Warning, TEXT("Line:%d HitResult.PenetrationDepth:%f"),__LINE__, OutFloorResult.HitResult.PenetrationDepth);
	// See if we should try to "perch" at this location.
	//gdg, I think I don't care about perching yet
	if (bNeedToValidateFloor && OutFloorResult.bBlockingHit && !OutFloorResult.bLineTrace)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
		const bool bCheckRadius = true;
		if (ShouldComputePerchResult(OutFloorResult.HitResult, bCheckRadius))
		{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
			float MaxPerchFloorDist = FMath::Max(MAX_FLOOR_DIST, MaxStepHeight + HeightCheckAdjust);
			if (IsMovingOnGround())
			{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
				MaxPerchFloorDist += FMath::Max(0.f, PerchAdditionalHeight);
			}

			FFindFloorResult PerchFloorResult;
			if (ComputePerchResult(GetValidPerchRadius(), OutFloorResult.HitResult, MaxPerchFloorDist, PerchFloorResult))
			{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
				// Don't allow the floor distance adjustment to push us up too high, or we will move beyond the perch distance and fall next time.
				const float AvgFloorDist = (MIN_FLOOR_DIST + MAX_FLOOR_DIST) * 0.5f;
				const float MoveUpDist = (AvgFloorDist - OutFloorResult.FloorDist);
				if (MoveUpDist + PerchFloorResult.FloorDist >= MaxPerchFloorDist)
				{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
					OutFloorResult.FloorDist = AvgFloorDist;
				}

				// If the regular capsule is on an unwalkable surface but the perched one would allow us to stand, override the normal to be one that is walkable.
				if (!OutFloorResult.bWalkableFloor)
				{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
					OutFloorResult.SetFromLineTrace(PerchFloorResult.HitResult, OutFloorResult.FloorDist, FMath::Min(PerchFloorResult.FloorDist, PerchFloorResult.LineDist), true);
				}
			}
			else
			{
		UE_LOG(LogTemp, Warning, TEXT("%d"), __LINE__);
				// We had no floor (or an invalid one because it was unwalkable), and couldn't perch here, so invalidate floor (which will cause us to start falling).
				OutFloorResult.bWalkableFloor = false;
			}
		}
	}
}

void UFallCharacterMovementComponent::ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, 
	float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const
{
	OutFloorResult.Clear();

	//	UE_LOG(LogTemp, Warning, TEXT("%d Comp SweepDistance:%f, SweepRadius:%f, LineDist:%f"), 
	//		__LINE__,SweepDistance, SweepRadius, LineDistance); //618 Comp
	///always seems to be this
	// 618 Comp SweepDistance : 47.400101, SweepRadius : 42.000000, LineDist : 47.400101
	// No collision, no floor...
	if (!UpdatedComponent->IsCollisionEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
		return;
	}

	float PawnRadius, PawnHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	bool bSkipSweep = false;
	if (DownwardSweepResult != NULL && DownwardSweepResult->IsValidBlockingHit())
	{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
		// Only if the supplied sweep was vertical and downward.
		if ((DownwardSweepResult->TraceStart.Z > DownwardSweepResult->TraceEnd.Z) &&
			(DownwardSweepResult->TraceStart - DownwardSweepResult->TraceEnd).SizeSquared2D() <= KINDA_SMALL_NUMBER)
		{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
			// Reject hits that are barely on the cusp of the radius of the capsule
			if (IsWithinEdgeTolerance(DownwardSweepResult->Location, DownwardSweepResult->ImpactPoint, PawnRadius))
			{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
				// Don't try a redundant sweep, regardless of whether this sweep is usable.
				bSkipSweep = true;

				const bool bIsWalkable = IsWalkable(*DownwardSweepResult);
				const float FloorDist = (CapsuleLocation.Z - DownwardSweepResult->Location.Z);
				OutFloorResult.SetFromSweep(*DownwardSweepResult, FloorDist, bIsWalkable);
				
				if (bIsWalkable)
				{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
					// Use the supplied downward sweep as the floor hit result.			
					return;
				}
			}
		}
	}

	// We require the sweep distance to be >= the line distance, otherwise the HitResult can't be interpreted as the sweep result.
	if (SweepDistance < LineDistance)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
		check(SweepDistance >= LineDistance);
		return;
	}

		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__); //666 Comp
	bool bBlockingHit = false;
	FCollisionQueryParams QueryParams(NAME_None, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(QueryParams, ResponseParam);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

	// Sweep test
	if (!bSkipSweep && SweepDistance > 0.f && SweepRadius > 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);//Penultimate line bf Bail to fall
		// Use a shorter height to avoid sweeps giving weird results if we start on a surface.
		// This also allows us to adjust out of penetrations.
		const float ShrinkScale = 0.9f;
		const float ShrinkScaleOverlap = 0.1f;
		float ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScale);
		float TraceDist = SweepDistance + ShrinkHeight;

		static const FName ComputeFloorDistName(TEXT("ComputeFloorDistSweep"));
		QueryParams.TraceTag = ComputeFloorDistName;
		FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(SweepRadius, PawnHalfHeight - ShrinkHeight);

		FHitResult Hit(1.f);









		//This must be what fails
		/*
		bBlockingHit = FloorSweepTest(Hit, CapsuleLocation, CapsuleLocation 
		+ FVector(0.f,0.f,-TraceDist), CollisionChannel, CapsuleShape, 
			QueryParams, ResponseParam);
*/




















		bBlockingHit = FloorSweepTest(Hit, CapsuleLocation,  
		  CapsuleLocation+(GravityDirection * TraceDist), CollisionChannel, CapsuleShape, 
			QueryParams, ResponseParam);//gdg
		UE_LOG(LogTemp, Warning, TEXT("Line:%d Hit.PenetrationDepth:%f TraceDist:%f"),
			__LINE__, Hit.PenetrationDepth, TraceDist);













		if (bBlockingHit)
		{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
			// Reject hits adjacent to us, we only care about hits on the bottom portion of our capsule.
			// Check 2D distance to impact point, reject if within a tolerance from radius.
			if (Hit.bStartPenetrating || !IsWithinEdgeTolerance(CapsuleLocation, Hit.ImpactPoint, CapsuleShape.Capsule.Radius))
			{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
				// Use a capsule with a slightly smaller radius and shorter height to avoid the adjacent object.
				ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScaleOverlap);
				TraceDist = SweepDistance + ShrinkHeight;
				CapsuleShape.Capsule.Radius = FMath::Max(0.f, CapsuleShape.Capsule.Radius - SWEEP_EDGE_REJECT_DISTANCE - KINDA_SMALL_NUMBER);
				CapsuleShape.Capsule.HalfHeight = FMath::Max(PawnHalfHeight - ShrinkHeight, CapsuleShape.Capsule.Radius);
				Hit.Reset(1.f, false);

				//bBlockingHit = FloorSweepTest(Hit, CapsuleLocation, CapsuleLocation + FVector(0.f,0.f,-TraceDist), CollisionChannel, CapsuleShape, QueryParams, ResponseParam);
				bBlockingHit = FloorSweepTest(Hit, CapsuleLocation, CapsuleLocation + (GravityDirection * TraceDist), 
					CollisionChannel, CapsuleShape, QueryParams, ResponseParam);//gdg
			}

			// Reduce hit distance by ShrinkHeight because we shrank the capsule for the trace.
			// We allow negative distances here, because this allows us to pull out of penetrations.
			const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
			const float SweepResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

			UE_LOG(LogTemp, Warning, TEXT("%d Comp Pene:%f, SweepRes:%f, TDist:%f"), __LINE__,
				MaxPenetrationAdjust, SweepResult, TraceDist);
			Hit.IsValidBlockingHit();
			OutFloorResult.SetFromSweep(Hit, SweepResult, false);
			UE_LOG(LogTemp, Warning, TEXT("%d Comp blocking:%d, walkable:%d"), __LINE__, Hit.IsValidBlockingHit(), IsWalkable(Hit));







			if (Hit.IsValidBlockingHit() && IsWalkable(Hit))
			{
				UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);//here
				if (SweepResult <= SweepDistance)
				{
					UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__); //here
					//gdg BAILS here when not starting to fall
					// Hit within test distance.
					OutFloorResult.bWalkableFloor = true;
					return;
				}
			}
		}
	}

	// Since we require a longer sweep than line trace, we don't want to run the line trace if the sweep missed everything.
	// We do however want to try a line trace if the sweep was stuck in penetration.
	if (!OutFloorResult.bBlockingHit && !OutFloorResult.HitResult.bStartPenetrating)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
		//gdg BAILS here when fallingng starts
		OutFloorResult.FloorDist = SweepDistance; //I think that's setting FloorDist to whatever was passed to this func 
		return;
	}

	// Line trace
	if (LineDistance > 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
		const float ShrinkHeight = PawnHalfHeight;
		const FVector LineTraceStart = CapsuleLocation;	
		const float TraceDist = LineDistance + ShrinkHeight;
		//const FVector Down = FVector(0.f, 0.f, -TraceDist);
		const FVector Down = (GravityDirection * TraceDist);//gdg

		static const FName FloorLineTraceName = FName(TEXT("ComputeFloorDistLineTrace"));
		QueryParams.TraceTag = FloorLineTraceName;

		FHitResult Hit(1.f);
		bBlockingHit = GetWorld()->LineTraceSingle(Hit, LineTraceStart, LineTraceStart + Down, CollisionChannel, QueryParams, ResponseParam);

		if (bBlockingHit)
		{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
			if (Hit.Time > 0.f)
			{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
				// Reduce hit distance by ShrinkHeight because we started the trace higher than the base.
				// We allow negative distances here, because this allows us to pull out of penetrations.
				const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
				const float LineResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

				OutFloorResult.bBlockingHit = true;
				if (LineResult <= LineDistance && IsWalkable(Hit))
				{
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
					OutFloorResult.SetFromLineTrace(Hit, OutFloorResult.FloorDist, LineResult, true);
					return;
				}
			}
		}
	}
	
		UE_LOG(LogTemp, Warning, TEXT("%d Comp"), __LINE__);
	// No hits were acceptable.
	OutFloorResult.bWalkableFloor = false;
	OutFloorResult.FloorDist = SweepDistance;
}

void UFallCharacterMovementComponent::MaintainHorizontalGroundVelocity()
{
	//if (Velocity.Z != 0.f)
			//FVector tmp = Velocity.ProjectOnTo( GravityDirection );//gdg
			//FVector tmp = GravityDirection.ProjectOnTo( Velocity );//gdg this one is smaller than one above
			//FVector tmp = GravityVector.ProjectOnTo( Velocity );//gdg this one is smaller than one above
			FVector tmp = Velocity.ProjectOnTo( GravityVector);//gdg this one is smaller than one above
			UE_LOG(LogTemp, Warning, TEXT("tmp:%s"), *tmp.ToString() );
	if (tmp.Size() != 0.f)//gdg
	{
		if (bMaintainHorizontalGroundVelocity)
		{
			// Ramp movement already maintained the velocity, so we just want to remove the vertical component.
			Velocity.Z = 0.f; //Wish I knew, vertical relative to what?
//			Velocity = Velocity + Velocity.ProjectOnTo(GravityDirection);//gdg

		}
		else
		{
			// Rescale velocity to be horizontal but maintain magnitude of last update.
//			Velocity = Velocity.SafeNormal2D() * Velocity.Size();
			//not sure I grasp the intent here

			UE_LOG(LogTemp, Warning, TEXT("Vel:%s"), *Velocity.ToString());
			//Velocity = (Velocity - tmp) * tmp.Size();//gdg ... definately not right
			Velocity = (Velocity - tmp);//gdg ... legit? Math is hard.

		}
	}
}
bool UFallCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
	if ( CharacterOwner && CharacterOwner->CanJump() )
	{
		FVector js = -GravityDirection * JumpZVelocity; //gdg shouldn't that be speed?
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
		//	Velocity.Z = JumpZVelocity;
			Velocity += js;//gdg
	UE_LOG(LogTemp, Warning, TEXT("%d Jumped, going to fall"), __LINE__ );
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}
	
	return false;
}

bool UFallCharacterMovementComponent::IsMovingOnGround() const
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return false;
	}

	return (MovementMode == MOVE_Walking) || (MovementMode == MOVE_Custom && CustomMovementMode == CUSTOM_MoonWalking);
}
void UFallCharacterMovementComponent::SetPostLandedPhysics(const FHitResult& Hit)
{
	if( CharacterOwner )
	{
		if ( GetPhysicsVolume()->bWaterVolume && CanEverSwim() )
		{
			SetMovementMode(MOVE_Swimming);
		}
		else
		{
			const FVector PreImpactAccel = Acceleration + (IsFalling() ? GravityVector : FVector::ZeroVector);
			const FVector PreImpactVelocity = Velocity;
			//SetMovementMode(MOVE_Walking);
			SetMovementMode(MOVE_Custom, CUSTOM_MoonWalking);//gdg
			ApplyImpactPhysicsForces(Hit, PreImpactAccel, PreImpactVelocity);
		}
	}
}

void UFallCharacterMovementComponent::OnTeleported()
{
	bJustTeleported = true;
	if (!HasValidData())
	{
		return;
	}

	// Find floor at current location
	UpdateFloorFromAdjustment();
	SaveBaseLocation();

	// Validate it. We don't want to pop down to walking mode from very high off the ground, but we'd like to keep walking if possible.
	UPrimitiveComponent* OldBase = CharacterOwner->GetMovementBase();
	UPrimitiveComponent* NewBase = NULL;
	
	if (OldBase && CurrentFloor.IsWalkableFloor() && CurrentFloor.FloorDist <= MAX_FLOOR_DIST && Velocity.Z <= 0.f)
	{
		// Close enough to land or just keep walking.
		NewBase = CurrentFloor.HitResult.Component.Get();
	}
	else
	{
		CurrentFloor.Clear();
	}

	// If we were walking but no longer have a valid base or floor, start falling.
	if (!CurrentFloor.IsWalkableFloor() || (OldBase && !NewBase))
	{
		if (DefaultLandMovementMode == MOVE_Walking || (DefaultLandMovementMode==MOVE_Custom && CustomMovementMode==CUSTOM_MoonWalking) )
		{
	UE_LOG(LogTemp, Warning, TEXT("%d Teleported, going to fall"), __LINE__ );
			SetMovementMode(MOVE_Falling);
			//SetMovementMode(MOVE_Custom, CUSTOM_MoonWalking);//gdg
		}
		else
		{
			SetDefaultMovementMode();
		}
	}
}
void UFallCharacterMovementComponent::CalcAvoidanceVelocity(float DeltaTime)
{
	UE_LOG(LogTemp, Warning, TEXT("CALC AVOID"));
	Super::CalcAvoidanceVelocity(DeltaTime);
}

void UFallCharacterMovementComponent::SetDefaultMovementMode()
{
	// check for water volume
	if ( IsInWater() && CanEverSwim() )
	{
		SetMovementMode(DefaultWaterMovementMode);
	}
	else if ( !CharacterOwner || MovementMode != DefaultLandMovementMode )
	{
		SetMovementMode(DefaultLandMovementMode);

		// Avoid 1-frame delay if trying to walk but walking fails at this location.
		if ( (MovementMode == MOVE_Walking || (MovementMode==MOVE_Custom && CustomMovementMode==CUSTOM_MoonWalking) ) && GetMovementBase() == NULL)
		{
	UE_LOG(LogTemp, Warning, TEXT("%d DefMoveMode, going to fall"), __LINE__ );
			SetMovementMode(MOVE_Falling);
		}
	}
}

float UFallCharacterMovementComponent::GetMaxJumpHeight() const
{
	//const float Gravity = GetGravityZ();
	const float Gravity = GravityMagnitude;//gdg
	if (FMath::Abs(Gravity) > KINDA_SMALL_NUMBER)
	{
		return FMath::Square(JumpZVelocity) / (-2.f * Gravity);
	}
	else
	{
		return 0.f;
	}
}

FVector UFallCharacterMovementComponent::GetFallingLateralAcceleration(float DeltaTime)
{
	// No acceleration in Z
	//FVector FallAcceleration = FVector(Acceleration.X, Acceleration.Y, 0.f);
	FVector FallAcceleration = Acceleration - GravityVector;//gdg

	// bound acceleration, falling object has minimal ability to impact acceleration
	//if (!HasRootMotion() && FallAcceleration.SizeSquared2D() > 0.f)
	if (!HasRootMotion() && FallAcceleration.SizeSquared() > 0.f)//gdg
	{
		FallAcceleration = GetAirControl(DeltaTime, AirControl, FallAcceleration);
		FallAcceleration = FallAcceleration.GetClampedToMaxSize(GetMaxAcceleration());
	}

	return FallAcceleration;
}

void UFallCharacterMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	FVector FallAcceleration = GetFallingLateralAcceleration(deltaTime);
	//FallAcceleration.Z = 0.f;
	FallAcceleration = GravityVector;
	//const bool bHasAirControl = (FallAcceleration.SizeSquared2D() > 0.f);
	const bool bHasAirControl = (FallAcceleration.SizeSquared2D() > 0.f);

	float remainingTime = deltaTime;
	while( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) )
	{
		Iterations++;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;
		
		const FVector OldLocation = CharacterOwner->GetActorLocation();
		const FRotator PawnRotation = CharacterOwner->GetActorRotation();
		bJustTeleported = false;

		FVector OldVelocity = Velocity;
		FVector VelocityNoAirControl = Velocity;

		// Apply input
		if (!HasRootMotion())
		{
			// Compute VelocityNoAirControl
			if (bHasAirControl)
			{
				// Find velocity *without* acceleration.
				TGuardValue<FVector> RestoreAcceleration(Acceleration, FVector::ZeroVector);
				TGuardValue<FVector> RestoreVelocity(Velocity, Velocity);
				//Velocity.Z = 0.f;
				Velocity = Velocity - GravityVector;
				CalcVelocity(timeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
				//VelocityNoAirControl = FVector(Velocity.X, Velocity.Y, OldVelocity.Z);
				VelocityNoAirControl = Velocity - (GravityDirection.ProjectOnTo(OldVelocity));//gdg
			}

			// Compute Velocity
			{
				// Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
				TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);
				//Velocity.Z = 0.f;
				Velocity = Velocity - (GravityDirection.ProjectOnTo(OldVelocity));//gdg
				CalcVelocity(timeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
				//Velocity.Z = OldVelocity.Z;
				Velocity = OldVelocity; //gdg
			}

			// Just copy Velocity to VelocityNoAirControl if they are the same (ie no acceleration).
			if (!bHasAirControl)
			{
				VelocityNoAirControl = Velocity;
			}
		}

		// Apply gravity
		//const FVector Gravity(0.f, 0.f, GetGravityZ());
		const FVector Gravity=GravityVector;//gdg
		Velocity = NewFallVelocity(Velocity, Gravity, timeTick);
		VelocityNoAirControl = NewFallVelocity(VelocityNoAirControl, Gravity, timeTick);
		const FVector AirControlAccel = (Velocity - VelocityNoAirControl) / timeTick;

		//if( bNotifyApex && CharacterOwner->Controller && (Velocity.Z <= 0.f) )
		if( bNotifyApex && CharacterOwner->Controller && ((GravityDirection.ProjectOnTo(Velocity)).Size() <= 0.f) )
		{
			// Just passed jump apex since now going down
			bNotifyApex = false;
			NotifyJumpApex();
		}

		// Move
		FHitResult Hit(1.f);
		FVector Adjusted = 0.5f*(OldVelocity + Velocity) * timeTick;
		SafeMoveUpdatedComponent( Adjusted, PawnRotation, true, Hit);
		
		if (!HasValidData())
		{
			return;
		}
		
		float LastMoveTimeSlice = timeTick;
		float subTimeTickRemaining = timeTick * (1.f - Hit.Time);
		
		if ( IsSwimming() ) //just entered water
		{
			remainingTime += subTimeTickRemaining;
			StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
			return;
		}
		else if ( Hit.bBlockingHit )
		{
		UE_LOG(LogTemp, Warning, TEXT("%d BlockingHit "), __LINE__ );
			if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				remainingTime += subTimeTickRemaining;
		UE_LOG(LogTemp, Warning, TEXT("%d Good place to land "), __LINE__ );
				ProcessLanded(Hit, remainingTime, Iterations);
				return;
			}
			else
			{
				// Compute impact deflection based on final velocity, not integration step.
				// This allows us to compute a new velocity from the deflected vector, and ensures the full gravity effect is included in the slide result.
				Adjusted = Velocity * timeTick;

				// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
				if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(timeTick, Adjusted, Hit))
				{
					const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
					FFindFloorResult FloorResult;
					//FindFloor(PawnLocation, FloorResult, false);
					Super::FindFloor(PawnLocation, FloorResult, false);//gdg
					if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
					{
						remainingTime += subTimeTickRemaining;
						ProcessLanded(FloorResult.HitResult, remainingTime, Iterations);
						return;
					}
				}

				HandleImpact(Hit, LastMoveTimeSlice, Adjusted);
				
				// If we've changed physics mode, abort.
				if (!HasValidData() || !IsFalling())
				{
					return;
				}

				// Limit air control based on what we hit.
				// We moved to the impact point using air control, but may want to deflect from there based on a limited air control acceleration.
				if (bHasAirControl)
				{
					const bool bCheckLandingSpot = false; // we already checked above.
					const FVector AirControlDeltaV = LimitAirControl(LastMoveTimeSlice, AirControlAccel, Hit, bCheckLandingSpot) * LastMoveTimeSlice;
					Adjusted = (VelocityNoAirControl + AirControlDeltaV) * LastMoveTimeSlice;
				}

				const FVector OldHitNormal = Hit.Normal;
				const FVector OldHitImpactNormal = Hit.ImpactNormal;				
				FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

				// Compute velocity after deflection (only gravity component for RootMotion)
				if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
				{
					const FVector NewVelocity = (Delta / subTimeTickRemaining);
					Velocity = HasRootMotion() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
				}

				if (subTimeTickRemaining > KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.f)
				{
					// Move in deflected direction.
					SafeMoveUpdatedComponent( Delta, PawnRotation, true, Hit);
					
					if (Hit.bBlockingHit)
					{
						// hit second wall
						LastMoveTimeSlice = subTimeTickRemaining;
						subTimeTickRemaining = subTimeTickRemaining * (1.f - Hit.Time);

						if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
						{
							remainingTime += subTimeTickRemaining;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}

						HandleImpact(Hit, LastMoveTimeSlice, Delta);

						// If we've changed physics mode, abort.
						if (!HasValidData() || !IsFalling())
						{
							return;
						}

						// Act as if there was no air control on the last move when computing new deflection.
						//if (bHasAirControl && Hit.Normal.Z > VERTICAL_SLOPE_NORMAL_Z)
						//{
							const FVector LastMoveNoAirControl = VelocityNoAirControl * LastMoveTimeSlice;
							Delta = ComputeSlideVector(LastMoveNoAirControl, 1.f, OldHitNormal, Hit);
						//}
							//gdg not sure what to do there

						FVector PreTwoWallDelta = Delta;
						TwoWallAdjust(Delta, Hit, OldHitNormal);

						// Limit air control, but allow a slide along the second wall.
						if (bHasAirControl)
						{
							const bool bCheckLandingSpot = false; // we already checked above.
							const FVector AirControlDeltaV = LimitAirControl(subTimeTickRemaining, AirControlAccel, Hit, bCheckLandingSpot) * subTimeTickRemaining;

							// Only allow if not back in to first wall
							if (FVector::DotProduct(AirControlDeltaV, OldHitNormal) > 0.f)
							{
								Delta += (AirControlDeltaV * subTimeTickRemaining);
							}
						}

						// Compute velocity after deflection (only gravity component for RootMotion)
						if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
						{
							const FVector NewVelocity = (Delta / subTimeTickRemaining);
							//gdg todo
							Velocity = HasRootMotion() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
						}

						// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
						//gdg todo
						bool bDitch = ( (OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f) );
						SafeMoveUpdatedComponent( Delta, PawnRotation, true, Hit);
						if ( Hit.Time == 0 )
						{
							// if we are stuck then try to side step
							FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).GetSafeNormal2D();
							if ( SideDelta.IsNearlyZero() )
							{
								SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).GetSafeNormal();
							}
							SafeMoveUpdatedComponent( SideDelta, PawnRotation, true, Hit);
						}
							
						if ( bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0  )
						{
							remainingTime = 0.f;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}
						//gdg todo
						else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
						{
							// We might be in a virtual 'ditch' within our perch radius. This is rare.
							const FVector PawnLocation = CharacterOwner->GetActorLocation();
							const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
							const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
							if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
							{
								Velocity.X += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Y += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
								Delta = Velocity * timeTick;
								SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
							}
						}
					}
				}
			}
		}

		//if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
		if (Velocity.SizeSquared() <= KINDA_SMALL_NUMBER * 10.f)
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}
	}
}

//might not need this
bool UFallCharacterMovementComponent::SafeMoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult& OutHit)
{
	if (UpdatedComponent == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d SafeMove"), __LINE__ );
		OutHit.Reset(1.f);
		return false;
	}

	bool bMoveResult = MoveUpdatedComponent(Delta, NewRotation, bSweep, &OutHit);

	//	UE_LOG(LogTemp, Warning, TEXT("%d SafeMove, bMoveResult:%d"), __LINE__, bMoveResult );
	// Handle initial penetrations
	if (OutHit.bStartPenetrating && UpdatedComponent)
	{
		//UE_LOG(LogTemp, Warning, TEXT("%d SafeMove"), __LINE__ );//here
		const FVector RequestedAdjustment = GetPenetrationAdjustment(OutHit);
		if (ResolvePenetration(RequestedAdjustment, OutHit, NewRotation))
		{
		//UE_LOG(LogTemp, Warning, TEXT("%d SafeMove"), __LINE__ );//here
			// Retry original move
			bMoveResult = MoveUpdatedComponent(Delta, NewRotation, bSweep, &OutHit);
	//	UE_LOG(LogTemp, Warning, TEXT("%d SafeMove, bMoveResult:%d"), __LINE__, bMoveResult );
		}
	}

	return bMoveResult;
}
void UFallCharacterMovementComponent::StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc)
{
	/*
	Don't have source compiled to know where this is actually being called
	but might not matter since it seems the landing is that bad part.
	*/
	// start falling 
	const float DesiredDist = Delta.Size();
	const float ActualDist = (CharacterOwner->GetActorLocation() - subLoc).Size2D();
	remainingTime = (DesiredDist < KINDA_SMALL_NUMBER) 
					? 0.f
					: remainingTime + timeTick * (1.f - FMath::Min(1.f,ActualDist/DesiredDist));

	Velocity.Z = 0.f;			
	if ( IsMovingOnGround() )
	{
		// This is to catch cases where the first frame of PIE is executed, and the
		// level is not yet visible. In those cases, the player will fall out of the
		// world... So, don't set MOVE_Falling straight away.
		if ( !GIsEditor || (GetWorld()->HasBegunPlay() && (GetWorld()->GetTimeSeconds() >= 1.f)) )
		{
		UE_LOG(LogTemp, Warning, TEXT("%d Start falling "), __LINE__ );
			SetMovementMode(MOVE_Falling); //default behavior if script didn't change physics
		}
		else
		{
			// Make sure that the floor check code continues processing during this delay.
			bForceNextFloorCheck = true;
		}
	}
	StartNewPhysics(remainingTime,Iterations);
}
void UFallCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
	if( CharacterOwner && CharacterOwner->ShouldNotifyLanded(Hit) )
	{
		UE_LOG(LogTemp, Warning, TEXT("%d post landed"), __LINE__ );
		CharacterOwner->Landed(Hit);
	}
	if( IsFalling() )
	{
		UE_LOG(LogTemp, Warning, TEXT("%d post landed"), __LINE__ );
		SetPostLandedPhysics(Hit);
	}
	if (PathFollowingComp.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("%d post landed"), __LINE__ );
		//PathFollowingComp->OnLanded();
	}

	StartNewPhysics(remainingTime, Iterations);
}
/*
void UFallCharacterMovementComponent::SetPostLandedPhysics(const FHitResult& Hit)
{
	//this is written elsewhere, possibly better
	if( CharacterOwner )
	{
		if ( GetPhysicsVolume()->bWaterVolume && CanEverSwim() )
		{
			SetMovementMode(MOVE_Swimming);
		}
		else
		{
		UE_LOG(LogTemp, Warning, TEXT("%d post land phys "), __LINE__ );
			const FVector PreImpactAccel = Acceleration + (IsFalling() ? FVector(0.f, 0.f, GetGravityZ()) : FVector::ZeroVector);
			const FVector PreImpactVelocity = Velocity;
			//SetMovementMode(GetGroundMovementMode());
			ApplyImpactPhysicsForces(Hit, PreImpactAccel, PreImpactVelocity);
		}
	}
}*/

bool UFallCharacterMovementComponent::IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	// Skip some checks if penetrating. Penetration will be handled by the FindFloor call (using a smaller capsule)
	if (!Hit.bStartPenetrating)
	{
		// Reject unwalkable floor normals.
		if (!IsWalkable(Hit))
		{
			return false;
		}

		float PawnRadius, PawnHalfHeight;
		CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

		// Reject hits that are above our lower hemisphere (can happen when sliding down a vertical surface).
		const float LowerHemisphereZ = Hit.Location.Z - PawnHalfHeight + PawnRadius;
		//bolloxed up here
		/*
		UE_LOG(LogTemp, Warning, TEXT("%d checking landing spot. Imp:%f, Hemi:%f"), __LINE__ ,Hit.ImpactPoint.Z,LowerHemisphereZ);
		if (Hit.ImpactPoint.Z >= LowerHemisphereZ)
		{
			return false;
		}
		*/

		// Reject hits that are barely on the cusp of the radius of the capsule
		if (!IsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
		{
		UE_LOG(LogTemp, Warning, TEXT("%d checking landing spot"), __LINE__ );
			return false;
		}
	}

		UE_LOG(LogTemp, Warning, TEXT("%d checking landing spot"), __LINE__ );
	FFindFloorResult FloorResult;
	FindFloor(CapsuleLocation, FloorResult, false, &Hit);

	if (!FloorResult.IsWalkableFloor())
	{
		UE_LOG(LogTemp, Warning, TEXT("%d checking landing spot"), __LINE__ );
		return false;
	}

		UE_LOG(LogTemp, Warning, TEXT("%d checking landing spot"), __LINE__ );
	return true;
}

bool UFallCharacterMovementComponent::FloorSweepTest(
	FHitResult& OutHit,
	const FVector& Start,
	const FVector& End,
	ECollisionChannel TraceChannel,
	const struct FCollisionShape& CollisionShape,
	const struct FCollisionQueryParams& Params,
	const struct FCollisionResponseParams& ResponseParam
	) const
{
	bool bBlockingHit = false;
	if (!bUseFlatBaseForFloorChecks)
	{
		bBlockingHit = GetWorld()->SweepSingle(OutHit, Start, End, GetPawnOwner()->GetActorRotation().Quaternion() , TraceChannel, CollisionShape, Params, ResponseParam);
		//bBlockingHit = GetWorld()->SweepSingle(OutHit, Start, End, FQuat::Identity, TraceChannel, CollisionShape, Params, ResponseParam);
		UE_LOG(LogTemp, Warning, TEXT("%d Sweeping the Floor. BlockingHit?:%d, Start:%s, End:%s, Params:%s"), 
			__LINE__,bBlockingHit, *Start.ToString(), *End.ToString(), *Params.ToString() );	 //here
		
		//worse bBlockingHit = GetWorld()->SweepSingle(OutHit, Start, End, GravityDirection.Rotation().Quaternion(), TraceChannel, CollisionShape, Params, ResponseParam);
	// Sweeping the Floor.BlockingHit ? : 0, Start : X = -3316.911 Y = 8.088 Z = 3545.250, 
										    // End : X = -3268.557 Y = 7.970 Z = 3566.457, 
	//Params : [MyCharacter_C_80:ComputeFloorDistSweep] TraceAsync(0), TraceComplex(0)

/*	LogTemp : Warning : 1458 Sweeping the Floor.BlockingHit ? : 1, 
	Start : X = -3211.155 Y = 7.830 Z = 3591.634, 
	End : X = -3162.801 Y = 7.712 Z = 3612.841, 
	Params : [MyCharacter_C_82:ComputeFloorDistSweep] TraceAsync(0), TraceComplex(0)
			 */
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%d Sweeping the Floor"), __LINE__ );
		// Test with a box that is enclosed by the capsule.
		const float CapsuleRadius = CollisionShape.GetCapsuleRadius();
		const float CapsuleHeight = CollisionShape.GetCapsuleHalfHeight();
		const FCollisionShape BoxShape = FCollisionShape::MakeBox(FVector(CapsuleRadius * 0.707f, CapsuleRadius * 0.707f, CapsuleHeight));

		// First test with the box rotated so the corners are along the major axes (ie rotated 45 degrees).
		bBlockingHit = GetWorld()->SweepSingle(OutHit, Start, End, FQuat(FVector(0.f, 0.f, -1.f), PI * 0.25f), TraceChannel, BoxShape, Params, ResponseParam);

		if (!bBlockingHit)
		{
		UE_LOG(LogTemp, Warning, TEXT("%d Sweeping the Floor"), __LINE__ );
			// Test again with the same box, not rotated.
			OutHit.Reset(1.f, false);			
			bBlockingHit = GetWorld()->SweepSingle(OutHit, Start, End, FQuat::Identity, TraceChannel, BoxShape, Params, ResponseParam);
		}
	}

	return bBlockingHit;
}
