#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Templates/SubclassOf.h"
#include "WeaponTypes.generated.h"

class AActor;
class UAnimInstance;
class UAnimMontage;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Unarmed,
	Knife,
	Pistol,
	Rifle,
	Sniper
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Single,
	Auto,
	Burst,
	Melee
};

UENUM(BlueprintType)
enum class ELocomotionState : uint8
{
	Walking,
	Sprinting,
	Crouching,
	Swimming,
	InVehicle
};

/**
 * ST_WeaponData from the design document. Usable as a DataTable row (see Content/Data/DT_Weapons.csv).
 * The first nine properties are the specified shared structure; the rest are optional extensions.
 */
USTRUCT(BlueprintType)
struct CENTRALEUROPETEST_API FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::Unarmed;

	/** Static or Skeletal Mesh. Leave empty to get a placeholder box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon",
		meta = (AllowedClasses = "/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
	TSoftObjectPtr<UObject> WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EFireMode FireMode = EFireMode::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float BaseDamage = 10.f;

	/** Delay in seconds between attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.01"))
	float FireRate = 0.5f;

	/** False = hitscan line trace, true = spawned ballistic projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bUseProjectile = false;

	/** Degrees of upward kick per shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RecoilPitch = 0.f;

	/** Anim Blueprint implementing ALI_Combat; linked at equip time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<UAnimInstance> AnimOverlayLayer;

	// ---- optional extensions ------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged")
	float Range = 10000.f;

	/** Cone half angle in degrees (hip fire; reduced while aiming). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged")
	float SpreadDegrees = 0.f;

	/** 0 = no ammo handling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged")
	int32 MagSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged")
	float ReloadTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged")
	int32 BurstCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged")
	float BurstInterval = 0.06f;

	/** Projectile actor class when bUseProjectile is set. Defaults to ASniperBullet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged")
	TSubclassOf<AActor> ProjectileClass;

	/** Mesh socket the weapon hangs on while not in hand. None = hidden when stowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName HolsterSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float HeadshotMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Melee")
	float MeleeReach = 110.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Melee")
	float MeleeRadius = 12.f;

	/** Attack montage carrying an ANS_MeleeTrace window. Empty = timed fallback swing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Melee")
	TSoftObjectPtr<UAnimMontage> AttackMontage;
};

/** Built-in rows used when no DataTable is assigned, so the prototype runs without any assets. */
struct CENTRALEUROPETEST_API FBuiltinWeapons
{
	static const FWeaponData* Find(FName RowName);
};

namespace CombatSockets
{
	static const FName Hand(TEXT("hand_r_socket"));
	static const FName HolsterThigh(TEXT("holster_thigh_r"));
	static const FName SpineRifle(TEXT("spine_rifle_socket"));
	static const FName SpineSniper(TEXT("spine_sniper_socket"));
	static const FName Muzzle(TEXT("muzzle"));
	static const FName BladeBase(TEXT("blade_base"));
	static const FName BladeTip(TEXT("blade_tip"));

	/** Bone to attach to when the socket has not been authored on the skeleton yet. */
	CENTRALEUROPETEST_API FName FallbackBone(FName Socket);
}

/** Skeletal mesh "head" bone, or any hit component tagged "head" (block mannequin). */
CENTRALEUROPETEST_API bool IsHeadshot(const struct FHitResult& Hit);
