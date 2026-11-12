#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Combat/CEWeaponTypes.h"
#include "CECombatAnimInstance.generated.h"

class ACEBaseCharacter;

/**
 * Native parent for ABP_Base. Publishes the variables the locomotion state machine and the
 * ALI_Combat linked layers read; the graphs themselves are authored in the editor.
 */
UCLASS()
class CENTRALEUROPETEST_API UCECombatAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion") float GroundSpeed = 0.f;
	/** Movement direction relative to the actor, degrees in [-180, 180]. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion") float Direction = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion") bool bIsFalling = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion") bool bIsCrouching = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion") bool bIsSwimming = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion") bool bIsSprinting = false;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion") ECELocomotionState LocomotionState = ECELocomotionState::Walking;

	UPROPERTY(BlueprintReadOnly, Category = "Combat") bool bIsAiming = false;
	UPROPERTY(BlueprintReadOnly, Category = "Combat") bool bIsReloading = false;
	UPROPERTY(BlueprintReadOnly, Category = "Combat") ECEWeaponType WeaponType = ECEWeaponType::Unarmed;
	/** Aim offset inputs, degrees relative to the actor. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat") float AimPitch = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Combat") float AimYaw = 0.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACEBaseCharacter> Character;
};
