#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Combat/WeaponTypes.h"
#include "BlockMannequinComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Asset-free stand-in mannequin: a jointed figure built from engine basic shapes at runtime,
 * 180 cm tall, origin at the capsule centre (feet at Z = -90), facing +X.
 *
 * - Procedurally animated from the owning character (walk / sprint cycle, crouch, swim, aim, punch).
 * - Every body part is a hit volume for the Weapon / Melee / Projectile channels; the head carries
 *   the component tag "head" so headshots work without a skeletal mesh.
 * - Provides attach points named like the design's sockets (hand_r_socket, holster_thigh_r,
 *   spine_rifle_socket, spine_sniper_socket).
 *
 * ABaseCharacter disables it automatically when a real skeletal mannequin is assigned.
 */
UCLASS(ClassGroup = (Character), meta = (BlueprintSpawnableComponent))
class CENTRALEUROPETEST_API UBlockMannequinComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UBlockMannequinComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mannequin")
	FLinearColor BodyColor = FLinearColor(0.22f, 0.32f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mannequin")
	FLinearColor HeadColor = FLinearColor(0.75f, 0.6f, 0.5f);

	/** False = static T-less idle pose (test dummies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mannequin")
	bool bAnimate = true;

	/** Disable before BeginPlay to suppress the figure (a real skeletal mesh is used instead). */
	void SetMannequinEnabled(bool bEnabled);

	/** Builds the figure if it is enabled and not built yet. Safe to call repeatedly. */
	void EnsureBuilt();

	UFUNCTION(BlueprintPure, Category = "Mannequin")
	bool IsBuilt() const { return bBuilt; }

	/** Attach point by socket name, or null. */
	USceneComponent* FindAttachPoint(FName SocketName) const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleAttack(const FWeaponData& WeaponData);

private:
	USceneComponent* AddPivot(USceneComponent* Parent, FName Name, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	UStaticMeshComponent* AddPart(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& SizeCm, const FVector& Center, UMaterialInstanceDynamic* Material, FName Tag = NAME_None);

	UPROPERTY(Transient) TObjectPtr<USceneComponent> BodyRoot;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> HipL;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> HipR;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> KneeL;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> KneeR;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> ShoulderL;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> ShoulderR;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> ElbowL;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> ElbowR;

	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> AttachPoints;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	bool bMannequinEnabled = true;
	bool bBuilt = false;

	float WalkPhase = 0.f;
	float CrouchAlpha = 0.f;
	float SwimAlpha = 0.f;
	float AimAlpha = 0.f;
	float PunchTimeLeft = 0.f;
};
