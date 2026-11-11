#include "Character/BaseCharacter.h"

#include "CentralEuropeTest.h"
#include "Camera/CameraComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/PrototypeInputSubsystem.h"
#include "InputActionValue.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 90.f);

	// Shooter style: the body follows the camera yaw.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	Movement->MaxSwimSpeed = SwimSpeed;
	Movement->JumpZVelocity = 420.f;
	Movement->AirControl = 0.25f;
	Movement->BrakingDecelerationWalking = 2048.f;
	Movement->SetCrouchedHalfHeight(60.f);
	Movement->NavAgentProps.bCanCrouch = true;
	Movement->NavAgentProps.bCanSwim = true;
	Movement->bCanWalkOffLedgesWhenCrouching = true;

	// Mannequin alignment (mesh asset is assigned in the BP child / editor).
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 280.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);   // over the right shoulder
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(DefaultFOV);

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	ConfigureHitCollision();
	UpdateLocomotionState();
}

void ABaseCharacter::ConfigureHitCollision()
{
	// With a skinned mesh + physics asset the mesh bodies are the hit target (bone names, headshots)
	// and the capsule ignores combat channels to avoid double hits. Without one, the capsule is the target.
	USkeletalMeshComponent* SkelMesh = GetMesh();
	const bool bMeshIsTarget = SkelMesh && SkelMesh->GetSkeletalMeshAsset() && SkelMesh->GetPhysicsAsset();
	const ECollisionResponse CapsuleResponse = bMeshIsTarget ? ECR_Ignore : ECR_Block;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->SetCollisionResponseToChannel(ECC_Weapon, CapsuleResponse);
	Capsule->SetCollisionResponseToChannel(ECC_Melee, CapsuleResponse);
	Capsule->SetCollisionResponseToChannel(ECC_Projectile, CapsuleResponse);

	if (bMeshIsTarget)
	{
		SkelMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SkelMesh->SetCollisionResponseToChannel(ECC_Weapon, ECR_Block);
		SkelMesh->SetCollisionResponseToChannel(ECC_Melee, ECR_Block);
		SkelMesh->SetCollisionResponseToChannel(ECC_Projectile, ECR_Block);
	}
}

// ---------------------------------------------------------------- input

void ABaseCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (UPrototypeInputSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UPrototypeInputSubsystem>() : nullptr)
	{
		Input->ApplyOnFootContexts();
	}
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	const UPrototypeInputSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UPrototypeInputSubsystem>() : nullptr;
	if (!EIC || !Input)
	{
		UE_LOG(LogCETest, Warning, TEXT("%s: Enhanced Input not available (check DefaultInput.ini input classes)."), *GetName());
		return;
	}

	// IMC_Default
	EIC->BindAction(Input->IA_Move, ETriggerEvent::Triggered, this, &ABaseCharacter::Input_Move);
	EIC->BindAction(Input->IA_Move, ETriggerEvent::Completed, this, &ABaseCharacter::Input_MoveCompleted);
	EIC->BindAction(Input->IA_Look, ETriggerEvent::Triggered, this, &ABaseCharacter::Input_Look);
	EIC->BindAction(Input->IA_Jump, ETriggerEvent::Started, this, &ABaseCharacter::Input_JumpStarted);
	EIC->BindAction(Input->IA_Jump, ETriggerEvent::Completed, this, &ABaseCharacter::Input_JumpCompleted);
	EIC->BindAction(Input->IA_Sprint, ETriggerEvent::Started, this, &ABaseCharacter::Input_SprintStarted);
	EIC->BindAction(Input->IA_Sprint, ETriggerEvent::Completed, this, &ABaseCharacter::Input_SprintCompleted);
	EIC->BindAction(Input->IA_Crouch, ETriggerEvent::Started, this, &ABaseCharacter::Input_Crouch);
	// IA_Interact is bound on APrototypePlayerController (it must keep working inside vehicles).

	// IMC_Combat
	EIC->BindAction(Input->IA_Aim, ETriggerEvent::Started, this, &ABaseCharacter::Input_AimStarted);
	EIC->BindAction(Input->IA_Aim, ETriggerEvent::Completed, this, &ABaseCharacter::Input_AimCompleted);
	EIC->BindAction(Input->IA_Fire, ETriggerEvent::Started, this, &ABaseCharacter::Input_FireStarted);
	EIC->BindAction(Input->IA_Fire, ETriggerEvent::Completed, this, &ABaseCharacter::Input_FireCompleted);
	EIC->BindAction(Input->IA_Reload, ETriggerEvent::Started, this, &ABaseCharacter::Input_Reload);
	EIC->BindAction(Input->IA_WeaponSlot, ETriggerEvent::Started, this, &ABaseCharacter::Input_WeaponSlot);
}

void ABaseCharacter::Input_Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}
	const FRotator ControlRotation = Controller->GetControlRotation();
	if (GetCharacterMovement()->IsSwimming())
	{
		// Swim where the camera looks (allows diving / surfacing).
		AddMovementInput(ControlRotation.Vector(), MoveInput.Y);
		AddMovementInput(FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y), MoveInput.X);
		return;
	}
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MoveInput.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MoveInput.X);
}

void ABaseCharacter::Input_MoveCompleted(const FInputActionValue& Value)
{
	MoveInput = FVector2D::ZeroVector;
}

void ABaseCharacter::Input_Look(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}
	// Applied straight to the control rotation: independent of the legacy input scale settings.
	const FVector2D Look = Value.Get<FVector2D>();
	const float Sensitivity = LookSensitivity * (CombatComponent->IsAiming() ? 0.55f : 1.f);
	FRotator Rotation = Controller->GetControlRotation();
	Rotation.Yaw += Look.X * Sensitivity;
	Rotation.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch) + Look.Y * Sensitivity, -85.f, 85.f);
	Controller->SetControlRotation(Rotation);
}

void ABaseCharacter::Input_JumpStarted(const FInputActionValue& Value)
{
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
		return;
	}
	Jump();
}

void ABaseCharacter::Input_JumpCompleted(const FInputActionValue& Value)
{
	StopJumping();
}

void ABaseCharacter::Input_SprintStarted(const FInputActionValue& Value)
{
	bWantsSprint = true;
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
	}
}

void ABaseCharacter::Input_SprintCompleted(const FInputActionValue& Value)
{
	bWantsSprint = false;
}

void ABaseCharacter::Input_Crouch(const FInputActionValue& Value)
{
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
	}
	else if (GetCharacterMovement()->IsMovingOnGround())
	{
		Crouch();
	}
}

void ABaseCharacter::Input_AimStarted(const FInputActionValue& Value)   { CombatComponent->SetAiming(true); }
void ABaseCharacter::Input_AimCompleted(const FInputActionValue& Value) { CombatComponent->SetAiming(false); }
void ABaseCharacter::Input_FireStarted(const FInputActionValue& Value)  { CombatComponent->StartFire(); }
void ABaseCharacter::Input_FireCompleted(const FInputActionValue& Value){ CombatComponent->StopFire(); }
void ABaseCharacter::Input_Reload(const FInputActionValue& Value)       { CombatComponent->Reload(); }

void ABaseCharacter::Input_WeaponSlot(const FInputActionValue& Value)
{
	const int32 Slot = FMath::RoundToInt(Value.Get<float>());
	if (Slot >= 1 && Slot <= 4)
	{
		CombatComponent->EquipSlot(Slot);
	}
}

// ---------------------------------------------------------------- locomotion

void ABaseCharacter::UpdateLocomotionState()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	ELocomotionState NewState = ELocomotionState::Walking;
	if (bInVehicle)
	{
		NewState = ELocomotionState::InVehicle;
	}
	else if (Movement->IsSwimming())
	{
		NewState = ELocomotionState::Swimming;
	}
	else if (GetCharacterMovement()->IsCrouching())
	{
		NewState = ELocomotionState::Crouching;
	}
	else if (bWantsSprint && MoveInput.Y > 0.1f && !CombatComponent->IsAiming() && Movement->IsMovingOnGround())
	{
		NewState = ELocomotionState::Sprinting;   // forward only, never while crouched / aiming / swimming
	}
	LocomotionState = NewState;

	float TargetSpeed = WalkSpeed;
	if (NewState == ELocomotionState::Sprinting)
	{
		TargetSpeed = SprintSpeed;
	}
	else if (CombatComponent->IsAiming())
	{
		TargetSpeed = AimWalkSpeed;
	}
	Movement->MaxWalkSpeed = TargetSpeed;
	Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	Movement->MaxSwimSpeed = SwimSpeed;
}

void ABaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateLocomotionState();

	float TargetFOV = DefaultFOV;
	if (CombatComponent->IsAiming())
	{
		TargetFOV = CombatComponent->GetCurrentWeaponData().WeaponType == EWeaponType::Sniper ? SniperAimFOV : AimFOV;
	}
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaSeconds, 12.f));

	if (bShowDebugHud && IsLocallyControlled())
	{
		DrawDebugHud();
	}
}

void ABaseCharacter::DrawDebugHud() const
{
	if (!GEngine)
	{
		return;
	}
	const FWeaponData& Weapon = CombatComponent->GetCurrentWeaponData();
	const UEnum* StateEnum = StaticEnum<ELocomotionState>();
	const FString State = StateEnum ? StateEnum->GetNameStringByValue(static_cast<int64>(LocomotionState)) : FString();
	const FString Ammo = Weapon.MagSize > 0
		? FString::Printf(TEXT("%d / %d%s"), CombatComponent->GetAmmoInMag(), Weapon.MagSize, CombatComponent->IsReloading() ? TEXT("  RELOADING") : TEXT(""))
		: FString(TEXT("-"));
	const FString Text = FString::Printf(TEXT("[%d] %s   ammo %s   |  %s  %.0f cm/s   HP %.0f\n1-4 weapons  LMB fire  RMB aim  R reload  Shift sprint  C crouch  E interact"),
		CombatComponent->GetCurrentSlot(), *Weapon.WeaponName.ToString(), *Ammo, *State, GetVelocity().Size(), HealthComponent->GetHealth());
	GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, FColor::White, Text);

	// Minimal crosshair.
	GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()) + 1, 0.f, FColor::Green,
		CombatComponent->IsAiming() ? TEXT("aiming") : TEXT(""));
}

// ---------------------------------------------------------------- vehicles

void ABaseCharacter::OnEnterVehicle(APawn* Vehicle, FName SeatSocket)
{
	bInVehicle = true;
	bWantsSprint = false;
	MoveInput = FVector2D::ZeroVector;
	CombatComponent->SetAttacksBlocked(true);   // holsters and rejects fire while driving

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	if (Vehicle && Vehicle->GetRootComponent())
	{
		AttachToComponent(Vehicle->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SeatSocket);
	}
	UpdateLocomotionState();
}

void ABaseCharacter::OnExitVehicle(const FVector& ExitLocation, const FRotator& ExitRotation)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocationAndRotation(ExitLocation, ExitRotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);   // settles to walking / swimming on its own

	bInVehicle = false;
	CombatComponent->SetAttacksBlocked(false);
	UpdateLocomotionState();
}
