#include "Character/CEBaseCharacter.h"

#include "CentralEuropeTest.h"
#include "Camera/CameraComponent.h"
#include "Character/CEBlockMannequinComponent.h"
#include "Combat/CECombatComponent.h"
#include "Combat/CEHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/CEInputSubsystem.h"
#include "InputActionValue.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"

ACEBaseCharacter::ACEBaseCharacter()
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

	CombatComponent = CreateDefaultSubobject<UCECombatComponent>(TEXT("CombatComponent"));
	HealthComponent = CreateDefaultSubobject<UCEHealthComponent>(TEXT("HealthComponent"));

	BlockMannequin = CreateDefaultSubobject<UCEBlockMannequinComponent>(TEXT("BlockMannequin"));
	BlockMannequin->SetupAttachment(GetRootComponent());

	// UE5 Mannequin locations in the Third Person content pack (5.5 layout first, then the newer one).
	MannequinMeshCandidates = {
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")),
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny")),
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")),
	};
	MannequinAnimClassCandidates = {
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C")),
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")),
	};
}

void ACEBaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Runs before any component BeginPlay, so the combat component sees the final body.
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		TryAutoAssignMannequin();
		const bool bHasSkeletalBody = GetMesh() && GetMesh()->GetSkeletalMeshAsset();
		BlockMannequin->SetMannequinEnabled(!bHasSkeletalBody);
	}
}

void ACEBaseCharacter::TryAutoAssignMannequin()
{
	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (!SkelMesh || SkelMesh->GetSkeletalMeshAsset())
	{
		return;   // a Blueprint child already assigned one
	}

	auto LoadIfPresent = [](const FSoftObjectPath& Path) -> UObject*
	{
		// Check the package first so a missing content pack does not spam load errors.
		return (Path.IsValid() && FPackageName::DoesPackageExist(Path.GetLongPackageName())) ? Path.TryLoad() : nullptr;
	};

	for (const FSoftObjectPath& MeshPath : MannequinMeshCandidates)
	{
		if (USkeletalMesh* Found = Cast<USkeletalMesh>(LoadIfPresent(MeshPath)))
		{
			SkelMesh->SetSkeletalMeshAsset(Found);
			for (const FSoftObjectPath& AnimPath : MannequinAnimClassCandidates)
			{
				if (UClass* AnimClass = Cast<UClass>(LoadIfPresent(AnimPath)))
				{
					SkelMesh->SetAnimInstanceClass(AnimClass);
					break;
				}
			}
			UE_LOG(LogCETest, Log, TEXT("%s: using mannequin %s"), *GetName(), *MeshPath.ToString());
			return;
		}
	}
	UE_LOG(LogCETest, Log, TEXT("%s: no skeletal mannequin in the project, using the block mannequin"), *GetName());
}

void ACEBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	BlockMannequin->EnsureBuilt();
	ConfigureHitCollision();
	UpdateLocomotionState();
}

void ACEBaseCharacter::ConfigureHitCollision()
{
	// With a skinned mesh + physics asset the mesh bodies are the hit target (bone names, headshots)
	// and the capsule ignores combat channels to avoid double hits. Without one, the capsule is the target.
	USkeletalMeshComponent* SkelMesh = GetMesh();
	const bool bMeshIsTarget = SkelMesh && SkelMesh->GetSkeletalMeshAsset() && SkelMesh->GetPhysicsAsset();
	const bool bBlocksAreTarget = BlockMannequin && BlockMannequin->IsBuilt();   // its parts carry their own hit volumes
	const ECollisionResponse CapsuleResponse = (bMeshIsTarget || bBlocksAreTarget) ? ECR_Ignore : ECR_Block;

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

void ACEBaseCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (UCEInputSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UCEInputSubsystem>() : nullptr)
	{
		Input->ApplyOnFootContexts();
	}
}

void ACEBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	const UCEInputSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UCEInputSubsystem>() : nullptr;
	if (!EIC || !Input)
	{
		UE_LOG(LogCETest, Warning, TEXT("%s: Enhanced Input not available (check DefaultInput.ini input classes)."), *GetName());
		return;
	}

	// IMC_Default
	EIC->BindAction(Input->IA_Move, ETriggerEvent::Triggered, this, &ACEBaseCharacter::Input_Move);
	EIC->BindAction(Input->IA_Move, ETriggerEvent::Completed, this, &ACEBaseCharacter::Input_MoveCompleted);
	EIC->BindAction(Input->IA_Look, ETriggerEvent::Triggered, this, &ACEBaseCharacter::Input_Look);
	EIC->BindAction(Input->IA_Jump, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_JumpStarted);
	EIC->BindAction(Input->IA_Jump, ETriggerEvent::Completed, this, &ACEBaseCharacter::Input_JumpCompleted);
	EIC->BindAction(Input->IA_Sprint, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_SprintStarted);
	EIC->BindAction(Input->IA_Sprint, ETriggerEvent::Completed, this, &ACEBaseCharacter::Input_SprintCompleted);
	EIC->BindAction(Input->IA_Crouch, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_Crouch);
	// IA_Interact is bound on ACEPlayerController (it must keep working inside vehicles).

	// IMC_Combat
	EIC->BindAction(Input->IA_Aim, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_AimStarted);
	EIC->BindAction(Input->IA_Aim, ETriggerEvent::Completed, this, &ACEBaseCharacter::Input_AimCompleted);
	EIC->BindAction(Input->IA_Fire, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_FireStarted);
	EIC->BindAction(Input->IA_Fire, ETriggerEvent::Completed, this, &ACEBaseCharacter::Input_FireCompleted);
	EIC->BindAction(Input->IA_Reload, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_Reload);
	EIC->BindAction(Input->IA_WeaponSlot1, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_WeaponSlot1);
	EIC->BindAction(Input->IA_WeaponSlot2, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_WeaponSlot2);
	EIC->BindAction(Input->IA_WeaponSlot3, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_WeaponSlot3);
	EIC->BindAction(Input->IA_WeaponSlot4, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_WeaponSlot4);
	EIC->BindAction(Input->IA_WeaponNext, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_WeaponNext);
	EIC->BindAction(Input->IA_WeaponPrev, ETriggerEvent::Started, this, &ACEBaseCharacter::Input_WeaponPrev);
}

void ACEBaseCharacter::Input_Move(const FInputActionValue& Value)
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

void ACEBaseCharacter::Input_MoveCompleted(const FInputActionValue& Value)
{
	MoveInput = FVector2D::ZeroVector;
}

void ACEBaseCharacter::Input_Look(const FInputActionValue& Value)
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

void ACEBaseCharacter::Input_JumpStarted(const FInputActionValue& Value)
{
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
		return;
	}
	Jump();
}

void ACEBaseCharacter::Input_JumpCompleted(const FInputActionValue& Value)
{
	StopJumping();
}

void ACEBaseCharacter::Input_SprintStarted(const FInputActionValue& Value)
{
	bWantsSprint = true;
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
	}
}

void ACEBaseCharacter::Input_SprintCompleted(const FInputActionValue& Value)
{
	bWantsSprint = false;
}

void ACEBaseCharacter::Input_Crouch(const FInputActionValue& Value)
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

void ACEBaseCharacter::Input_AimStarted(const FInputActionValue& Value)   { CombatComponent->SetAiming(true); }
void ACEBaseCharacter::Input_AimCompleted(const FInputActionValue& Value) { CombatComponent->SetAiming(false); }
void ACEBaseCharacter::Input_FireStarted(const FInputActionValue& Value)  { CombatComponent->StartFire(); }
void ACEBaseCharacter::Input_FireCompleted(const FInputActionValue& Value){ CombatComponent->StopFire(); }
void ACEBaseCharacter::Input_Reload(const FInputActionValue& Value)       { CombatComponent->Reload(); }

void ACEBaseCharacter::Input_WeaponSlot1(const FInputActionValue& Value) { CombatComponent->EquipSlot(1); }
void ACEBaseCharacter::Input_WeaponSlot2(const FInputActionValue& Value) { CombatComponent->EquipSlot(2); }
void ACEBaseCharacter::Input_WeaponSlot3(const FInputActionValue& Value) { CombatComponent->EquipSlot(3); }
void ACEBaseCharacter::Input_WeaponSlot4(const FInputActionValue& Value) { CombatComponent->EquipSlot(4); }
void ACEBaseCharacter::Input_WeaponNext(const FInputActionValue& Value)  { CombatComponent->CycleSlot(+1); }
void ACEBaseCharacter::Input_WeaponPrev(const FInputActionValue& Value)  { CombatComponent->CycleSlot(-1); }

// ---------------------------------------------------------------- locomotion

void ACEBaseCharacter::UpdateLocomotionState()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	ECELocomotionState NewState = ECELocomotionState::Walking;
	if (bInVehicle)
	{
		NewState = ECELocomotionState::InVehicle;
	}
	else if (Movement->IsSwimming())
	{
		NewState = ECELocomotionState::Swimming;
	}
	else if (GetCharacterMovement()->IsCrouching())
	{
		NewState = ECELocomotionState::Crouching;
	}
	else if (bWantsSprint && MoveInput.Y > 0.1f && !CombatComponent->IsAiming() && Movement->IsMovingOnGround())
	{
		NewState = ECELocomotionState::Sprinting;   // forward only, never while crouched / aiming / swimming
	}
	LocomotionState = NewState;

	float TargetSpeed = WalkSpeed;
	if (NewState == ECELocomotionState::Sprinting)
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

void ACEBaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateLocomotionState();

	float TargetFOV = DefaultFOV;
	if (CombatComponent->IsAiming())
	{
		TargetFOV = CombatComponent->GetCurrentWeaponData().WeaponType == ECEWeaponType::Sniper ? SniperAimFOV : AimFOV;
	}
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaSeconds, 12.f));

	if (bShowDebugHud && IsLocallyControlled())
	{
		DrawDebugHud();
	}
}

void ACEBaseCharacter::DrawDebugHud() const
{
	if (!GEngine)
	{
		return;
	}
	const FCEWeaponData& Weapon = CombatComponent->GetCurrentWeaponData();
	const UEnum* StateEnum = StaticEnum<ECELocomotionState>();
	const FString State = StateEnum ? StateEnum->GetNameStringByValue(static_cast<int64>(LocomotionState)) : FString();
	const FString Ammo = Weapon.MagSize > 0
		? FString::Printf(TEXT("%d / %d%s"), CombatComponent->GetAmmoInMag(), Weapon.MagSize, CombatComponent->IsReloading() ? TEXT("  RELOADING") : TEXT(""))
		: FString(TEXT("-"));
	const FString Text = FString::Printf(TEXT("[%d] %s   ammo %s   |  %s  %.0f cm/s   HP %.0f\n1-4 / wheel / Q weapons  LMB fire  RMB aim  R reload  Shift sprint  C crouch  E interact"),
		CombatComponent->GetCurrentSlot(), *Weapon.WeaponName.ToString(), *Ammo, *State, GetVelocity().Size(), HealthComponent->GetHealth());
	GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, FColor::White, Text);

	// Minimal crosshair.
	GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()) + 1, 0.f, FColor::Green,
		CombatComponent->IsAiming() ? TEXT("aiming") : TEXT(""));
}

// ---------------------------------------------------------------- vehicles

void ACEBaseCharacter::OnEnterVehicle(APawn* Vehicle, FName SeatSocket)
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

void ACEBaseCharacter::OnExitVehicle(const FVector& ExitLocation, const FRotator& ExitRotation)
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
