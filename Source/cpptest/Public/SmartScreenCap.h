// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include <string>
#include <memory>
#include <vector>

#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Actor.h"
#include "SmartScreenCap.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSmartCam, Log, All);


UCLASS()
class CPPTEST_API ASmartScreenCap : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASmartScreenCap();
	FSocket* Socket; // Define the socket variable
	FTimerHandle TimerHandle;
	//Anish: I'll reserve space for the image frame here
	const int32 ImageWidth = 1024;
	const int32 ImageHeight = 1024;
	const int32 NumChannels = 3;

	TArray<uint8> DataToSend;
	bool SafeSocketSend();

protected:

	uint32_t internResolutionX; // Textures need to be power of 2
	uint32_t internResolutionY; // Textures need to be power of 2

	// Current counter of image
	uint32_t counterImage;

	// Complete compacted base filename
	std::string baseFilenameDepth;
	std::string baseFilenameColor;
	std::string basePathFolder;
	std::string sCounter;

	UTextureRenderTarget2D* renderTargetDepth;
	class USceneCaptureComponent2D* sceneCaptureDepth;
	class UCameraComponent* OurCameraDepth;

	UTextureRenderTarget2D* renderTargetColor;
	class USceneCaptureComponent2D* sceneCaptureColor;
	class UCameraComponent* OurCameraColor;

	std::vector<uint16_t> depthVector;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void SaveTextureDepthmap();
	void SaveTextureColor();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Output Information", meta = (ClampMin = "32", ClampMax = "4096", UIMin = "32", UIMax = "4096"))
	uint32 resolutionX;

	UPROPERTY(EditAnywhere, Category = "Output Information", meta = (ClampMin = "32", ClampMax = "4096", UIMin = "32", UIMax = "4096"))
	uint32 resolutionY;

	UPROPERTY(EditAnywhere, Category = "Output Information", meta = (ClampMin = "20.0", ClampMax = "170.0", UIMin = "20.0", UIMax = "179.9"))
	float field_of_view;

	UPROPERTY(EditAnywhere, Category = "Output Information")
	FString outputFolderPath;

	UPROPERTY(EditAnywhere, Category = "Stereo Setup")
	FVector colorCameraTranslation;

	UPROPERTY(EditAnywhere, Category = "Stereo Setup")
	FQuat colorCameraRotation;
};
