// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WatchGame.h"
#include "WatchGameCharacter.h"
#include "WatchGameProjectile.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/HUD.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AWatchGameCharacter

AWatchGameCharacter::AWatchGameCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	recording = false;
	recorded = false;
	playingBack = false;
	velocitiesIter = storedVelocities.begin();
	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->RelativeLocation = FVector(0.f, 0.f, -150.f);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	PrimaryActorTick.bCanEverTick = true;
	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

float AWatchGameCharacter::GetMult()
{
	return mult;
}

// Called every frame
void AWatchGameCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (playingBack)
	{
		UE_LOG(LogTemp, Warning, TEXT("PLAYING BACK VELOCITY: %s"), *velocitiesIter->ToString());
		FVector newVel = *velocitiesIter*DeltaTime*mult;
		LaunchCharacter(newVel-oldVelocity,false,false);
		++velocitiesIter;
		oldVelocity = newVel;
		if (velocitiesIter == storedVelocities.end())
		{
			playingBack = false;
			velocitiesIter = storedVelocities.begin();
			oldVelocity = FVector(0, 0, 0);
		}
	}
	if (recording)
	{	
		UE_LOG(LogTemp, Warning, TEXT("RECORDING VELOCITY: %s"), *(GetMovementComponent()->Velocity / DeltaTime).ToString());
		//TotalTime += DeltaTime;
		storedVelocities.push_back(GetMovementComponent()->Velocity/DeltaTime);
	}
	
}

//////////////////////////////////////////////////////////////////////////
// Input

void AWatchGameCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	InputComponent->BindAction("MultInc", IE_Pressed, this, &AWatchGameCharacter::IncreaseMult);
	InputComponent->BindAction("MultDec", IE_Pressed, this, &AWatchGameCharacter::DecreaseMult);
	
	//InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AWatchGameCharacter::TouchStarted);
	if( EnableTouchscreenMovement(InputComponent) == false )
	{
		InputComponent->BindAction("Fire", IE_Pressed, this, &AWatchGameCharacter::OnFire);
		InputComponent->BindAction("RMB", IE_Pressed, this, &AWatchGameCharacter::Playback);
	}
	
	InputComponent->BindAxis("MoveForward", this, &AWatchGameCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AWatchGameCharacter::MoveRight);
	
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AWatchGameCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AWatchGameCharacter::LookUpAtRate);
}

bool AWatchGameCharacter::IsRecording()
{
	return recording;
}

void AWatchGameCharacter::IncreaseMult()
{
	mult++;
}

void AWatchGameCharacter::DecreaseMult()
{
	mult--;
}

void AWatchGameCharacter::Playback()
{
	if (recorded && !recording)
		playingBack = true;
}

void AWatchGameCharacter::OnFire()
{ 
	if (!recording && !playingBack)
	{
		UE_LOG(LogTemp, Warning, TEXT("STARTING RECORDING"));
		//TotalTime = 0;
		InitialPos = GetActorLocation();
		recording = true;
		storedVelocities.clear();
		
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FINISHED RECORDING"));
		//AvgVelocity = (GetActorLocation() - InitialPos) / TotalTime;
		SetActorLocation(InitialPos);
		recording = false;
		recorded = true;
		velocitiesIter = storedVelocities.begin();
		oldVelocity = FVector(0, 0, 0);
	}
	//// try and fire a projectile
	//if (ProjectileClass != NULL)
	//{
	//	const FRotator SpawnRotation = GetControlRotation();
	//	// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
	//	const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(GunOffset);

	//	UWorld* const World = GetWorld();
	//	if (World != NULL)
	//	{
	//		// spawn the projectile at the muzzle
	//		World->SpawnActor<AWatchGameProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
	//	}
	//}

	//// try and play the sound if specified
	//if (FireSound != NULL)
	//{
	//	UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	//}

	//// try and play a firing animation if specified
	//if(FireAnimation != NULL)
	//{
	//	// Get the animation object for the arms mesh
	//	UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
	//	if(AnimInstance != NULL)
	//	{
	//		AnimInstance->Montage_Play(FireAnimation, 1.f);
	//	}
	//}

}

void AWatchGameCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if( TouchItem.bIsPressed == true )
	{
		return;
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AWatchGameCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	if( ( FingerIndex == TouchItem.FingerIndex ) && (TouchItem.bMoved == false) )
	{
		OnFire();
	}
	TouchItem.bIsPressed = false;
}

void AWatchGameCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if ((TouchItem.bIsPressed == true) && ( TouchItem.FingerIndex==FingerIndex))
	{
		if (TouchItem.bIsPressed)
		{
			if (GetWorld() != nullptr)
			{
				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
				if (ViewportClient != nullptr)
				{
					FVector MoveDelta = Location - TouchItem.Location;
					FVector2D ScreenSize;
					ViewportClient->GetViewportSize(ScreenSize);
					FVector2D ScaledDelta = FVector2D( MoveDelta.X, MoveDelta.Y) / ScreenSize;									
					if (ScaledDelta.X != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.X * BaseTurnRate;
						AddControllerYawInput(Value);
					}
					if (ScaledDelta.Y != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.Y* BaseTurnRate;
						AddControllerPitchInput(Value);
					}
					TouchItem.Location = Location;
				}
				TouchItem.Location = Location;
			}
		}
	}
}

void AWatchGameCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AWatchGameCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AWatchGameCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AWatchGameCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AWatchGameCharacter::EnableTouchscreenMovement(class UInputComponent* InputComponent)
{
	bool bResult = false;
	if(FPlatformMisc::GetUseVirtualJoysticks() || GetDefault<UInputSettings>()->bUseMouseForTouch )
	{
		bResult = true;
		InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AWatchGameCharacter::BeginTouch);
		InputComponent->BindTouch(EInputEvent::IE_Released, this, &AWatchGameCharacter::EndTouch);
		InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AWatchGameCharacter::TouchUpdate);
	}
	return bResult;
}
