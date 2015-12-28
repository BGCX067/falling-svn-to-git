// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Fall.h"
#include "FallCharacter.h"
#include "FallProjectile.h"
#include "Animation/AnimInstance.h"
#include "FallCharacterMovementComponent.h"


//////////////////////////////////////////////////////////////////////////
// AFallCharacter

AFallCharacter::AFallCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFallCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
//	: Super(ObjectInitializer)
{


	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FirstPersonCamera"));
	//FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("CharacterMesh1P"));
	//Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("HeroTPP"));
	//Mesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT(""));
	Mesh1P->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
//	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->RelativeLocation = FVector(0.f, 0.f, -150.f);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->AttachParent = GetCapsuleComponent();
	FirstPersonCameraComponent->AttachParent = GetMesh();
	//FirstPersonCameraComponent->AttachParent = GetMesh1P();
		//GetFirstPersonCameraComponent();
//	Mesh1P->AttachParent = FirstPersonCameraComponent;
	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)
//Skeleton'/Game/AnimStarterPack/Character/HeroTPP_Skeleton.HeroTPP_Skeleton'
	Boner = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("HeroTPP_Skeleton"));
//	Boner->AttachParent(Mesh);
	
		UE_LOG(LogTemp, Warning, TEXT("Mesh Name: %s"), *GetMesh()->GetName() );


	/*
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
//	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("CharacterMesh1P"));
//	Mesh1P->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
//	Mesh1P->AttachParent =  GetCapsuleComponent();//gdg paragraph
//	Mesh1P->RelativeLocation = FVector(0.f, 0.f, -150.f);
//	Mesh1P->bCastDynamicShadow = false;
//	Mesh1P->CastShadow = false;
	//SkeletalMesh'/Game/AnimStarterPack/Character/HeroTPP.HeroTPP'

	//auto BoneIdx = SkeletalMeshComponent->GetBoneIndex(TEXT("SheKRArm2"));

	// Create a CameraComponent	
	FirstPersonCameraComponent = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FirstPersonCamera"));
//	FirstPersonCameraComponent->AttachParent = Mesh1P;
	FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	//Mesh1P->GetSocketByName(FName("headSocket"))->AttachActor(FirstPersonCameraComponent->GetAttachmentRootActor(), Mesh1P);
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f); // Position the camera
	//FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 0.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
//	Mesh1P->AttachParent =  FirstPersonCameraComponent;//gdg paragraph
//	Mesh1P->GetSocketByName(FName("Head"))->AttachActor(FirstPersonCameraComponent, )

	//auto Head = GetMesh1P()->GetBodyInstance(FName("Head"));
	//FirstPersonCameraComponent->AttachParent = Mesh1P->GetBodyInstance(FName("Head"));
	//Mesh1P->AnimBlueprintGeneratedClass->GetTargetSkeleton()->GetSock
	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);
	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->RelativeLocation = FVector(0.f, 0.f, -150.f);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	*/
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFallCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	
	InputComponent->BindAction("Fire", IE_Pressed, this, &AFallCharacter::OnFire);
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AFallCharacter::TouchStarted);

	InputComponent->BindAxis("MoveForward", this, &AFallCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AFallCharacter::MoveRight);
	
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick

	//That explanation makes no sense even though I know approximately what it is trying to say.

	InputComponent->BindAxis("Turn", this, &AFallCharacter::AddControllerYawInput);
	//InputComponent->BindAxis("Turn", this, &AFallCharacter::Turn);
	InputComponent->BindAxis("TurnRate", this, &AFallCharacter::TurnAtRate);//keyboard

	//InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);//mouse 

	InputComponent->BindAxis("LookUp", this, &AFallCharacter::LookUp);
	InputComponent->BindAxis("LookUpRate", this, &AFallCharacter::LookUpAtRate);//keyboard?
}

void AFallCharacter::OnFire()
{
	
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		const FRotator SpawnRotation = GetControlRotation();
		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(GunOffset);

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			// spawn the projectile at the muzzle
			World->SpawnActor<AFallProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if(FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if(AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

}

void AFallCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// only fire for first finger down
	if (FingerIndex == 0)
	{
		OnFire();
	}
}

void AFallCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AFallCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AFallCharacter::TurnAtRate(float Rate) //keyboard
{
	// calculate delta for this frame from the rate information
	//no effect AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	//no effedt AddControllerYawInput(Rate);

/* crashed here when using arrow keys	FirstPersonCameraComponent->AddLocalRotation(FRotator( 0,  -45.f * Rate * GetWorld()->GetDeltaSeconds(), 0));
	Mesh1P->AddLocalRotation(FRotator(0, -45.f * Rate * GetWorld()->GetDeltaSeconds(), 0));
	*/
}

void AFallCharacter::Turn(float Rate)
{
	FirstPersonCameraComponent->AddLocalRotation(FRotator( 0, 45.f * Rate * GetWorld()->GetDeltaSeconds(), 0));
	//InputComponent->BindAxis("Turn", this, &AFallCharacter::AddControllerYawInput);
	//AddControllerYawInput(Rate);
	//no effect AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());

//no crash or effect Boner->AddLocalRotation(FRotator(0, -45.f * Rate * GetWorld()->GetDeltaSeconds(), 0));
//crash	Mesh1P->AddLocalRotation(FRotator(0, -45.f * Rate * GetWorld()->GetDeltaSeconds(), 0));
//instacrash	Mesh1P->AddRelativeRotation(FRotator(0, -45.f * Rate * GetWorld()->GetDeltaSeconds(), 0));
//crash @1st move	
		//GetMesh()->AddWorldRotation(FRotator(0, -45.f * Rate * GetWorld()->GetDeltaSeconds(), 0));//nothing
		//GetCapsuleComponent()->AddWorldRotation(FRotator(0, -45.f * Rate * GetWorld()->GetDeltaSeconds(), 0));
	//	GetCapsuleComponent()->AddLocalRotation(FRotator(0, 90, 0));

	/*
		AController* ctrl = GetController();
		FTransform tranny = GetTransform();
		FRotator rot(0, Rate, 0);
		tranny.ConcatenateRotation(rot.Quaternion());
		//ClientSetRotation(FRotator( tranny.GetRotation()));
		//GetMesh1P()->SetRelativeTransform(tranny);
*/
}

void AFallCharacter::LookUpAtRate(float Rate)//don't know
{
	// calculate delta for this frame from the rate information
		//UE_LOG(LogTemp, Warning, TEXT("Pitch: %f"), Rate*360.0 );
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}


void AFallCharacter::LookUp(float Rate)//More like glance up--looks up then drops back
{
	//	UE_LOG(LogTemp, Warning, TEXT("Rate:%f"), Rate);
	FirstPersonCameraComponent->AddLocalRotation(FRotator(-45.f * Rate * GetWorld()->GetDeltaSeconds(), 0, 0));

	TArray<FName> boners;
	Boner->GetBoneNames(boners);
	if (boners.Num() > 0){
		for (auto &c : boners){
			UE_LOG(LogTemp, Warning, TEXT("name:%s"), *c.ToString());
		}
	}
	/*
	else {
			UE_LOG(LogTemp, Warning, TEXT("you don't have a boner, it is safe to stand"));
	}
	*/
	//Mesh1P->GetBoneNames(out array<name> BoneNames);
	//= ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FirstPersonCamera"));
//		UE_LOG(LogTemp, Warning, TEXT("index: %d"), headIdx);
	//UE_LOG(LogTemp, Warning, TEXT("mesh: %s"), *Mesh1P->Bodies[headIdx]->GetBodyDebugName());
	/*
		AController* ctrl = GetController();
		FTransform tranny = GetTransform();
		FRotator rot(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds(), 0, 0);
		tranny.ConcatenateRotation(rot.Quaternion());
		ClientSetRotation(FRotator( tranny.GetRotation()));
		*/
}
/*
void AFallCharacter::K2_UpdateCustomMovement(float DeltaTime)
{
//	UE_LOG(LogTemp, Warning, TEXT("UDATE CUSTOM"));
}
*/