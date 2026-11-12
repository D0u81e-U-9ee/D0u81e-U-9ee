#include "Combat/CECombatComponent.h"

#include "CentralEuropeTest.h"
#include "Audio/CEAudioSubsystem.h"
#include "Character/CEBlockMannequinComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/CESniperBullet.h"
#include "Combat/CEWeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

static TAutoConsoleVariable<int32> CVarCombatDebug(
	TEXT("combat.Debug"), 1,
	TEXT("1 = draw hitscan tracers and melee sweeps (prototype default), 0 = off."));

namespace
{
	bool DebugEnabled()
	{
		return CVarCombatDebug.GetValueOnGameThread() != 0;
	}
}

UCECombatComponent::UCECombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	SlotRows = { TEXT("Pistol"), TEXT("AssaultRifle"), TEXT("SniperRifle"), TEXT("Knife") };
}

void UCECombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	SpawnSlotWeapons();
	ApplySlot(0);
}

void UCECombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelActions();
	for (ACEWeaponBase* Weapon : SlotWeapons)
	{
		if (Weapon)
		{
			Weapon->Destroy();
		}
	}
	SlotWeapons.Reset();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------- data / slots

const FCEWeaponData* UCECombatComponent::FindWeaponData(FName RowName) const
{
	if (RowName.IsNone())
	{
		return nullptr;
	}
	if (WeaponTable)
	{
		if (const FCEWeaponData* Row = WeaponTable->FindRow<FCEWeaponData>(RowName, TEXT("CombatComponent"), false))
		{
			return Row;
		}
	}
	return FCEBuiltinWeapons::Find(RowName);
}

FName UCECombatComponent::GetRowForSlot(int32 Slot) const
{
	if (Slot == 0)
	{
		return UnarmedRow;
	}
	return SlotRows.IsValidIndex(Slot - 1) ? SlotRows[Slot - 1] : NAME_None;
}

void UCECombatComponent::SpawnSlotWeapons()
{
	SlotWeapons.Init(nullptr, NumSlots);
	SlotData.Init(FCEWeaponData(), NumSlots);
	SlotAmmo.Init(0, NumSlots);

	UWorld* World = GetWorld();
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		const FCEWeaponData* Data = FindWeaponData(GetRowForSlot(Slot));
		if (!Data)
		{
			if (Slot > 0)
			{
				UE_LOG(LogCETest, Warning, TEXT("Combat: no weapon row '%s' for slot %d"), *GetRowForSlot(Slot).ToString(), Slot);
			}
			if (const FCEWeaponData* Fallback = FCEBuiltinWeapons::Find(TEXT("Fists")))
			{
				SlotData[Slot] = *Fallback;
			}
			continue;
		}

		SlotData[Slot] = *Data;
		SlotAmmo[Slot] = Data->MagSize;

		if (Data->WeaponType == ECEWeaponType::Unarmed || !World)
		{
			continue;
		}

		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ACEWeaponBase* Weapon = World->SpawnActor<ACEWeaponBase>(ACEWeaponBase::StaticClass(), FTransform::Identity, Params))
		{
			Weapon->InitFromData(*Data);
			SlotWeapons[Slot] = Weapon;
			AttachWeaponToHolster(Weapon, *Data);
		}
		else
		{
			UE_LOG(LogCETest, Error, TEXT("Combat: failed to spawn the weapon actor for slot %d (%s)"), Slot, *Data->WeaponName.ToString());
		}
	}
}

bool UCECombatComponent::ResolveAttachPoint(FName Socket, USceneComponent*& OutParent, FName& OutSocket) const
{
	OutParent = nullptr;
	OutSocket = NAME_None;
	if (Socket.IsNone() || !GetOwner())
	{
		return false;
	}

	// 1) Real skeletal mannequin: authored socket, else the matching bone.
	USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	if (Mesh && Mesh->GetSkeletalMeshAsset())
	{
		const FName Bone = CombatSockets::FallbackBone(Socket);
		const FName Use = Mesh->DoesSocketExist(Socket) ? Socket : (!Bone.IsNone() && Mesh->DoesSocketExist(Bone) ? Bone : NAME_None);
		if (!Use.IsNone())
		{
			OutParent = Mesh;
			OutSocket = Use;
			return true;
		}
	}

	// 2) Block mannequin attach points.
	if (UCEBlockMannequinComponent* Mannequin = GetOwner()->FindComponentByClass<UCEBlockMannequinComponent>())
	{
		Mannequin->EnsureBuilt();
		if (USceneComponent* Point = Mannequin->FindAttachPoint(Socket))
		{
			OutParent = Point;
			return true;
		}
	}
	return false;
}

void UCECombatComponent::AttachWeaponToHand(ACEWeaponBase* Weapon) const
{
	if (!Weapon || !GetOwner())
	{
		return;
	}
	USceneComponent* Parent = nullptr;
	FName Socket = NAME_None;
	if (ResolveAttachPoint(CombatSockets::Hand, Parent, Socket))
	{
		Weapon->AttachToComponent(Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
		Weapon->SetActorRelativeTransform(HandAttachOffset);
	}
	else
	{
		// No body at all: float the weapon at the right hip so it is still visible.
		Weapon->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorRelativeLocation(FVector(35.f, 22.f, 35.f));
	}
	Weapon->SetActorHiddenInGame(false);
}

void UCECombatComponent::AttachWeaponToHolster(ACEWeaponBase* Weapon, const FCEWeaponData& Data) const
{
	if (!Weapon || !GetOwner())
	{
		return;
	}
	USceneComponent* Parent = nullptr;
	FName Socket = NAME_None;
	if (ResolveAttachPoint(Data.HolsterSocket, Parent, Socket))
	{
		Weapon->AttachToComponent(Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
		Weapon->SetActorHiddenInGame(false);
	}
	else
	{
		// Knife, or no holster point: stow invisibly.
		Weapon->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorHiddenInGame(true);
	}
}

void UCECombatComponent::ShowStatus(const FString& Message, bool bProblem) const
{
	UE_LOG(LogCETest, Log, TEXT("Combat: %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, bProblem ? 6.f : 2.f, bProblem ? FColor::Red : FColor::Yellow, Message);
	}
}

void UCECombatComponent::EquipSlot(int32 Slot)
{
	// Every early-out reports why, so a "key does nothing" report can be traced on screen.
	if (bAttacksBlocked)
	{
		ShowStatus(TEXT("Weapon switch ignored: weapons are blocked (in a vehicle)"), true);
		return;
	}
	if (!SlotData.IsValidIndex(NumSlots - 1) || !SlotWeapons.IsValidIndex(NumSlots - 1))
	{
		ShowStatus(TEXT("Weapon switch failed: weapon slots were never set up (combat component BeginPlay did not run)"), true);
		return;
	}
	Slot = FMath::Clamp(Slot, 0, NumSlots - 1);
	if (Slot == CurrentSlot)
	{
		Slot = 0;   // selecting the active slot again holsters -> fists
	}
	if (Slot > 0 && !SlotWeapons[Slot])
	{
		ShowStatus(FString::Printf(TEXT("Slot %d is empty: weapon row '%s' was not found or its actor failed to spawn"),
			Slot, *GetRowForSlot(Slot).ToString()), true);
		return;
	}
	ApplySlot(Slot);
	UCEAudioSubsystem::PlayAt(this, TEXT("Equip"), GetOwner()->GetActorLocation(), ECESoundRange::Near, 0.8f);
	ShowStatus(FString::Printf(TEXT("Equipped [%d] %s"), CurrentSlot, *CurrentData.WeaponName.ToString()));
}

void UCECombatComponent::CycleSlot(int32 Direction)
{
	if (!SlotWeapons.IsValidIndex(NumSlots - 1))
	{
		EquipSlot(1);   // reports the setup problem
		return;
	}
	const int32 Step = Direction >= 0 ? 1 : -1;
	int32 Slot = CurrentSlot;
	for (int32 Attempt = 0; Attempt < NumSlots; ++Attempt)
	{
		Slot = (Slot + Step + NumSlots) % NumSlots;
		if (Slot == 0 || SlotWeapons[Slot])
		{
			break;
		}
	}
	if (Slot == CurrentSlot)
	{
		return;
	}
	if (Slot == 0)
	{
		EquipSlot(CurrentSlot);   // same slot again = holster to fists
	}
	else
	{
		EquipSlot(Slot);
	}
}

void UCECombatComponent::ApplySlot(int32 Slot)
{
	if (!SlotData.IsValidIndex(Slot))
	{
		return;
	}
	CancelActions();

	if (SlotWeapons.IsValidIndex(CurrentSlot) && SlotWeapons[CurrentSlot])
	{
		AttachWeaponToHolster(SlotWeapons[CurrentSlot], SlotData[CurrentSlot]);
	}

	CurrentSlot = Slot;
	CurrentData = SlotData[Slot];
	AttachWeaponToHand(SlotWeapons[Slot]);

	// Linked anim layers: swap the combat pose set without touching the main state machine.
	if (USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr)
	{
		if (Mesh->GetAnimInstance())
		{
			if (LinkedLayerClass && LinkedLayerClass != CurrentData.AnimOverlayLayer)
			{
				Mesh->UnlinkAnimClassLayers(LinkedLayerClass);
				LinkedLayerClass = nullptr;
			}
			if (CurrentData.AnimOverlayLayer && LinkedLayerClass != CurrentData.AnimOverlayLayer)
			{
				Mesh->LinkAnimClassLayers(CurrentData.AnimOverlayLayer);
				LinkedLayerClass = CurrentData.AnimOverlayLayer;
			}
		}
	}

	OnWeaponChanged.Broadcast(CurrentSlot, CurrentData);
}

int32 UCECombatComponent::GetAmmoInMag() const
{
	return SlotAmmo.IsValidIndex(CurrentSlot) ? SlotAmmo[CurrentSlot] : 0;
}

ACEWeaponBase* UCECombatComponent::GetCurrentWeaponActor() const
{
	return SlotWeapons.IsValidIndex(CurrentSlot) ? SlotWeapons[CurrentSlot].Get() : nullptr;
}

void UCECombatComponent::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming && !bAttacksBlocked;
}

void UCECombatComponent::SetAttacksBlocked(bool bBlocked)
{
	if (bBlocked)
	{
		ApplySlot(0);
		bIsAiming = false;
	}
	bAttacksBlocked = bBlocked;
}

void UCECombatComponent::CancelActions()
{
	bWantsFire = false;
	bReloading = false;
	BurstShotsRemaining = 0;
	if (UWorld* World = GetWorld())
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.ClearTimer(FireTimer);
		Timers.ClearTimer(BurstTimer);
		Timers.ClearTimer(ReloadTimer);
	}
	if (bMeleeTracing)
	{
		EndMeleeTrace();
	}
	bFallbackSwing = false;
}

// ---------------------------------------------------------------- fire control

bool UCECombatComponent::CanAttack() const
{
	return GetOwner() && GetWorld() && !bAttacksBlocked && !bReloading;
}

void UCECombatComponent::StartFire()
{
	bWantsFire = true;
	if (!CanAttack())
	{
		return;
	}

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	const float Now = GetWorld()->GetTimeSeconds();
	const float Remaining = CurrentData.FireRate - (Now - LastAttackTime);
	const bool bRepeatWhileHeld = CurrentData.FireMode == ECEFireMode::Auto || CurrentData.FireMode == ECEFireMode::Melee;

	// The press itself is rate limited (no tap-spam above FireRate). While held, the looping
	// timer is the limiter, so its ticks do not re-check the clock (avoids frame-quantisation skips).
	if (Remaining <= 0.f)
	{
		DoAttack();
		if (bRepeatWhileHeld && bWantsFire)
		{
			Timers.SetTimer(FireTimer, this, &UCECombatComponent::FireTimerTick, CurrentData.FireRate, true);
		}
	}
	else if (bRepeatWhileHeld)
	{
		Timers.SetTimer(FireTimer, this, &UCECombatComponent::FireTimerTick, CurrentData.FireRate, true, Remaining);
	}
}

void UCECombatComponent::StopFire()
{
	bWantsFire = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimer);
	}
}

void UCECombatComponent::FireTimerTick()
{
	if (!bWantsFire || !CanAttack())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimer);
		return;
	}
	DoAttack();
}

void UCECombatComponent::DoAttack()
{
	LastAttackTime = GetWorld()->GetTimeSeconds();

	switch (CurrentData.FireMode)
	{
	case ECEFireMode::Melee:
		StartMeleeAttack();
		OnAttack.Broadcast(CurrentData);
		break;

	case ECEFireMode::Burst:
		BurstShotsRemaining = FMath::Max(CurrentData.BurstCount, 1);
		BurstTimerTick();
		if (BurstShotsRemaining > 0)
		{
			GetWorld()->GetTimerManager().SetTimer(BurstTimer, this, &UCECombatComponent::BurstTimerTick,
				FMath::Max(CurrentData.BurstInterval, 0.01f), true);
		}
		break;

	default:
		FireOneShot();
		break;
	}
}

void UCECombatComponent::BurstTimerTick()
{
	if (BurstShotsRemaining <= 0 || !CanAttack())
	{
		BurstShotsRemaining = 0;
		GetWorld()->GetTimerManager().ClearTimer(BurstTimer);
		return;
	}
	--BurstShotsRemaining;
	FireOneShot();
	if (BurstShotsRemaining <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstTimer);
	}
}

void UCECombatComponent::FireOneShot()
{
	if (CurrentData.MagSize > 0)
	{
		if (SlotAmmo[CurrentSlot] <= 0)
		{
			UCEAudioSubsystem::PlayAt(this, TEXT("Gun_Empty"), GetOwner()->GetActorLocation(), ECESoundRange::Near);
			Reload();
			return;
		}
		--SlotAmmo[CurrentSlot];
	}

	if (CurrentData.bUseProjectile)
	{
		FireProjectile();
	}
	else
	{
		FireHitscan();
	}
	// Gunshot: one sound per weapon class, slight pitch scatter so automatic fire does not sound looped.
	FName ShotSound = TEXT("Gun_Pistol");
	if (CurrentData.WeaponType == ECEWeaponType::Rifle)
	{
		ShotSound = TEXT("Gun_Rifle");
	}
	else if (CurrentData.WeaponType == ECEWeaponType::Sniper)
	{
		ShotSound = TEXT("Gun_Sniper");
	}
	const ACEWeaponBase* ShotWeapon = GetCurrentWeaponActor();
	UCEAudioSubsystem::PlayAt(this, ShotSound, ShotWeapon ? ShotWeapon->GetMuzzleTransform().GetLocation() : GetOwner()->GetActorLocation(),
		ECESoundRange::Far, 1.f, FMath::FRandRange(0.96f, 1.04f));

	ApplyRecoil();
	OnAttack.Broadcast(CurrentData);
}

void UCECombatComponent::Reload()
{
	if (bReloading || bAttacksBlocked || CurrentData.MagSize <= 0 || !GetWorld())
	{
		return;
	}
	if (SlotAmmo[CurrentSlot] >= CurrentData.MagSize)
	{
		return;
	}
	bReloading = true;
	UCEAudioSubsystem::PlayAt(this, TEXT("Reload_Start"), GetOwner()->GetActorLocation(), ECESoundRange::Near);
	BurstShotsRemaining = 0;
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(FireTimer);
	Timers.ClearTimer(BurstTimer);
	Timers.SetTimer(ReloadTimer, this, &UCECombatComponent::FinishReload, FMath::Max(CurrentData.ReloadTime, 0.05f), false);
}

void UCECombatComponent::FinishReload()
{
	bReloading = false;
	SlotAmmo[CurrentSlot] = CurrentData.MagSize;
	if (GetOwner())
	{
		UCEAudioSubsystem::PlayAt(this, TEXT("Reload_End"), GetOwner()->GetActorLocation(), ECESoundRange::Near);
	}
	if (bWantsFire && CurrentData.FireMode == ECEFireMode::Auto)
	{
		StartFire();
	}
}

// ---------------------------------------------------------------- aiming helpers

void UCECombatComponent::GetAimView(FVector& OutLocation, FRotator& OutRotation) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (const AController* Controller = Pawn ? Pawn->GetController() : nullptr)
	{
		Controller->GetPlayerViewPoint(OutLocation, OutRotation);
	}
	else if (GetOwner())
	{
		GetOwner()->GetActorEyesViewPoint(OutLocation, OutRotation);
	}
}

void UCECombatComponent::GetAimRay(FVector& OutStart, FVector& OutDirection) const
{
	FRotator ViewRotation = FRotator::ZeroRotator;
	GetAimView(OutStart, ViewRotation);
	OutDirection = ViewRotation.Vector();

	// Third person: start the ray level with the pawn so geometry between camera and pawn is skipped.
	if (GetOwner())
	{
		const float Along = FVector::DotProduct(GetOwner()->GetActorLocation() - OutStart, OutDirection);
		if (Along > 0.f)
		{
			OutStart += OutDirection * Along;
		}
	}
}

void UCECombatComponent::DealDamage(const FHitResult& Hit, const FVector& Direction, float Damage)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	UGameplayStatics::ApplyPointDamage(HitActor, Damage, Direction, Hit,
		Pawn ? Pawn->GetController() : nullptr, GetOwner(), UDamageType::StaticClass());
	OnHit.Broadcast(HitActor, Damage, Hit);

	// Impact at the target, plus a quiet confirmation tick for the local shooter.
	const bool bMelee = CurrentData.FireMode == ECEFireMode::Melee;
	UCEAudioSubsystem::PlayAt(this, bMelee ? FName(TEXT("Melee_Hit")) : FName(TEXT("Bullet_Impact")), Hit.ImpactPoint, ECESoundRange::Mid);
	if (Pawn && Pawn->IsLocallyControlled())
	{
		UCEAudioSubsystem::Play2D(this, TEXT("Hit_Confirm"), 0.6f);
	}
}

// ---------------------------------------------------------------- hitscan

void UCECombatComponent::FireHitscan()
{
	FVector Start, Direction;
	GetAimRay(Start, Direction);

	if (CurrentData.SpreadDegrees > 0.f)
	{
		const float Spread = CurrentData.SpreadDegrees * (bIsAiming ? 0.35f : 1.f);
		Direction = FMath::VRandCone(Direction, FMath::DegreesToRadians(Spread));
	}
	const FVector End = Start + Direction * CurrentData.Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponHitscan), false, GetOwner());
	Params.bReturnPhysicalMaterial = true;
	for (const ACEWeaponBase* Weapon : SlotWeapons)
	{
		if (Weapon)
		{
			Params.AddIgnoredActor(Weapon);
		}
	}

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Weapon, Params);
	if (bHit)
	{
		const bool bHeadshot = CEIsHeadshot(Hit);
		DealDamage(Hit, Direction, CurrentData.BaseDamage * (bHeadshot ? CurrentData.HeadshotMultiplier : 1.f));
		if (!Hit.GetActor() || !Hit.GetActor()->CanBeDamaged())
		{
			UCEAudioSubsystem::PlayAt(this, TEXT("Bullet_Impact"), Hit.ImpactPoint, ECESoundRange::Mid, 0.8f);
		}
	}

#if ENABLE_DRAW_DEBUG
	if (DebugEnabled())
	{
		const ACEWeaponBase* Weapon = GetCurrentWeaponActor();
		const FVector TracerStart = Weapon ? Weapon->GetMuzzleTransform().GetLocation() : Start;
		const FVector TracerEnd = bHit ? Hit.ImpactPoint : End;
		DrawDebugLine(GetWorld(), TracerStart, TracerEnd, FColor::Yellow, false, 0.35f, 0, 3.f);
		if (bHit)
		{
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 14.f, FColor::Red, false, 2.f);
		}
	}
#endif
}

// ---------------------------------------------------------------- ballistic projectile

void UCECombatComponent::FireProjectile()
{
	UWorld* World = GetWorld();
	FVector AimStart, AimDirection;
	GetAimRay(AimStart, AimDirection);

	// Where the crosshair points: the bullet leaves the muzzle toward that point.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponAim), false, GetOwner());
	for (const ACEWeaponBase* Weapon : SlotWeapons)
	{
		if (Weapon)
		{
			Params.AddIgnoredActor(Weapon);
		}
	}
	FHitResult AimHit;
	const FVector AimEnd = AimStart + AimDirection * 100000.f;
	const FVector AimPoint = World->LineTraceSingleByChannel(AimHit, AimStart, AimEnd, ECC_Weapon, Params) ? AimHit.ImpactPoint : AimEnd;

	const ACEWeaponBase* Weapon = GetCurrentWeaponActor();
	FVector Muzzle = Weapon ? Weapon->GetMuzzleTransform().GetLocation() : AimStart;
	FVector ShotDirection = (AimPoint - Muzzle).GetSafeNormal();
	if (FVector::DotProduct(ShotDirection, AimDirection) < 0.5f)
	{
		// Target closer than the muzzle (hugging a wall): fire straight along the camera ray instead.
		Muzzle = AimStart;
		ShotDirection = AimDirection;
	}

	UClass* BulletClass = ACESniperBullet::StaticClass();
	if (CurrentData.ProjectileClass && CurrentData.ProjectileClass->IsChildOf(ACESniperBullet::StaticClass()))
	{
		BulletClass = CurrentData.ProjectileClass;
	}

	const FTransform SpawnTransform(ShotDirection.Rotation(), Muzzle);
	ACESniperBullet* Bullet = World->SpawnActorDeferred<ACESniperBullet>(BulletClass, SpawnTransform, GetOwner(),
		Cast<APawn>(GetOwner()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Bullet)
	{
		Bullet->Damage = CurrentData.BaseDamage;
		Bullet->HeadshotMultiplier = CurrentData.HeadshotMultiplier;
		Bullet->FinishSpawning(SpawnTransform);
	}
}

// ---------------------------------------------------------------- recoil

void UCECombatComponent::ApplyRecoil()
{
	if (CurrentData.RecoilPitch <= 0.f)
	{
		return;
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}
	// Written straight into the control rotation so it does not depend on input scale settings.
	const float Kick = CurrentData.RecoilPitch * (bIsAiming ? 0.6f : 1.f);
	FRotator Rotation = PC->GetControlRotation();
	Rotation.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch) + Kick, -89.f, 89.f);
	Rotation.Yaw += FMath::FRandRange(-0.25f, 0.25f) * Kick;
	PC->SetControlRotation(Rotation);
	PendingRecoilRecovery += Kick * 0.6f;
}

void UCECombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Recoil recovery: ease back 60 % of the kick once the trigger has rested briefly.
	if (PendingRecoilRecovery > 0.f && GetWorld() && GetWorld()->GetTimeSeconds() - LastAttackTime > 0.12f)
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		if (APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
		{
			const float Step = FMath::Min(PendingRecoilRecovery, PendingRecoilRecovery * FMath::Min(DeltaTime * 12.f, 1.f) + 0.01f);
			FRotator Rotation = PC->GetControlRotation();
			Rotation.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch) - Step, -89.f, 89.f);
			PC->SetControlRotation(Rotation);
			PendingRecoilRecovery -= Step;
		}
		else
		{
			PendingRecoilRecovery = 0.f;
		}
	}

	// Timed melee window used when no montage with ANS_MeleeTrace drives the swing.
	if (bFallbackSwing)
	{
		FallbackElapsed += DeltaTime;
		if (!bMeleeTracing)
		{
			if (FallbackElapsed >= FallbackSwingDelay)
			{
				BeginMeleeTrace(NAME_None, NAME_None);
			}
		}
		else
		{
			TickMeleeTrace(DeltaTime);
			if (FallbackElapsed >= FallbackSwingDelay + FallbackSwingDuration)
			{
				EndMeleeTrace();
				bFallbackSwing = false;
			}
		}
	}
}

// ---------------------------------------------------------------- melee

void UCECombatComponent::StartMeleeAttack()
{
	if (bMeleeTracing)
	{
		EndMeleeTrace();
	}
	bFallbackSwing = false;
	UCEAudioSubsystem::PlayAt(this, TEXT("Melee_Swing"), GetOwner()->GetActorLocation(), ECESoundRange::Near, 1.f, FMath::FRandRange(0.92f, 1.08f));

	UAnimMontage* Montage = CurrentData.AttackMontage.IsNull() ? nullptr : CurrentData.AttackMontage.LoadSynchronous();
	const bool bCanPlay = Montage && OwnerCharacter && OwnerCharacter->GetMesh() && OwnerCharacter->GetMesh()->GetAnimInstance();
	if (bCanPlay && OwnerCharacter->PlayAnimMontage(Montage) > 0.f)
	{
		return;   // ANS_MeleeTrace inside the montage opens / closes the trace window
	}

	bFallbackSwing = true;
	FallbackElapsed = 0.f;
}

void UCECombatComponent::BeginMeleeTrace(FName StartSocket, FName EndSocket)
{
	MeleeStartSocket = StartSocket;
	MeleeEndSocket = EndSocket;
	HitActors.Reset();
	SwingAlpha = 0.f;
	bMeleeTracing = true;

	FVector Base, Tip;
	ComputeMeleePoints(Base, Tip);
	PrevMeleeCenter = (Base + Tip) * 0.5f;
	PrevMeleeTip = Tip;
}

void UCECombatComponent::EndMeleeTrace()
{
	bMeleeTracing = false;
	HitActors.Reset();
}

void UCECombatComponent::ComputeMeleePoints(FVector& OutBase, FVector& OutTip) const
{
	// 1) Authored blade sockets on the weapon mesh.
	if (const ACEWeaponBase* Weapon = GetCurrentWeaponActor())
	{
		if (Weapon->GetSocketLocation(CombatSockets::BladeBase, OutBase) && Weapon->GetSocketLocation(CombatSockets::BladeTip, OutTip))
		{
			return;
		}
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	GetAimView(ViewLocation, ViewRotation);

	// 2) Character sockets (hand / notify supplied), animation driven.
	const USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	const FName StartSocket = MeleeStartSocket.IsNone() ? CombatSockets::Hand : MeleeStartSocket;
	if (!bFallbackSwing && Mesh && Mesh->DoesSocketExist(StartSocket))
	{
		OutBase = Mesh->GetSocketLocation(StartSocket);
		if (!MeleeEndSocket.IsNone() && Mesh->DoesSocketExist(MeleeEndSocket))
		{
			OutTip = Mesh->GetSocketLocation(MeleeEndSocket);
		}
		else
		{
			const float Extra = CurrentData.WeaponType == ECEWeaponType::Knife ? 30.f : 10.f;
			OutTip = OutBase + ViewRotation.Vector() * Extra;
		}
		return;
	}

	// 3) No animation yet: synthesise a right-to-left arc in front of the chest.
	const AActor* Owner = GetOwner();
	const FVector Chest = Owner ? Owner->GetActorLocation() + FVector(0.f, 0.f, 35.f) : ViewLocation;
	FRotator SwingRotation(ViewRotation.Pitch * 0.6f, ViewRotation.Yaw + FMath::Lerp(50.f, -50.f, SwingAlpha), 0.f);
	const FVector Direction = SwingRotation.Vector();
	OutBase = Chest + Direction * 30.f;
	OutTip = Chest + Direction * FMath::Max(CurrentData.MeleeReach, 40.f);
}

void UCECombatComponent::TickMeleeTrace(float DeltaTime)
{
	if (!bMeleeTracing || !GetWorld())
	{
		return;
	}
	SwingAlpha = FMath::Clamp(SwingAlpha + DeltaTime / FMath::Max(FallbackSwingDuration, 0.05f), 0.f, 1.f);

	FVector Base, Tip;
	ComputeMeleePoints(Base, Tip);
	const FVector Center = (Base + Tip) * 0.5f;
	const FVector Axis = Tip - Base;
	const float Length = Axis.Size();
	const float Radius = FMath::Max(CurrentData.MeleeRadius, 1.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeTrace), false, GetOwner());
	for (const ACEWeaponBase* Weapon : SlotWeapons)
	{
		if (Weapon)
		{
			Params.AddIgnoredActor(Weapon);
		}
	}

	// Sweep from last frame's pose to this frame's so fast swings / low frame rates cannot tunnel.
	auto SafeEnd = [](const FVector& From, const FVector& To)
	{
		return FVector::DistSquared(From, To) < 1.f ? From + FVector(0.f, 0.f, 1.f) : To;
	};

	TArray<FHitResult> Hits;
	const bool bBox = CurrentData.WeaponType == ECEWeaponType::Knife;
	if (bBox)
	{
		// Knife: box aligned with the blade.
		const FQuat Orientation = Length > KINDA_SMALL_NUMBER ? Axis.ToOrientationQuat() : FQuat::Identity;
		const FCollisionShape Shape = FCollisionShape::MakeBox(FVector(FMath::Max(Length * 0.5f, 1.f), Radius, Radius));
		GetWorld()->SweepMultiByChannel(Hits, PrevMeleeCenter, SafeEnd(PrevMeleeCenter, Center), Orientation, ECC_Melee, Shape, Params);
		ProcessMeleeHits(Hits);
	}
	else
	{
		// Fists: sphere along the travelled path, then along the arm so close targets register too.
		const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
		GetWorld()->SweepMultiByChannel(Hits, PrevMeleeTip, SafeEnd(PrevMeleeTip, Tip), FQuat::Identity, ECC_Melee, Shape, Params);
		ProcessMeleeHits(Hits);
		Hits.Reset();
		GetWorld()->SweepMultiByChannel(Hits, Base, SafeEnd(Base, Tip), FQuat::Identity, ECC_Melee, Shape, Params);
		ProcessMeleeHits(Hits);
	}

#if ENABLE_DRAW_DEBUG
	if (DebugEnabled())
	{
		DrawDebugLine(GetWorld(), Base, Tip, FColor::Cyan, false, 0.4f, 0, 1.5f);
		DrawDebugLine(GetWorld(), PrevMeleeTip, Tip, FColor::Blue, false, 0.4f, 0, 1.f);
	}
#endif

	PrevMeleeCenter = Center;
	PrevMeleeTip = Tip;
}

void UCECombatComponent::ProcessMeleeHits(const TArray<FHitResult>& Hits)
{
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == GetOwner() || !HitActor->CanBeDamaged())
		{
			continue;
		}
		bool bAlreadyHit = false;
		HitActors.Add(TWeakObjectPtr<AActor>(HitActor), &bAlreadyHit);
		if (bAlreadyHit)
		{
			continue;   // one damage event per actor per swing
		}
		const FVector Direction = (Hit.ImpactPoint - GetOwner()->GetActorLocation()).GetSafeNormal();
		DealDamage(Hit, Direction, CurrentData.BaseDamage);
	}
}
