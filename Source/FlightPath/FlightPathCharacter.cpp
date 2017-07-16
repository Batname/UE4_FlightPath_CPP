// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FlightPathCharacter.h"
#include "FlightStopActor.h"

#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Curves/CurveFloat.h"


//////////////////////////////////////////////////////////////////////////
// AFlightPathCharacter

AFlightPathCharacter::AFlightPathCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)


	// Flight path logic
	FlightBoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("FlightBoxCollider"));
	FlightBoxCollider->SetupAttachment(RootComponent);
	FlightBoxCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FlightBoxCollider->SetBoxExtent(FVector(150.f));
	FlightBoxCollider->bGenerateOverlapEvents = true;
	FlightBoxCollider->SetCollisionResponseToAllChannels(ECR_Overlap);   

}

void AFlightPathCharacter::BeginPlay()
{
	Super::BeginPlay();

	FlightBoxCollider->OnComponentBeginOverlap.AddDynamic(this, &AFlightPathCharacter::OnFlightBoxColliderOverlap);
}

void AFlightPathCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// If the timeline has started, advance it by DeltaSeconds
	if (FlightTimeline.IsPlaying())
	{
		FlightTimeline.TickTimeline(DeltaSeconds);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFlightPathCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AFlightPathCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFlightPathCharacter::MoveRight);


	PlayerInputComponent->BindAction("NextFlightPath", IE_Pressed, this, &AFlightPathCharacter::NextFlightPathSelected);
	PlayerInputComponent->BindAction("PreviousFlightPath", IE_Released, this, &AFlightPathCharacter::PreviousFlightPathSelected);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AFlightPathCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFlightPathCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AFlightPathCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AFlightPathCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AFlightPathCharacter::OnResetVR);
}


void AFlightPathCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AFlightPathCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AFlightPathCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AFlightPathCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AFlightPathCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AFlightPathCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AFlightPathCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}


void AFlightPathCharacter::OnFlightBoxColliderOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<AFlightStopActor>())
	{
		ActiveFlightStopActor = Cast<AFlightStopActor>(OtherActor);
	}
}

void AFlightPathCharacter::UpdateFlightTimeline(class UCurveFloat* CurveFloatToBind)
{
	// Initialize a timeline
	FlightTimeline = FTimeline();

	FOnTimelineFloat ProgressFunction;

	// Bind the function that ticks the timeline
	ProgressFunction.BindUFunction(this, TEXT("TickTimeline"));

	FlightTimeline.AddInterpFloat(CurveFloatToBind, ProgressFunction);
	FlightTimeline.SetLooping(false);
	FlightTimeline.PlayFromStart();

	// Set the timeline's
	FlightTimeline.SetTimelineLengthMode(TL_LastKeyFrame);

	FOnTimelineEvent TimelineEvent;
	TimelineEvent.BindUFunction(this, TEXT("ResetActiveFlightStopActor"));
	FlightTimeline.SetTimelineFinishedFunc(TimelineEvent);
}

void AFlightPathCharacter::NextFlightPathSelected()
{	
	if (ActiveFlightStopActor)
	{
		ActiveSplineComponent = ActiveFlightStopActor->GetNextFlightSplineComp();
		UpdateFlightTimeline(ActiveFlightStopActor->GetNextFlightCurve());
	}
}

void AFlightPathCharacter::PreviousFlightPathSelected()
{
	if (ActiveFlightStopActor)
	{
		ActiveSplineComponent = ActiveFlightStopActor->GetPreviousFlightSplineComp();
		UpdateFlightTimeline(ActiveFlightStopActor->GetPreviousFlightCurve());
	}
}

void AFlightPathCharacter::TickTimeline(float Value)
{
	float SplineLenght = ActiveSplineComponent->GetSplineLength();

	// Get location based on the provided values the timeline
	// the reason we're multiplying value with  SplineLenght is basause all our designed curves is the UE4 editor have a timerange of 0 - X
	// Where X is the total flight time
	FVector NewLocation = ActiveSplineComponent->GetLocationAtDistanceAlongSpline(Value * SplineLenght, ESplineCoordinateSpace::World);

	SetActorLocation(NewLocation);

	FRotator NewRotation = ActiveSplineComponent->GetRotationAtDistanceAlongSpline(Value * SplineLenght, ESplineCoordinateSpace::World);

	// We're not intrested in the pitch of the above so we make  sure to set it to zero
	NewRotation.Pitch = 0;

	SetActorRotation(NewRotation);	
}

void AFlightPathCharacter::ResetActiveFlightStopActor()
{
	ActiveFlightStopActor = nullptr;
}