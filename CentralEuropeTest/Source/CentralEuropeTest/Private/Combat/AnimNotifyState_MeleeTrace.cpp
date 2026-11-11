#include "Combat/AnimNotifyState_MeleeTrace.h"

#include "Combat/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	UCombatComponent* FindCombat(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Owner ? Owner->FindComponentByClass<UCombatComponent>() : nullptr;
	}
}

void UAnimNotifyState_MeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (UCombatComponent* Combat = FindCombat(MeshComp))
	{
		Combat->BeginMeleeTrace(StartSocket, EndSocket);
	}
}

void UAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (UCombatComponent* Combat = FindCombat(MeshComp))
	{
		Combat->TickMeleeTrace(FrameDeltaTime);
	}
}

void UAnimNotifyState_MeleeTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UCombatComponent* Combat = FindCombat(MeshComp))
	{
		Combat->EndMeleeTrace();
	}
}

FString UAnimNotifyState_MeleeTrace::GetNotifyName_Implementation() const
{
	return TEXT("Melee Trace");
}
