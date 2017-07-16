// Fill out your copyright notice in the Description page of Project Settings.


#include "FlightStopActor.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"


// Sets default values
AFlightStopActor::AFlightStopActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SM = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SM"));
	SetRootComponent(SM);

	// Init Spline
	NextFlightStop = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComp"));
	PreviousFlightStop = CreateDefaultSubobject<USplineComponent>(TEXT("PreviousFlightStop"));

	// Attach stop to root component
	NextFlightStop->SetupAttachment(SM);
	PreviousFlightStop->SetupAttachment(SM);

}

// Called when the game starts or when spawned
void AFlightStopActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFlightStopActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

