#include "Combat/CombatComponent.h"

#include "CentralEuropeTest.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/SniperBullet.h"
#include "Combat/WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DataTable.h"
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
	const FName HeadBone(TEXT("head"));

	bool DebugEnabled()
	{
		return CVarCombatDebug.GetValueOnGameThread() != 0;
	}
}

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	SlotRows = { TEXT("Pistol"), TEXT("AssaultRifle"), TEXT("SniperRifle"), TEXT("Knife") };
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	SpawnSlotWeapons();
	ApplySlot(0);
}

void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelActions();
	for (AWeaponBase* Weapon : SlotWeapons)
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

const FWeaponData* UCombatComponent::FindWeaponData(FName RowName) const
{
	if (RowName.IsNone())
	{
		return nullptr;
	}
	if (WeaponTable)
	{
		if (const FWeaponData* Row = WeaponTable->FindRow<FWeaponData>(RowName, TEXT("CombatComponent"), false))
		{
			return Row;
		}
	}
	return FBuiltinWeapons::Find(RowName);
}

FName UCombatComponent::GetRowForSlot(int32 Slot) const
{
	if (Slot == 0)
	{
		return UnarmedRow;
	}
	return SlotRows.IsValidIndex(Slot - 1) ? SlotRows[Slot - 1] : NAME_None;
}

void UCombatComponent::SpawnSlotWeapons()
{
	SlotWeapons.Init(nullptr, NumSlots);
	SlotData.Init(FWeaponData(), NumSlots);
	SlotAmmo.Init(0, NumSlots);

	UWorld* World = GetWorld();
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		const FWeaponData* Data = FindWeaponData(GetRowForSlot(Slot));
		if (!Data)
		{
			if (Slot > 0)
			{
				UE_LOG(LogCETest, Warning, TEXT("Combat: no weapon row '%s' for slot %d"), *GetRowForSlot(Slot).ToString(), Slot);
			}
			if (const FWeaponData* Fallback = FBuiltinWeapons::Find(TEXT("Fists")))
			{
				SlotData[Slot] = *Fallback;
			}
			continue;
		}

		SlotData[Slot] = *Data;
		SlotAmmo[Slot] = Data->MagSize;

		if (Data->WeaponType == EWeaponType::Unarmed || !World)
		{
			continue;
		}

		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AWeaponBase* Weapon = World->SpawnActor<AWeaponBase>(AWeaponBase::StaticClass(), FTransform::Identity, Params))
		{
			Weapon->InitFromData(*Data);
			SlotWeapons[Slot] = Weapon;
			AttachWeaponToHolster(Weapon, *Data);
		}
	}
}

void UCombatComponent::AttachWeaponToHand(AWeaponBase* Weapon) const
{
	if (!Weapon || !GetOwner())
	{
		return;
	}
	USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	if (Mesh && Mesh->DoesSocketExist(CombatSockets::Hand))
	{
		Weapon->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, CombatSockets::Hand);
	}
	else
	{
		// No mannequin / socket yet: float the weapon at the right hip so it is still visible.
		Weapon->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorRelativeLocation(FVector(35.f, 22.f, 35.f));
	}
	Weapon->SetActorHiddenInGame(false);
}

void UCombatComponent::AttachWeaponToHolster(AWeaponBase* Weapon, const FWeaponData& Data) const
{
	if (!Weapon || !GetOwner())
	{
		return;
	}
	USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	if (Mesh && !Data.HolsterSocket.IsNone() && Mesh->DoesSocketExist(Data.HolsterSocket))
	{
		Weapon->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data.HolsterSocket);
		Weapon->SetActorHiddenInGame(false);
	}
	else
	{
		// Knife, or holster socket not authored yet: stow invisibly.
		Weapon->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorHiddenInGame(true);
	}
}

void UCombatComponent::EquipSlot(int32 Slot)
{
	if (bAttacksBlocked || !SlotData.IsValidIndex(0))
	{
		return;
	}
	Slot = FMath::Clamp(Slot, 0, NumSlots - 1);
	if (Slot == CurrentSlot)
	{
		Slot = 0;   // selecting the active slot again holsters -> fists
	}
	if (Slot > 0 && !SlotWeapons[Slot] && SlotData[Slot].WeaponType != EWeaponType::Unarmed)
	{
		return;     // slot has no valid row
	}
	ApplySlot(Slot);
}

void UCombatComponent::ApplySlot(int32 Slot)
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

int32 UCombatComponent::GetAmmoInMag() const
{
	return SlotAmmo.IsValidIndex(CurrentSlot) ? SlotAmmo[CurrentSlot] : 0;
}

AWeaponBase* UCombatComponent::GetCurrentWeaponActor() const
{
	return SlotWeapons.IsValidIndex(CurrentSlot) ? SlotWeapons[CurrentSlot].Get() : nullptr;
}

void UCombatComponent::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming && !bAttacksBlocked;
}

void UCombatComponent::SetAttacksBlocked(bool bBlocked)
{
	if (bBlocked)
	{
		ApplySlot(0);
		bIsAiming = false;
	}
	bAttacksBlocked = bBlocked;
}

void UCombatComponent::CancelActions()
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

bool UCombatComponent::CanAttack() const
{
	return GetOwner() && GetWorld() && !bAttacksBlocked && !bReloading;
}

void UCombatComponent::StartFire()
{
	bWantsFire = true;
	if (!CanAttack())
	{
		return;
	}

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	const float Now = GetWorld()->GetTimeSeconds();
	const float Remaining = CurrentData.FireRate - (Now - LastAttackTime);
	const bool bRepeatWhileHeld = CurrentData.FireMode == EFireMode::Auto || CurrentData.FireMode == EFireMode::Melee;

	// The press itself is rate limited (no tap-spam above FireRate). While held, the looping
	// timer is the limiter, so its ticks do not re-check the clock (avoids frame-quantisation skips).
	if (Remaining <= 0.f)
	{
		DoAttack();
		if (bRepeatWhileHeld && bWantsFire)
		{
			Timers.SetTimer(FireTimer, this, &UCombatComponent::FireTimerTick, CurrentData.FireRate, true);
		}
	}
	else if (bRepeatWhileHeld)
	{
		Timers.SetTimer(FireTimer, this, &UCombatComponent::FireTimerTick, CurrentData.FireRate, true, Remaining);
	}
}

void UCombatComponent::StopFire()
{
	bWantsFire = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimer);
	}
}

void UCombatComponent::FireTimerTick()
{
	if (!bWantsFire || !CanAttack())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimer);
		return;
	}
	DoAttack();
}

void UCombatComponent::DoAttack()
{
	LastAttackTime = GetWorld()->GetTimeSeconds();

	switch (CurrentData.FireMode)
	{
	case EFireMode::Melee:
		StartMeleeAttack();
		OnAttack.Broadcast(CurrentData);
		break;

	case EFireMode::Burst:
		BurstShotsRemaining = FMath::Max(CurrentData.BurstCount, 1);
		BurstTimerTick();
		if (BurstShotsRemaining > 0)
		{
			GetWorld()->GetTimerManager().SetTimer(BurstTimer, this, &UCombatComponent::BurstTimerTick,
				FMath::Max(CurrentData.BurstInterval, 0.01f), true);
		}
		break;

	default:
		FireOneShot();
		break;
	}
}

void UCombatComponent::BurstTimerTick()
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

void UCombatComponent::FireOneShot()
{
	if (CurrentData.MagSize > 0)
	{
		if (SlotAmmo[CurrentSlot] <= 0)
		{
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
	ApplyRecoil();
	OnAttack.Broadcast(CurrentData);
}

void UCombatComponent::Reload()
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
	BurstShotsRemaining = 0;
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(FireTimer);
	Timers.ClearTimer(BurstTimer);
	Timers.SetTimer(ReloadTimer, this, &UCombatComponent::FinishReload, FMath::Max(CurrentData.ReloadTime, 0.05f), false);
}

void UCombatComponent::FinishReload()
{
	bReloading = false;
	SlotAmmo[CurrentSlot] = CurrentData.MagSize;
	if (bWantsFire && CurrentData.FireMode == EFireMode::Auto)
	{
		StartFire();
	}
}

// ---------------------------------------------------------------- aiming helpers

void UCombatComponent::GetAimView(FVector& OutLocation, FRotator& OutRotation) const
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

void UCombatComponent::GetAimRay(FVector& OutStart, FVector& OutDirection) const
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

void UCombatComponent::DealDamage(const FHitResult& Hit, const FVector& Direction, float Damage)
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
}

// ---------------------------------------------------------------- hitscan

void UCombatComponent::FireHitscan()
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
	for (const AWeaponBase* Weapon : SlotWeapons)
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
		const bool bHeadshot = Hit.BoneName == HeadBone;
		DealDamage(Hit, Direction, CurrentData.BaseDamage * (bHeadshot ? CurrentData.HeadshotMultiplier : 1.f));
	}

#if ENABLE_DRAW_DEBUG
	if (DebugEnabled())
	{
		const AWeaponBase* Weapon = GetCurrentWeaponActor();
		const FVector TracerStart = Weapon ? Weapon->GetMuzzleTransform().GetLocation() : Start;
		const FVector TracerEnd = bHit ? Hit.ImpactPoint : End;
		DrawDebugLine(GetWorld(), TracerStart, TracerEnd, FColor::Yellow, false, 0.08f, 0, 1.f);
		if (bHit)
		{
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 8.f, FColor::Red, false, 1.5f);
		}
	}
#endif
}

// ---------------------------------------------------------------- ballistic projectile

void UCombatComponent::FireProjectile()
{
	UWorld* World = GetWorld();
	FVector AimStart, AimDirection;
	GetAimRay(AimStart, AimDirection);

	// Where the crosshair points: the bullet leaves the muzzle toward that point.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponAim), false, GetOwner());
	for (const AWeaponBase* Weapon : SlotWeapons)
	{
		if (Weapon)
		{
			Params.AddIgnoredActor(Weapon);
		}
	}
	FHitResult AimHit;
	const FVector AimEnd = AimStart + AimDirection * 100000.f;
	const FVector AimPoint = World->LineTraceSingleByChannel(AimHit, AimStart, AimEnd, ECC_Weapon, Params) ? AimHit.ImpactPoint : AimEnd;

	const AWeaponBase* Weapon = GetCurrentWeaponActor();
	FVector Muzzle = Weapon ? Weapon->GetMuzzleTransform().GetLocation() : AimStart;
	FVector ShotDirection = (AimPoint - Muzzle).GetSafeNormal();
	if (FVector::DotProduct(ShotDirection, AimDirection) < 0.5f)
	{
		// Target closer than the muzzle (hugging a wall): fire straight along the camera ray instead.
		Muzzle = AimStart;
		ShotDirection = AimDirection;
	}

	UClass* BulletClass = ASniperBullet::StaticClass();
	if (CurrentData.ProjectileClass && CurrentData.ProjectileClass->IsChildOf(ASniperBullet::StaticClass()))
	{
		BulletClass = CurrentData.ProjectileClass;
	}

	const FTransform SpawnTransform(ShotDirection.Rotation(), Muzzle);
	ASniperBullet* Bullet = World->SpawnActorDeferred<ASniperBullet>(BulletClass, SpawnTransform, GetOwner(),
		Cast<APawn>(GetOwner()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Bullet)
	{
		Bullet->Damage = CurrentData.BaseDamage;
		Bullet->HeadshotMultiplier = CurrentData.HeadshotMultiplier;
		Bullet->FinishSpawning(SpawnTransform);
	}
}

// ---------------------------------------------------------------- recoil

void UCombatComponent::ApplyRecoil()
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

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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

void UCombatComponent::StartMeleeAttack()
{
	if (bMeleeTracing)
	{
		EndMeleeTrace();
	}
	bFallbackSwing = false;

	UAnimMontage* Montage = CurrentData.AttackMontage.IsNull() ? nullptr : CurrentData.AttackMontage.LoadSynchronous();
	const bool bCanPlay = Montage && OwnerCharacter && OwnerCharacter->GetMesh() && OwnerCharacter->GetMesh()->GetAnimInstance();
	if (bCanPlay && OwnerCharacter->PlayAnimMontage(Montage) > 0.f)
	{
		return;   // ANS_MeleeTrace inside the montage opens / closes the trace window
	}

	bFallbackSwing = true;
	FallbackElapsed = 0.f;
}

void UCombatComponent::BeginMeleeTrace(FName StartSocket, FName EndSocket)
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

void UCombatComponent::EndMeleeTrace()
{
	bMeleeTracing = false;
	HitActors.Reset();
}

void UCombatComponent::ComputeMeleePoints(FVector& OutBase, FVector& OutTip) const
{
	// 1) Authored blade sockets on the weapon mesh.
	if (const AWeaponBase* Weapon = GetCurrentWeaponActor())
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
			const float Extra = CurrentData.WeaponType == EWeaponType::Knife ? 30.f : 10.f;
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

void UCombatComponent::TickMeleeTrace(float DeltaTime)
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
	for (const AWeaponBase* Weapon : SlotWeapons)
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
	const bool bBox = CurrentData.WeaponType == EWeaponType::Knife;
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

void UCombatComponent::ProcessMeleeHits(const TArray<FHitResult>& Hits)
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
