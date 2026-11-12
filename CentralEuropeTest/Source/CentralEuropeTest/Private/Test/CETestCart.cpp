#include "Test/CETestCart.h"

#include "Audio/CEAudioSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/CEInputSubsystem.h"
#include "InputActionValue.h"
#include "Interaction/CEVehicleSeatComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const float RideHeight = 70.f;    // body centre above the ground
	const float Gravity = 980.f;
}

ACETestCart::ACETestCart()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
	Body->SetBoxExtent(FVector(200.f, 90.f, 50.f));
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Body);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Body);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		BodyMesh->SetStaticMesh(Cube.Object);
	}
	BodyMesh->SetRelativeScale3D(FVector(4.f, 1.8f, 1.f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Body);
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	CameraBoom->TargetArmLength = 750.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	Seat = CreateDefaultSubobject<UCEVehicleSeatComponent>(TEXT("Seat"));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ACETestCart::BeginPlay()
{
	Super::BeginPlay();
	if (UCEAudioSubsystem* Audio = UCEAudioSubsystem::Get(this))
	{
		EngineLoop = Audio->StartLoopAttached(TEXT("Cart_Engine"), Body, 9000.f, 0.f);
	}
}

void ACETestCart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	const UCEInputSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UCEInputSubsystem>() : nullptr;
	if (!EIC || !Input)
	{
		return;
	}
	EIC->BindAction(Input->IA_VehicleThrottle, ETriggerEvent::Triggered, this, &ACETestCart::Input_Throttle);
	EIC->BindAction(Input->IA_VehicleThrottle, ETriggerEvent::Completed, this, &ACETestCart::Input_ThrottleEnd);
	EIC->BindAction(Input->IA_VehicleSteer, ETriggerEvent::Triggered, this, &ACETestCart::Input_Steer);
	EIC->BindAction(Input->IA_VehicleSteer, ETriggerEvent::Completed, this, &ACETestCart::Input_SteerEnd);
	EIC->BindAction(Input->IA_VehicleBrake, ETriggerEvent::Started, this, &ACETestCart::Input_BrakeStart);
	EIC->BindAction(Input->IA_VehicleBrake, ETriggerEvent::Completed, this, &ACETestCart::Input_BrakeEnd);
	EIC->BindAction(Input->IA_VehicleLook, ETriggerEvent::Triggered, this, &ACETestCart::Input_Look);
}

void ACETestCart::UnPossessed()
{
	Super::UnPossessed();
	Throttle = 0.f;
	Steer = 0.f;
	bBraking = true;   // parking brake until someone drives again
}

void ACETestCart::Input_Throttle(const FInputActionValue& Value)    { Throttle = FMath::Clamp(Value.Get<float>(), -1.f, 1.f); bBraking = false; }
void ACETestCart::Input_ThrottleEnd(const FInputActionValue& Value) { Throttle = 0.f; }
void ACETestCart::Input_Steer(const FInputActionValue& Value)       { Steer = FMath::Clamp(Value.Get<float>(), -1.f, 1.f); }
void ACETestCart::Input_SteerEnd(const FInputActionValue& Value)    { Steer = 0.f; }
void ACETestCart::Input_BrakeStart(const FInputActionValue& Value)  { bBraking = true; }
void ACETestCart::Input_BrakeEnd(const FInputActionValue& Value)    { bBraking = false; }

void ACETestCart::Input_Look(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}
	const FVector2D Look = Value.Get<FVector2D>();
	FRotator Rotation = Controller->GetControlRotation();
	Rotation.Yaw += Look.X;
	Rotation.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch) + Look.Y, -70.f, 30.f);
	Controller->SetControlRotation(Rotation);
}

bool ACETestCart::TraceGround(const FVector& At, FHitResult& OutHit) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CartGround), false, this);
	return GetWorld()->LineTraceSingleByChannel(OutHit, At + FVector(0.f, 0.f, 250.f), At - FVector(0.f, 0.f, 600.f), ECC_WorldStatic, Params);
}

void ACETestCart::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld())
	{
		return;
	}

	const float TopSpeed = bHeavyTruck ? 1530.f : MaxSpeed;
	const float Accel = bHeavyTruck ? DriveAcceleration * 0.25f : DriveAcceleration;

	FVector Location = GetActorLocation();
	FRotator Rotation = GetActorRotation();

	// Ground under the cart: gives the slope the drive force has to work against.
	FHitResult Ground;
	const bool bOnGround = TraceGround(Location, Ground);
	const FVector GroundNormal = bOnGround ? Ground.ImpactNormal : FVector::UpVector;
	const FVector Forward2D = FRotator(0.f, Rotation.Yaw, 0.f).Vector();
	// Slope along the heading: positive = uphill.
	const float SlopeAlong = -FVector::DotProduct(GroundNormal, Forward2D) / FMath::Max(GroundNormal.Z, 0.1f);
	GradePercent = SlopeAlong * 100.f;

	// Longitudinal dynamics: drive (fades toward top speed), gravity along the slope, rolling drag, brake.
	const float Direction = Speed >= 0.f ? 1.f : -1.f;
	float Acceleration = Throttle * Accel * FMath::Clamp(1.f - FMath::Abs(Speed) / TopSpeed, 0.f, 1.f);
	Acceleration -= Gravity * SlopeAlong / FMath::Sqrt(1.f + SlopeAlong * SlopeAlong);
	Acceleration -= Direction * (40.f + 0.02f * FMath::Abs(Speed));
	Speed += Acceleration * DeltaSeconds;
	if (bBraking || (FMath::IsNearlyZero(Throttle) && FMath::Abs(Speed) < 60.f && FMath::Abs(SlopeAlong) < 0.12f))
	{
		const float Brake = BrakeDeceleration * DeltaSeconds;
		Speed = FMath::Abs(Speed) <= Brake ? 0.f : Speed - Direction * Brake;
	}
	Speed = FMath::Clamp(Speed, -TopSpeed * 0.3f, TopSpeed);

	// Steering: needs motion, tightest at low speed.
	const float SpeedFactor = FMath::Clamp(FMath::Abs(Speed) / 300.f, 0.f, 1.f) * FMath::GetMappedRangeValueClamped(FVector2f(300.f, 2500.f), FVector2f(1.f, 0.35f), FMath::Abs(Speed));
	Rotation.Yaw += Steer * TurnRate * SpeedFactor * Direction * DeltaSeconds;

	// Move, then follow the ground at the new position.
	const FVector Step = FRotator(0.f, Rotation.Yaw, 0.f).Vector() * Speed * DeltaSeconds;
	FHitResult NextGround;
	if (!Step.IsNearlyZero() && TraceGround(Location + Step, NextGround))
	{
		const float ClimbAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(NextGround.ImpactNormal.Z, 0.f, 1.f)));
		const bool bTooSteepUphill = ClimbAngle > MaxClimbAngle && NextGround.ImpactPoint.Z > Location.Z - RideHeight;

		// Obstacle check a little above the ground so the slope itself does not count.
		FHitResult Obstacle;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CartObstacle), false, this);
		const FVector Lift(0.f, 0.f, 45.f);
		const bool bBlocked = GetWorld()->SweepSingleByChannel(Obstacle, Location + Lift, NextGround.ImpactPoint + FVector(0.f, 0.f, RideHeight) + Lift,
			FRotator(0.f, Rotation.Yaw, 0.f).Quaternion(), ECC_Vehicle, FCollisionShape::MakeBox(FVector(190.f, 80.f, 30.f)), Params);

		if (bTooSteepUphill || bBlocked)
		{
			Speed = 0.f;
		}
		else
		{
			Location = NextGround.ImpactPoint + FVector(0.f, 0.f, RideHeight);
			// Tilt with the terrain.
			const FVector Normal = NextGround.ImpactNormal;
			const FVector Fwd = FVector::VectorPlaneProject(FRotator(0.f, Rotation.Yaw, 0.f).Vector(), Normal).GetSafeNormal();
			const FRotator Target = FRotationMatrix::MakeFromXZ(Fwd, Normal).Rotator();
			Rotation.Pitch = FMath::FInterpTo(Rotation.Pitch, Target.Pitch, DeltaSeconds, 8.f);
			Rotation.Roll = FMath::FInterpTo(Rotation.Roll, Target.Roll, DeltaSeconds, 8.f);
		}
	}
	else if (bOnGround)
	{
		Location.Z = FMath::FInterpTo(Location.Z, Ground.ImpactPoint.Z + RideHeight, DeltaSeconds, 10.f);
	}

	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);

	// Engine note: silent when parked and empty, idle when occupied, rising with speed and load.
	if (EngineLoop)
	{
		const bool bOccupied = GetController() != nullptr;
		const float SpeedRatio = FMath::Clamp(FMath::Abs(Speed) / TopSpeed, 0.f, 1.f);
		const float Load = FMath::Abs(Throttle);
		EngineLoop->SetPitchMultiplier((bHeavyTruck ? 0.55f : 0.75f) + SpeedRatio * (bHeavyTruck ? 0.7f : 1.1f) + Load * 0.12f);
		EngineLoop->SetVolumeMultiplier(bOccupied || SpeedRatio > 0.02f ? 0.35f + 0.45f * FMath::Max(SpeedRatio, Load) : 0.001f);
	}

	if (GEngine && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, FColor::Cyan,
			FString::Printf(TEXT("%s  %.0f km/h   grade %+.1f %%   W/S throttle  A/D steer  Space brake  E exit"),
				bHeavyTruck ? TEXT("TRUCK profile") : TEXT("Cart"), GetSpeedKmh(), GradePercent));
	}
}
