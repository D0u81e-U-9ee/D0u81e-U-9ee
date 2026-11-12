#include "Combat/CEAnimNotifyState_MeleeTrace.h"

#include "Combat/CECombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	UCECombatComponent* FindCombat(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Owner ? Owner->FindComponentByClass<UCECombatComponent>() : nullptr;
	}
}

void UCEAnimNotifyState_MeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (UCECombatComponent* Combat = FindCombat(MeshComp))
	{
		Combat->BeginMeleeTrace(StartSocket, EndSocket);
	}
}

void UCEAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (UCECombatComponent* Combat = FindCombat(MeshComp))
	{
		Combat->TickMeleeTrace(FrameDeltaTime);
	}
}

void UCEAnimNotifyState_MeleeTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UCECombatComponent* Combat = FindCombat(MeshComp))
	{
		Combat->EndMeleeTrace();
	}
}

FString UCEAnimNotifyState_MeleeTrace::GetNotifyName_Implementation() const
{
	return TEXT("Melee Trace");
}
