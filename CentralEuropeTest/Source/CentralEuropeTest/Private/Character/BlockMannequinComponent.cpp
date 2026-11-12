#include "Character/BlockMannequinComponent.h"

#include "CentralEuropeTest.h"
#include "Combat/CombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const float PunchDuration = 0.3f;
	const FName HeadTag(TEXT("head"));
}

UBlockMannequinComponent::UBlockMannequinComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UBlockMannequinComponent::SetMannequinEnabled(bool bEnabled)
{
	bMannequinEnabled = bEnabled;
}

void UBlockMannequinComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureBuilt();

	if (AActor* Owner = GetOwner())
	{
		if (UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>())
		{
			Combat->OnAttack.AddDynamic(this, &UBlockMannequinComponent::HandleAttack);
		}
	}
}

USceneComponent* UBlockMannequinComponent::AddPivot(USceneComponent* Parent, FName Name, const FVector& Location, const FRotator& Rotation)
{
	USceneComponent* Pivot = NewObject<USceneComponent>(GetOwner(), Name);
	Pivot->SetupAttachment(Parent);
	Pivot->SetRelativeLocationAndRotation(Location, Rotation);
	Pivot->RegisterComponent();
	return Pivot;
}

UStaticMeshComponent* UBlockMannequinComponent::AddPart(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& SizeCm, const FVector& Center, UMaterialInstanceDynamic* Material, FName Tag)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(GetOwner());
	Part->SetupAttachment(Parent);
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Center);
	Part->SetRelativeScale3D(SizeCm / 100.f);      // engine basic shapes are 100 cm
	if (Material)
	{
		Part->SetMaterial(0, Material);
	}
	// Hit volume for combat only: never blocks movement, camera or interaction traces.
	Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Part->SetCollisionObjectType(ECC_WorldDynamic);
	Part->SetCollisionResponseToAllChannels(ECR_Ignore);
	Part->SetCollisionResponseToChannel(ECC_Weapon, ECR_Block);
	Part->SetCollisionResponseToChannel(ECC_Melee, ECR_Block);
	Part->SetCollisionResponseToChannel(ECC_Projectile, ECR_Block);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCanEverAffectNavigation(false);
	if (!Tag.IsNone())
	{
		Part->ComponentTags.Add(Tag);
	}
	Part->RegisterComponent();
	Parts.Add(Part);
	return Part;
}

void UBlockMannequinComponent::EnsureBuilt()
{
	if (bBuilt || !bMannequinEnabled || !GetOwner() || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	bBuilt = true;

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UMaterialInstanceDynamic* BodyMaterial = nullptr;
	UMaterialInstanceDynamic* HeadMaterial = nullptr;
	if (BaseMaterial)
	{
		BodyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), BodyColor);
		HeadMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		HeadMaterial->SetVectorParameterValue(TEXT("Color"), HeadColor);
	}

	// Rotating a pivot by +pitch swings the limb hanging below it forward (+X).
	BodyRoot = AddPivot(this, NAME_None, FVector::ZeroVector);
	AddPart(BodyRoot, Cube, FVector(22.f, 34.f, 52.f), FVector(0.f, 0.f, 28.f), BodyMaterial);            // pelvis + torso
	AddPart(BodyRoot, Sphere, FVector(22.f, 22.f, 22.f), FVector(0.f, 0.f, 68.f), HeadMaterial, HeadTag);  // head

	auto BuildLeg = [&](float Side, TObjectPtr<USceneComponent>& Hip, TObjectPtr<USceneComponent>& Knee)
	{
		Hip = AddPivot(BodyRoot, NAME_None, FVector(0.f, Side * 9.f, 2.f));
		AddPart(Hip, Cube, FVector(13.f, 13.f, 45.f), FVector(0.f, 0.f, -22.5f), BodyMaterial);
		Knee = AddPivot(Hip, NAME_None, FVector(0.f, 0.f, -45.f));
		AddPart(Knee, Cube, FVector(11.f, 11.f, 47.f), FVector(0.f, 0.f, -23.5f), BodyMaterial);           // foot ends at Z = -90
	};
	BuildLeg(-1.f, HipL, KneeL);
	BuildLeg(+1.f, HipR, KneeR);

	auto BuildArm = [&](float Side, TObjectPtr<USceneComponent>& Shoulder, TObjectPtr<USceneComponent>& Elbow)
	{
		Shoulder = AddPivot(BodyRoot, NAME_None, FVector(0.f, Side * 22.f, 50.f));
		AddPart(Shoulder, Cube, FVector(10.f, 10.f, 28.f), FVector(0.f, 0.f, -14.f), BodyMaterial);
		Elbow = AddPivot(Shoulder, NAME_None, FVector(0.f, 0.f, -28.f));
		AddPart(Elbow, Cube, FVector(9.f, 9.f, 28.f), FVector(0.f, 0.f, -14.f), HeadMaterial);
	};
	BuildArm(-1.f, ShoulderL, ElbowL);
	BuildArm(+1.f, ShoulderR, ElbowR);

	// Attach points. A weapon's +X is its barrel; pitch -90 aligns it with the forearm.
	AttachPoints.Add(AddPivot(ElbowR, CombatSockets::Hand, FVector(0.f, 0.f, -30.f), FRotator(-90.f, 0.f, 0.f)));
	AttachPoints.Add(AddPivot(HipR, CombatSockets::HolsterThigh, FVector(0.f, 8.f, -12.f), FRotator(-90.f, 0.f, 0.f)));
	AttachPoints.Add(AddPivot(BodyRoot, CombatSockets::SpineRifle, FVector(-14.f, 7.f, 8.f), FRotator(75.f, 0.f, 0.f)));
	AttachPoints.Add(AddPivot(BodyRoot, CombatSockets::SpineSniper, FVector(-14.f, -7.f, 4.f), FRotator(80.f, 0.f, 0.f)));
}

USceneComponent* UBlockMannequinComponent::FindAttachPoint(FName SocketName) const
{
	for (USceneComponent* Point : AttachPoints)
	{
		if (Point && Point->GetFName() == SocketName)
		{
			return Point;
		}
	}
	return nullptr;
}

void UBlockMannequinComponent::HandleAttack(const FWeaponData& WeaponData)
{
	if (WeaponData.FireMode == EFireMode::Melee)
	{
		PunchTimeLeft = PunchDuration;
	}
}

void UBlockMannequinComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!bBuilt || !bAnimate || !Character)
	{
		return;
	}
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const UCombatComponent* Combat = Character->FindComponentByClass<UCombatComponent>();

	const float Speed = Character->GetVelocity().Size2D();
	const bool bSwimming = Movement->IsSwimming();
	const bool bFalling = Movement->IsFalling();
	const bool bArmed = Combat && Combat->GetCurrentWeaponData().WeaponType != EWeaponType::Unarmed;
	const bool bAiming = Combat && Combat->IsAiming();
	const float AimPitch = FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch);

	CrouchAlpha = FMath::FInterpTo(CrouchAlpha, Movement->IsCrouching() ? 1.f : 0.f, DeltaTime, 10.f);
	SwimAlpha = FMath::FInterpTo(SwimAlpha, bSwimming ? 1.f : 0.f, DeltaTime, 6.f);
	AimAlpha = FMath::FInterpTo(AimAlpha, bAiming ? 1.f : 0.f, DeltaTime, 14.f);
	PunchTimeLeft = FMath::Max(PunchTimeLeft - DeltaTime, 0.f);

	// Gait: ~0.9 Hz at walking speed, ~1.9 Hz sprinting. Swimming kicks at a fixed rate.
	WalkPhase += (bSwimming ? 7.f : Speed / 45.f) * DeltaTime;
	if (WalkPhase > 2.f * PI)
	{
		WalkPhase -= 2.f * PI;
	}
	const float Stride = bSwimming ? 22.f : FMath::Clamp(Speed / 300.f, 0.f, 1.f) * FMath::GetMappedRangeValueClamped(FVector2f(250.f, 550.f), FVector2f(30.f, 46.f), Speed);
	const float Swing = FMath::Sin(WalkPhase) * Stride;
	const float AirTuck = bFalling ? 25.f : 0.f;

	// Body: crouch lowers the hips so the feet stay on the ground while the capsule shrinks by 30 cm;
	// swimming lays the figure forward.
	const float SprintLean = FMath::GetMappedRangeValueClamped(FVector2f(300.f, 550.f), FVector2f(0.f, -10.f), Speed);
	BodyRoot->SetRelativeLocation(FVector(0.f, 0.f, -15.7f * CrouchAlpha));
	BodyRoot->SetRelativeRotation(FRotator(FMath::Lerp(SprintLean, -78.f, SwimAlpha), 0.f, 0.f));

	// Legs.
	const float CrouchHip = 70.f * CrouchAlpha;
	const float CrouchKnee = 120.f * CrouchAlpha;
	HipL->SetRelativeRotation(FRotator(Swing + CrouchHip + AirTuck, 0.f, 0.f));
	HipR->SetRelativeRotation(FRotator(-Swing + CrouchHip + AirTuck, 0.f, 0.f));
	KneeL->SetRelativeRotation(FRotator(-(FMath::Max(FMath::Cos(WalkPhase), 0.f) * Stride * 1.3f + CrouchKnee + AirTuck * 1.5f), 0.f, 0.f));
	KneeR->SetRelativeRotation(FRotator(-(FMath::Max(-FMath::Cos(WalkPhase), 0.f) * Stride * 1.3f + CrouchKnee + AirTuck * 1.5f), 0.f, 0.f));

	// Left arm counter-swings; when swimming both arms reach forward.
	const float SwimReach = 150.f * SwimAlpha;
	ShoulderL->SetRelativeRotation(FRotator(-Swing * 0.8f + SwimReach, 0.f, 0.f));
	ElbowL->SetRelativeRotation(FRotator(FMath::Abs(Swing) * 0.4f + 8.f, 0.f, 0.f));

	// Right arm: punch > aim > armed low-ready > counter-swing.
	float ShoulderPitch = Swing * 0.8f + SwimReach;
	float ShoulderYaw = 0.f;
	float ElbowPitch = FMath::Abs(Swing) * 0.4f + 8.f;
	if (bArmed)
	{
		// Low ready: upper arm 35 deg forward, forearm level. Aiming: arm straight along the view pitch.
		ShoulderPitch = FMath::Lerp(35.f, 90.f + AimPitch, AimAlpha);
		ElbowPitch = FMath::Lerp(55.f, 0.f, AimAlpha);
	}
	if (PunchTimeLeft > 0.f)
	{
		const float T = 1.f - PunchTimeLeft / PunchDuration;
		const float Arc = FMath::Sin(T * PI);
		ShoulderPitch = FMath::Lerp(ShoulderPitch, 95.f + AimPitch * 0.6f, Arc);
		ShoulderYaw = FMath::Lerp(35.f, -35.f, T) * Arc;      // right-to-left, like the damage sweep
		ElbowPitch = FMath::Lerp(ElbowPitch, 5.f, Arc);
	}
	ShoulderR->SetRelativeRotation(FRotator(ShoulderPitch, ShoulderYaw, 0.f));
	ElbowR->SetRelativeRotation(FRotator(ElbowPitch, 0.f, 0.f));
}
