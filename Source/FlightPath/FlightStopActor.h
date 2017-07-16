// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlightStopActor.generated.h"

UCLASS()
class FLIGHTPATH_API AFlightStopActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFlightStopActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	/** The flightcurve corresponding to the previous flight spline component */
	UPROPERTY(EditAnywhere, Category = CurveFloat)
	class UCurveFloat* PreviousFlightCurve;

	/** The FlighCurve correspondint to the newxt flight spline component */
	UPROPERTY(EditAnywhere, Category = CurveFloat)
	class UCurveFloat* NextFlightCurve;

	/** A static mesh for our flight stop */
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* SM;

	/** The spline component that discribes the flight path of the flight */
	UPROPERTY(VisibleAnywhere)
	class USplineComponent* NextFlightStop;

	/** The spline component that describes the flight path of the previous flight */
	UPROPERTY(VisibleAnywhere)
	class USplineComponent* PreviousFlightStop;

public:
	/** Sets default values for this actor's properties */
	class UCurveFloat* GetPreviousFlightCurve() { return PreviousFlightCurve; }

	/** Returns the next flight curve */
	class UCurveFloat* GetNextFlightCurve() { return NextFlightCurve; }

	/** Returns the next flight spline component */
	class USplineComponent* GetNextFlightSplineComp() { return NextFlightStop; }

	/** Returns the previous flight spline component */
	class USplineComponent* GetPreviousFlightSplineComp() { return PreviousFlightStop; }
};
