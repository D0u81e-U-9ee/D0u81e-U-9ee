#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MeleeTrace.generated.h"

/**
 * ANS_MeleeTrace: place over the swing frames of a melee montage.
 * Opens the damage window on the owner's UCombatComponent, which sweeps the hand / blade every
 * tick and damages each actor at most once per swing.
 */
UCLASS(meta = (DisplayName = "ANS Melee Trace"))
class CENTRALEUROPETEST_API UAnimNotifyState_MeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Character mesh socket at the start of the trace. None = hand_r_socket (or the weapon's blade sockets). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	FName StartSocket = NAME_None;

	/** Optional second socket (e.g. lowerarm for a forearm strike). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	FName EndSocket = NAME_None;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
