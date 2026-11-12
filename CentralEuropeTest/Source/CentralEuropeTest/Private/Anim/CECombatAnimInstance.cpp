#include "Anim/CECombatAnimInstance.h"

#include "Character/CEBaseCharacter.h"
#include "Combat/CECombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCECombatAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Character = Cast<ACEBaseCharacter>(TryGetPawnOwner());
}

void UCECombatAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character)
	{
		Character = Cast<ACEBaseCharacter>(TryGetPawnOwner());
		if (!Character)
		{
			return;
		}
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const FVector Velocity = Character->GetVelocity();
	GroundSpeed = Velocity.Size2D();

	const FVector LocalVelocity = Character->GetActorRotation().UnrotateVector(Velocity);
	Direction = GroundSpeed > 3.f ? FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X)) : 0.f;

	bIsFalling = Movement->IsFalling();
	bIsCrouching = Movement->IsCrouching();
	bIsSwimming = Movement->IsSwimming();
	LocomotionState = Character->GetLocomotionState();
	bIsSprinting = LocomotionState == ECELocomotionState::Sprinting;

	if (const UCECombatComponent* Combat = Character->CombatComponent)
	{
		bIsAiming = Combat->IsAiming();
		bIsReloading = Combat->IsReloading();
		WeaponType = Combat->GetCurrentWeaponData().WeaponType;
	}

	const FRotator Delta = (Character->GetBaseAimRotation() - Character->GetActorRotation()).GetNormalized();
	AimPitch = FMath::Clamp(Delta.Pitch, -90.f, 90.f);
	AimYaw = FMath::Clamp(Delta.Yaw, -90.f, 90.f);
}
