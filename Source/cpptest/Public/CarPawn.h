// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CarPawn.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVehicleData, Log, All);

UCLASS()
class CPPTEST_API ACarPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ACarPawn();
	FSocket* Socket; // Define the socket variable
	FTimerHandle TimerHandle;
	void sendSocketMessage();
	void FrameSender();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};