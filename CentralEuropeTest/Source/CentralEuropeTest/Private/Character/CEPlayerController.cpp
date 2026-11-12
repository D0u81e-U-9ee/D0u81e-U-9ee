#include "Character/CEPlayerController.h"

#include "CentralEuropeTest.h"
#include "Character/CEBaseCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "Input/CEInputSubsystem.h"
#include "InputActionValue.h"
#include "Interaction/CEInteractable.h"
#include "Interaction/CEVehicleSeatComponent.h"

void ACEPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	BindInteract();
}

void ACEPlayerController::BeginPlay()
{
	Super::BeginPlay();
	BindInteract();   // in case the local player was not assigned yet during SetupInputComponent
}

void ACEPlayerController::BindInteract()
{
	if (bInteractBound)
	{
		return;
	}
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	const UCEInputSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UCEInputSubsystem>() : nullptr;
	if (!EIC || !Input || !Input->IA_Interact)
	{
		return;
	}
	EIC->BindAction(Input->IA_Interact, ETriggerEvent::Started, this, &ACEPlayerController::Input_Interact);
	bInteractBound = true;
}

void ACEPlayerController::Input_Interact(const FInputActionValue& Value)
{
	if (IsInVehicle())
	{
		ExitVehicle();
	}
	else
	{
		TryInteract();
	}
}

void ACEPlayerController::TryInteract()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !GetWorld())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector Direction = ViewRotation.Vector();
	// Start level with the pawn so the third-person camera boom does not eat the reach.
	const float Along = FMath::Max(FVector::DotProduct(ControlledPawn->GetActorLocation() - ViewLocation, Direction), 0.f);
	const FVector Start = ViewLocation + Direction * Along;
	const FVector End = Start + Direction * InteractDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractTrace), false, ControlledPawn);
	FHitResult Hit;
	if (!GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(30.f), Params))
	{
		return;
	}
	AActor* Target = Hit.GetActor();
	if (!Target)
	{
		return;
	}

	if (Target->Implements<UCEInteractable>())
	{
		ICEInteractable::Execute_Interact(Target, ControlledPawn);
	}
	else if (UActorComponent* Component = Target->FindComponentByInterface(UCEInteractable::StaticClass()))
	{
		ICEInteractable::Execute_Interact(Component, ControlledPawn);
	}
}

void ACEPlayerController::EnterVehicle(APawn* Vehicle, UCEVehicleSeatComponent* Seat)
{
	ACEBaseCharacter* Driver = Cast<ACEBaseCharacter>(GetPawn());
	if (!Vehicle || !Driver || IsInVehicle())
	{
		return;
	}

	StoredCharacter = Driver;
	DrivenVehicle = Vehicle;
	CurrentSeat = Seat;

	Driver->OnEnterVehicle(Vehicle, Seat ? Seat->SeatSocket : NAME_None);
	Possess(Vehicle);

	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UCEInputSubsystem* Input = LocalPlayer->GetSubsystem<UCEInputSubsystem>())
		{
			Input->ApplyVehicleContexts();
		}
	}
	UE_LOG(LogCETest, Log, TEXT("Entered vehicle %s"), *Vehicle->GetName());
}

FVector ACEPlayerController::FindExitLocation() const
{
	const FTransform VehicleTransform = DrivenVehicle->GetActorTransform();
	static const TArray<FVector> DefaultOffsets = { FVector(0.f, -220.f, 60.f), FVector(0.f, 0.f, 260.f) };
	const TArray<FVector>& Offsets = (CurrentSeat && CurrentSeat->ExitOffsets.Num() > 0) ? CurrentSeat->ExitOffsets : DefaultOffsets;

	float Radius = 34.f, HalfHeight = 90.f;
	if (StoredCharacter && StoredCharacter->GetCapsuleComponent())
	{
		StoredCharacter->GetCapsuleComponent()->GetUnscaledCapsuleSize(Radius, HalfHeight);
	}
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(VehicleExit), false, DrivenVehicle);
	Params.AddIgnoredActor(StoredCharacter);

	FVector Fallback = VehicleTransform.GetLocation() + FVector(0.f, 0.f, 300.f);
	for (const FVector& Offset : Offsets)
	{
		// Yaw-only so a tilted or rolled vehicle still yields an upright, sideways exit.
		const FRotator YawOnly(0.f, VehicleTransform.Rotator().Yaw, 0.f);
		const FVector Candidate = VehicleTransform.GetLocation() + YawOnly.RotateVector(Offset) + FVector(0.f, 0.f, HalfHeight);
		Fallback = Candidate;
		if (!GetWorld()->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, ECC_Pawn, Capsule, Params))
		{
			return Candidate;
		}
	}
	return Fallback;
}

void ACEPlayerController::ExitVehicle()
{
	if (!IsInVehicle() || !StoredCharacter || !GetWorld())
	{
		return;
	}

	const FVector ExitLocation = FindExitLocation();
	const FRotator ExitRotation(0.f, DrivenVehicle->GetActorRotation().Yaw, 0.f);

	ACEBaseCharacter* Driver = StoredCharacter;
	DrivenVehicle = nullptr;
	CurrentSeat = nullptr;
	StoredCharacter = nullptr;

	Driver->OnExitVehicle(ExitLocation, ExitRotation);
	Possess(Driver);          // PawnClientRestart on the character restores the on-foot contexts
	SetControlRotation(ExitRotation);
	UE_LOG(LogCETest, Log, TEXT("Exited vehicle"));
}
