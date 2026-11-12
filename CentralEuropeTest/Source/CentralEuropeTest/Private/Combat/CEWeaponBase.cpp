#include "Combat/CEWeaponBase.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACEWeaponBase::ACEWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(Root);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(Root);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	PlaceholderMesh = Cube.Object;
}

void ACEWeaponBase::InitFromData(const FCEWeaponData& Data)
{
	bUsingSkeletal = false;
	PlaceholderLength = 0.f;
	StaticMesh->SetRelativeTransform(FTransform::Identity);

	UObject* Asset = Data.WeaponMesh.IsNull() ? nullptr : Data.WeaponMesh.LoadSynchronous();
	if (USkeletalMesh* SkelAsset = Cast<USkeletalMesh>(Asset))
	{
		SkeletalMesh->SetSkeletalMeshAsset(SkelAsset);
		StaticMesh->SetStaticMesh(nullptr);
		bUsingSkeletal = true;
		return;
	}
	if (UStaticMesh* StaticAsset = Cast<UStaticMesh>(Asset))
	{
		StaticMesh->SetStaticMesh(StaticAsset);
		return;
	}

	// Placeholder: the engine cube is 100 cm, scale it to a rough weapon silhouette with the grip at the origin.
	FVector Size(20.f, 4.f, 12.f);
	switch (Data.WeaponType)
	{
	case ECEWeaponType::Knife:  Size = FVector(25.f, 1.f, 4.f); break;
	case ECEWeaponType::Pistol: Size = FVector(20.f, 4.f, 12.f); break;
	case ECEWeaponType::Rifle:  Size = FVector(80.f, 5.f, 15.f); break;
	case ECEWeaponType::Sniper: Size = FVector(110.f, 5.f, 14.f); break;
	default: break;
	}
	PlaceholderLength = Size.X;
	StaticMesh->SetStaticMesh(PlaceholderMesh);
	StaticMesh->SetRelativeScale3D(Size / 100.f);
	StaticMesh->SetRelativeLocation(FVector(Size.X * 0.5f, 0.f, 0.f));
}

const USceneComponent* ACEWeaponBase::GetActiveMesh() const
{
	if (bUsingSkeletal)
	{
		return SkeletalMesh;
	}
	return StaticMesh;
}

bool ACEWeaponBase::GetSocketLocation(FName Socket, FVector& OutLocation) const
{
	const USceneComponent* Mesh = GetActiveMesh();
	if (PlaceholderLength <= 0.f && Mesh && Mesh->DoesSocketExist(Socket))
	{
		OutLocation = Mesh->GetSocketLocation(Socket);
		return true;
	}
	return false;
}

FTransform ACEWeaponBase::GetMuzzleTransform() const
{
	const USceneComponent* Mesh = GetActiveMesh();
	if (PlaceholderLength <= 0.f && Mesh && Mesh->DoesSocketExist(CombatSockets::Muzzle))
	{
		return Mesh->GetSocketTransform(CombatSockets::Muzzle);
	}
	const float Length = PlaceholderLength > 0.f ? PlaceholderLength : 30.f;
	return FTransform(GetActorRotation(), GetActorLocation() + GetActorForwardVector() * Length);
}
