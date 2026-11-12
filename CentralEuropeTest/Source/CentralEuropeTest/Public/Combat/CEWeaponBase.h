#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/CEWeaponTypes.h"
#include "CEWeaponBase.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;

/** Visual weapon actor (BP_WeaponBase parent). Purely cosmetic; all logic lives in UCECombatComponent. */
UCLASS(Blueprintable)
class CENTRALEUROPETEST_API ACEWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	ACEWeaponBase();

	/** Loads the mesh named in the row, or builds a scaled placeholder box when none is set. */
	void InitFromData(const FCEWeaponData& Data);

	/** "muzzle" socket of the active mesh, or the front of the placeholder. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FTransform GetMuzzleTransform() const;

	/** Returns false when the active mesh has no such socket. */
	bool GetSocketLocation(FName Socket, FVector& OutLocation) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

private:
	const USceneComponent* GetActiveMesh() const;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaceholderMesh;

	float PlaceholderLength = 0.f;
	bool bUsingSkeletal = false;
};
