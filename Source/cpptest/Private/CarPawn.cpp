// Fill out your copyright notice in the Description page of Project Settings.

#include "CarPawn.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "TimerManager.h"
#include "Interfaces/IPv4/IPv4Address.h"

DEFINE_LOG_CATEGORY(LogVehicleData);


// Sets default values
ACarPawn::ACarPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
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
		UE_LOG(LogVehicleData, Warning, TEXT("Vehicle Init"));
		uint8 baseVal = 65;		
		TArray<uint8> DataToSend;
		DataToSend.Append((uint8*)&baseVal , sizeof(baseVal));

		int32 BytesSent = 0;
		bool successful = Socket->Send(DataToSend.GetData(), DataToSend.Num(), BytesSent);		
	}
}


ACarPawn::~ACarPawn()
{
	// Ensure the socket exists
	if (Socket)
	{
		// Close the socket
		Socket->Close();

		// Free the memory associated with the socket
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);

		// Nullify the pointer for safety
		Socket = nullptr;
	}
}

bool ACarPawn::SafeSocketSend() {
	uint8 baseVal = 67;
	TArray<uint8> DataToSend;
	int32 BytesSent = 0;
	bool successful;

	if (Socket)
	{
		ESocketConnectionState ConnectionState = Socket->GetConnectionState();
		switch (ConnectionState)
		{
			case SCS_NotConnected:
				UE_LOG(LogVehicleData, Warning, TEXT("Socket not connected"));
				return false;
			case SCS_Connected:
				DataToSend.Append((uint8*)&baseVal, sizeof(baseVal));
				successful = Socket->Send(DataToSend.GetData(), DataToSend.Num(), BytesSent);
				if (successful) {
					return true;
				} else {
					UE_LOG(LogVehicleData, Warning, TEXT("Error while sending"));
					return false;
				}
			
			case SCS_ConnectionError:
				UE_LOG(LogVehicleData, Warning, TEXT("Socket Connection Error"));
				return false;

			default:
				UE_LOG(LogVehicleData, Warning, TEXT("Unknown socket state"));
				return false;
		}
	}
	else {
		UE_LOG(LogVehicleData, Warning, TEXT("SafeSocketSend couldn't get the socket"));
		return false;
	}


}

// Called when the game starts or when spawned
void ACarPawn::BeginPlay()
{
	Super::BeginPlay();
	//GetWorldTimerManager().SetTimer(TimerHandle, this, &ACarPawn:FrameSender, 0.5f, false);
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ACarPawn::FrameSender, 1.0f, true);
}

void ACarPawn::FrameSender() {
		
		UE_LOG(LogVehicleData, Log, TEXT("Hello World"));
		ACarPawn::SafeSocketSend();

}
// Called every frame
void ACarPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACarPawn::sendSocketMessage() {
}

// Called to bind functionality to input
void ACarPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

