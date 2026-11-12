#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "Combat/WeaponTypes.h"
#include "CombatComponent.generated.h"

class ACharacter;
class AWeaponBase;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChanged, int32, Slot, const FWeaponData&, WeaponData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatAttack, const FWeaponData&, WeaponData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCombatHit, AActor*, HitActor, float, Damage, const FHitResult&, Hit);

/**
 * BP_CombatComponent parent. Owns weapon slots, fire timing, hitscan, projectile spawning,
 * melee sweeps (driven by UAnimNotifyState_MeleeTrace or a timed fallback), recoil and anim-layer linking.
 *
 * Slot 0 is always unarmed (fists); slots 1-4 come from SlotRows. Selecting the active slot again holsters.
 * Prototype scope: authoritative on the owning machine (standalone / listen server). No RPCs yet.
 */
UCLASS(ClassGroup = (Combat), Blueprintable, meta = (BlueprintSpawnableComponent))
class CENTRALEUROPETEST_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	// ---- configuration ----

	/** DataTable with FWeaponData rows. When empty the built-in defaults are used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<UDataTable> WeaponTable;

	/** Row names for slots 1-4. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<FName> SlotRows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName UnarmedRow = TEXT("Fists");

	/** Length of the timed melee window when no montage / notify drives the trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	float FallbackSwingDuration = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	float FallbackSwingDelay = 0.08f;

	/** Grip offset relative to the hand socket / bone. Tune per skeleton when no socket is authored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FTransform HandAttachOffset;

	// ---- input facing API ----

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EquipSlot(int32 Slot);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetAiming(bool bNewAiming);

	/** Used while driving a vehicle: holsters to fists and rejects attacks. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetAttacksBlocked(bool bBlocked);

	// ---- melee trace API (called by UAnimNotifyState_MeleeTrace) ----

	UFUNCTION(BlueprintCallable, Category = "Combat|Melee")
	void BeginMeleeTrace(FName StartSocket, FName EndSocket);

	UFUNCTION(BlueprintCallable, Category = "Combat|Melee")
	void TickMeleeTrace(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Combat|Melee")
	void EndMeleeTrace();

	// ---- state ----

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsReloading() const { return bReloading; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetCurrentSlot() const { return CurrentSlot; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	const FWeaponData& GetCurrentWeaponData() const { return CurrentData; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetAmmoInMag() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	AWeaponBase* GetCurrentWeaponActor() const;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatAttack OnAttack;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatHit OnHit;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	static constexpr int32 NumSlots = 5;   // 0 = unarmed, 1-4 = weapons

	const FWeaponData* FindWeaponData(FName RowName) const;
	FName GetRowForSlot(int32 Slot) const;

	void SpawnSlotWeapons();
	bool ResolveAttachPoint(FName Socket, USceneComponent*& OutParent, FName& OutSocket) const;
	void AttachWeaponToHand(AWeaponBase* Weapon) const;
	void AttachWeaponToHolster(AWeaponBase* Weapon, const FWeaponData& Data) const;
	void ApplySlot(int32 Slot);

	bool CanAttack() const;
	void FireTimerTick();
	void DoAttack();
	void BurstTimerTick();
	void FireOneShot();
	void FireHitscan();
	void FireProjectile();
	void StartMeleeAttack();
	void FinishReload();
	void ApplyRecoil();
	void CancelActions();

	/** Camera view for players, eyes for AI. */
	void GetAimView(FVector& OutLocation, FRotator& OutRotation) const;
	/** Camera ray with its origin pushed forward to the owner so cover behind the pawn is not hit. */
	void GetAimRay(FVector& OutStart, FVector& OutDirection) const;
	void ComputeMeleePoints(FVector& OutBase, FVector& OutTip) const;
	void ProcessMeleeHits(const TArray<FHitResult>& Hits);
	void DealDamage(const FHitResult& Hit, const FVector& Direction, float Damage);

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AWeaponBase>> SlotWeapons;

	UPROPERTY(Transient)
	TArray<FWeaponData> SlotData;

	TArray<int32> SlotAmmo;

	UPROPERTY(Transient)
	FWeaponData CurrentData;
	int32 CurrentSlot = 0;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> LinkedLayerClass;

	bool bIsAiming = false;
	bool bWantsFire = false;
	bool bReloading = false;
	bool bAttacksBlocked = false;

	float LastAttackTime = -1000.f;
	int32 BurstShotsRemaining = 0;
	float PendingRecoilRecovery = 0.f;

	// melee
	bool bMeleeTracing = false;
	bool bFallbackSwing = false;
	float FallbackElapsed = 0.f;
	float SwingAlpha = 0.f;
	FName MeleeStartSocket = NAME_None;
	FName MeleeEndSocket = NAME_None;
	FVector PrevMeleeCenter = FVector::ZeroVector;
	FVector PrevMeleeTip = FVector::ZeroVector;
	TSet<TWeakObjectPtr<AActor>> HitActors;

	FTimerHandle FireTimer;
	FTimerHandle BurstTimer;
	FTimerHandle ReloadTimer;
};
