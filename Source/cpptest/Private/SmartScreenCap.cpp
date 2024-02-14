// Fill out your copyright notice in the Description page of Project Settings.


#include "SmartScreenCap.h"
#include "HAL/PlatformFilemanager.h"
#include "GenericPlatform/GenericPlatformFile.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageUtils.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "TimerManager.h"
#include "Interfaces/IPv4/IPv4Address.h"

#include <stdlib.h>
#include <time.h>
#include <fstream>

DEFINE_LOG_CATEGORY(LogSmartCam);

// Sets default values
ASmartScreenCap::ASmartScreenCap()
: resolutionX(1024)
, resolutionY(1024)
, field_of_view(90.0f)
, outputFolderPath(TEXT("."))
, colorCameraTranslation(0.0f, 0.0f, 0.0f)
, colorCameraRotation(0., 0., 0., 1.)
, counterImage(0)
, baseFilenameDepth("dpfile")
, baseFilenameColor("texfile")
, basePathFolder("")
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Only tick once all updates regarding movement and physics have happened
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));


	OurCameraDepth = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewportCameraDepth"));
	OurCameraDepth->SetupAttachment(RootComponent);

	OurCameraColor = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewportCameraColor"));
	OurCameraColor->SetupAttachment(RootComponent);

	// Resolution has to be a power of 2. This code finds the lowest RxR resolution which has more pixel than set

	sceneCaptureDepth = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureDepth"));
	sceneCaptureDepth->SetupAttachment(OurCameraDepth);

	sceneCaptureColor = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureColor"));
	sceneCaptureColor->SetupAttachment(OurCameraColor);


	DataToSend.Reserve(ImageWidth * ImageHeight * NumChannels);
	//Anish: Lets also set up the network components here
		// Create the TCP socket
	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
	FIPv4Address IP(127, 0, 0, 1); // Localhost
	int32 Port = 7766;
	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Addr->SetIp(IP.Value);
	Addr->SetPort(Port);

	bool connected = Socket->Connect(*Addr);
	if (!connected)
	{
		// Handle connection error
	}
	else {
		//GetWorldTimerManager().SetTimer(TimerHandle, this, &ACarPawn::sendSocketMessage, 1.0f, false);
		UE_LOG(LogSmartCam, Warning, TEXT("Vehicle Init"));
		uint8 baseVal = 65;
		
		//DataToSend.Append((uint8*)&baseVal, sizeof(baseVal));

		//int32 BytesSent = 0;
		//bool successful = Socket->Send(DataToSend.GetData(), DataToSend.Num(), BytesSent);
	}

}



// Called when the game starts or when spawned
void ASmartScreenCap::BeginPlay()
{
	Super::BeginPlay();
	basePathFolder = std::string(TCHAR_TO_UTF8(*outputFolderPath));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectory(*outputFolderPath);

	// Go on with the file name
	if (basePathFolder.back() != '/')
		basePathFolder.append("/");

#pragma region Get_Resolution_Power_of_2
	// Resolution has to be a power of 2. This code finds the lowest RxR resolution which has equal or more pixel than set
	uint32_t higherX = resolutionX;

	higherX--;
	higherX |= higherX >> 1;
	higherX |= higherX >> 2;
	higherX |= higherX >> 4;
	higherX |= higherX >> 8;
	higherX |= higherX >> 16;
	higherX++;

	internResolutionX = higherX;

	uint32_t higherY = resolutionY;

	higherY--;
	higherY |= higherY >> 1;
	higherY |= higherY >> 2;
	higherY |= higherY >> 4;
	higherY |= higherY >> 8;
	higherY |= higherY >> 16;
	higherY++;

	internResolutionY = higherY;
#pragma endregion

	OurCameraDepth->FieldOfView = field_of_view;
	OurCameraColor->FieldOfView = field_of_view;
	sceneCaptureColor->FOVAngle = field_of_view;
	sceneCaptureDepth->FOVAngle = field_of_view;

	OurCameraColor->SetRelativeLocation(colorCameraTranslation);
	OurCameraColor->SetRelativeRotation(colorCameraRotation);

	renderTargetDepth = NewObject<UTextureRenderTarget2D>();
	renderTargetDepth->InitCustomFormat(internResolutionX, internResolutionY, EPixelFormat::PF_FloatRGBA, true);

	renderTargetDepth->UpdateResourceImmediate();

	renderTargetColor = NewObject<UTextureRenderTarget2D>();
	renderTargetColor->InitCustomFormat(internResolutionX, internResolutionY, EPixelFormat::PF_B8G8R8A8, true);

	renderTargetColor->UpdateResourceImmediate();

	sceneCaptureDepth->CaptureSource = SCS_SceneDepth;
	sceneCaptureDepth->TextureTarget = renderTargetDepth;
	sceneCaptureDepth->bCaptureEveryFrame = true;

	sceneCaptureColor->CaptureSource = SCS_FinalColorLDR;
	sceneCaptureColor->TextureTarget = renderTargetColor;
	sceneCaptureColor->bCaptureEveryFrame = true;

	//imageRendered = sceneCaptureDepth->TextureTarget->ConstructTexture2D(this, "CameraImage", EObjectFlags::RF_NoFlags, CTF_DeferCompression);

#pragma region Get_File_Name
// Temporary buffer
	char targetBuffer[10];

	std::ofstream metaData;

	// World location, as string
	std::string strPosX;
	std::string strPosY;
	std::string strPosZ;

	// World rotation, as string
	std::string strRotPitch;
	std::string strRotRoll;
	std::string strRotYaw;

	std::string strRotX;
	std::string strRotY;
	std::string strRotZ;
	std::string strRotW;


	// Field of view (same for both cameras)
	sprintf(targetBuffer, "%.3f", field_of_view);
	std::string fov = std::string(targetBuffer);

	// Get world location of Actor
	FVector location = OurCameraDepth->GetComponentLocation();
	sprintf(targetBuffer, "%.3f", location.X);
	strPosX = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", location.Y);
	strPosY = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", location.Z);
	strPosZ = std::string(targetBuffer);

	// Get yaw pitch roll of actor
	FRotator rotation = OurCameraDepth->GetComponentRotation();
	FQuat quaternion = rotation.Quaternion();

	sprintf(targetBuffer, "%.3f", quaternion.X);
	strRotX = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", quaternion.Y);
	strRotY = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", quaternion.Z);
	strRotZ = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", quaternion.W);
	strRotW = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", rotation.Pitch);
	strRotPitch = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", rotation.Roll);
	strRotRoll = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", rotation.Yaw);
	strRotYaw = std::string(targetBuffer);


	//sprintf(targetBuffer, "%.3f", tempPtr->frames_per_second);
	//std::string strFPS = std::string(targetBuffer);


	baseFilenameDepth = basePathFolder + std::string("image");
	baseFilenameDepth += std::string("_number_");

	location = OurCameraColor->GetComponentLocation();

	sprintf(targetBuffer, "%.3f", location.X);
	strPosX = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", location.Y);
	strPosY = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", location.Z);
	strPosZ = std::string(targetBuffer);

	// Get yaw pitch roll of actor
	rotation = OurCameraColor->GetComponentRotation();
	quaternion = rotation.Quaternion();

	sprintf(targetBuffer, "%.3f", quaternion.X);
	strRotX = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", quaternion.Y);
	strRotY = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", quaternion.Z);
	strRotZ = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", quaternion.W);
	strRotW = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", rotation.Pitch);
	strRotPitch = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", rotation.Roll);
	strRotRoll = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", rotation.Yaw);
	strRotYaw = std::string(targetBuffer);

	baseFilenameColor = basePathFolder + std::string("image");
	baseFilenameColor += std::string("_number_");


	// Get world location of Actor
	location = colorCameraTranslation;
	sprintf(targetBuffer, "%.3f", location.X);
	strPosX = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", location.Y);
	strPosY = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", location.Z);
	strPosZ = std::string(targetBuffer);

	// Get yaw pitch roll of actor
	quaternion = colorCameraRotation;

	sprintf(targetBuffer, "%.3f", quaternion.X);
	strRotX = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", quaternion.Y);
	strRotY = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", quaternion.Z);
	strRotZ = std::string(targetBuffer);

	sprintf(targetBuffer, "%.3f", quaternion.W);
	strRotW = std::string(targetBuffer);

	//Anish: We'll set the timer callback here at the start
	//GetWorldTimerManager().SetTimer(TimerHandle, this, &ACarPawn::FrameSender, 1.0f, true);
	
}




bool ASmartScreenCap::SafeSocketSend() {
	uint8 baseVal = 67;
	
	int32 BytesSent = 0;
	bool successful;
	//UE_LOG(LogSmartCam, Warning, TEXT("SafeSocketSend called"));
	if (Socket)
	{

		/*
		for (int32 i = 0; i < ImageWidth * ImageHeight * NumChannels; ++i) {
			uint8 RandomValue = FMath::RandRange(0, 255);
			DataToSend.Add(RandomValue);
		}
		*/
		DataToSend.Empty(ImageWidth * ImageHeight * NumChannels);
		auto RenderTargetResource = renderTargetColor->GameThread_GetRenderTargetResource();

		if (RenderTargetResource)
		{
			TArray<FColor> buffer8;
			RenderTargetResource->ReadPixels(buffer8);


			UE_LOG(LogSmartCam, Warning, TEXT("buffer is %d bytes"), buffer8.Num()*3);

			for (const FColor& Pixel : buffer8)
			{
				// Append R, G, B components
				DataToSend.Add(Pixel.R);
				DataToSend.Add(Pixel.G);
				DataToSend.Add(Pixel.B);
				// Ignoring the Alpha component
			}
		}

		/*
		

		*/
		ESocketConnectionState ConnectionState = Socket->GetConnectionState();
		switch (ConnectionState)
		{
		case SCS_NotConnected:
			UE_LOG(LogSmartCam, Warning, TEXT("smartCam: Socket not connected"));
			return false;
		case SCS_Connected:
			//DataToSend.Append((uint8*)&baseVal, sizeof(baseVal));
			UE_LOG(LogSmartCam, Warning, TEXT("DTS is %d bytes"), DataToSend.Num());
			successful = Socket->Send(DataToSend.GetData(), DataToSend.Num(), BytesSent);
			if (successful) {
				UE_LOG(LogSmartCam, Warning, TEXT("Data sent"));
				return true;
			}
			else {
				UE_LOG(LogSmartCam, Warning, TEXT("Error while sending"));
				return false;
			}

		case SCS_ConnectionError:
			UE_LOG(LogSmartCam, Warning, TEXT("Socket Connection Error"));
			return false;

		default:
			UE_LOG(LogSmartCam, Warning, TEXT("Unknown socket state"));
			return false;
		}

		//empty out the imagearray so its ready for the next round
		
	}
	else {
		UE_LOG(LogSmartCam, Warning, TEXT("SafeSocketSend couldn't get the socket"));
		return false;
	}


}


// Called every frame
void ASmartScreenCap::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	sCounter = std::to_string(counterImage);
	sCounter = std::string(6 - sCounter.length(), '0') + sCounter;
	//UE_LOG(LogSmartCam, Warning, TEXT("Tick"));

	//if (counterImage > 0)
	//{
	//	
	//	SaveTextureDepthmap();
	//	SaveTextureColor();
	//}
	ASmartScreenCap::SafeSocketSend();
	counterImage++;
}


void ASmartScreenCap::SaveTextureDepthmap()
{
	auto RenderTargetResource = renderTargetDepth->GameThread_GetRenderTargetResource();

	if (RenderTargetResource)
	{
		TArray<FFloat16Color> buffer16;
		RenderTargetResource->ReadFloat16Pixels(buffer16);

		std::string fileName = baseFilenameDepth;
		fileName += sCounter + std::string(".depth16");
		std::ofstream targetFileDepth(fileName, std::ofstream::binary);

		depthVector.resize(buffer16.Num());

		for (int32_t index = 0; index < buffer16.Num(); index++)
		{
			depthVector[index] = static_cast<uint16_t>(buffer16[index].R.GetFloat() * 10 + 0.5);
		}

		targetFileDepth.write(reinterpret_cast<char*>(depthVector.data()), depthVector.size() * sizeof(decltype(depthVector)::value_type));
		targetFileDepth.close();
	}
}

/*

void ASmartScreenCap::SaveTextureColor()
{
	auto RenderTargetResource = renderTargetColor->GameThread_GetRenderTargetResource();

	if (RenderTargetResource)
	{
		TArray<FColor> buffer8;
		RenderTargetResource->ReadPixels(buffer8);

		std::string fileName = "C:\\temp\\delete\\scrap\\fakeFilename";
		FString fileFName = FString(fileName.c_str());
		fileName += sCounter + std::string(".bgr8");
		std::ofstream targetFileColor(fileName, std::ofstream::binary);


		int32 numPixels = buffer8.Num();
		UE_LOG(LogSmartCam, Warning, TEXT("Number of Pixels: %d"), numPixels);

		UE_LOG(LogSmartCam, Warning, TEXT("%s"), *fileFName );
		targetFileColor.write(reinterpret_cast<char*>(buffer8.GetData()), buffer8.Num() * sizeof(FColor));
		targetFileColor.close();
	}
}
*/

void ASmartScreenCap::SaveTextureColor()
{
	auto RenderTargetResource = renderTargetColor->GameThread_GetRenderTargetResource();

	if (RenderTargetResource)
	{
		TArray<FColor> buffer8;
		RenderTargetResource->ReadPixels(buffer8);

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

		if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(buffer8.GetData(), buffer8.GetAllocatedSize(), renderTargetColor->GetSurfaceWidth(), renderTargetColor->GetSurfaceHeight(), ERGBFormat::BGRA, 8))
		{
			const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed();
			TArray<uint8> PNGData;
			PNGData.Append(CompressedData.GetData(), CompressedData.Num());

			// Convert std::string to FString
			FString CounterString = FString(UTF8_TO_TCHAR(sCounter.c_str()));

			// Create the filename
			FString FileName = FString::Printf(TEXT("view%s.png"), *CounterString);
			FString FilePath = FPaths::Combine(TEXT("C:/temp/delete/scrap/"), FileName);

			FFileHelper::SaveArrayToFile(PNGData, *FilePath);

			UE_LOG(LogSmartCam, Log, TEXT("Saved PNG to %s"), *FilePath);
		}
	}
}
