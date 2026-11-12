#include "Combat/CEWeaponTypes.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"

namespace
{
	FCEWeaponData MakeRow(FName Name, ECEWeaponType Type, ECEFireMode Mode, float Damage, float FireRate, bool bProjectile, float Recoil)
	{
		FCEWeaponData D;
		D.WeaponName = Name;
		D.WeaponType = Type;
		D.FireMode = Mode;
		D.BaseDamage = Damage;
		D.FireRate = FireRate;
		D.bUseProjectile = bProjectile;
		D.RecoilPitch = Recoil;
		return D;
	}

	TMap<FName, FCEWeaponData> BuildRows()
	{
		TMap<FName, FCEWeaponData> Rows;

		FCEWeaponData Fists = MakeRow(TEXT("Fists"), ECEWeaponType::Unarmed, ECEFireMode::Melee, 15.f, 0.55f, false, 0.f);
		Fists.MeleeReach = 110.f;
		Fists.MeleeRadius = 12.f;
		Rows.Add(Fists.WeaponName, Fists);

		FCEWeaponData Knife = MakeRow(TEXT("Knife"), ECEWeaponType::Knife, ECEFireMode::Melee, 40.f, 0.7f, false, 0.f);
		Knife.MeleeReach = 140.f;
		Knife.MeleeRadius = 4.f;
		Rows.Add(Knife.WeaponName, Knife);

		FCEWeaponData Pistol = MakeRow(TEXT("Pistol"), ECEWeaponType::Pistol, ECEFireMode::Single, 25.f, 0.25f, false, 1.2f);
		Pistol.Range = 6000.f;
		Pistol.SpreadDegrees = 0.6f;
		Pistol.MagSize = 15;
		Pistol.ReloadTime = 1.4f;
		Pistol.HolsterSocket = CombatSockets::HolsterThigh;
		Rows.Add(Pistol.WeaponName, Pistol);

		FCEWeaponData Rifle = MakeRow(TEXT("AssaultRifle"), ECEWeaponType::Rifle, ECEFireMode::Auto, 20.f, 0.1f, false, 0.6f);
		Rifle.Range = 20000.f;
		Rifle.SpreadDegrees = 0.9f;
		Rifle.MagSize = 30;
		Rifle.ReloadTime = 2.1f;
		Rifle.HolsterSocket = CombatSockets::SpineRifle;
		Rows.Add(Rifle.WeaponName, Rifle);

		FCEWeaponData Sniper = MakeRow(TEXT("SniperRifle"), ECEWeaponType::Sniper, ECEFireMode::Single, 120.f, 1.4f, true, 4.f);
		Sniper.Range = 100000.f;
		Sniper.MagSize = 5;
		Sniper.ReloadTime = 2.8f;
		Sniper.HolsterSocket = CombatSockets::SpineSniper;
		Rows.Add(Sniper.WeaponName, Sniper);

		return Rows;
	}
}

const FCEWeaponData* FCEBuiltinWeapons::Find(FName RowName)
{
	static const TMap<FName, FCEWeaponData> Rows = BuildRows();
	return Rows.Find(RowName);
}

FName CombatSockets::FallbackBone(FName Socket)
{
	if (Socket == Hand)         { return FName(TEXT("hand_r")); }
	if (Socket == HolsterThigh) { return FName(TEXT("thigh_r")); }
	if (Socket == SpineRifle)   { return FName(TEXT("spine_03")); }
	if (Socket == SpineSniper)  { return FName(TEXT("spine_02")); }
	return NAME_None;
}

bool CEIsHeadshot(const FHitResult& Hit)
{
	static const FName Head(TEXT("head"));
	if (Hit.BoneName == Head)
	{
		return true;
	}
	const UPrimitiveComponent* Component = Hit.GetComponent();
	return Component && Component->ComponentHasTag(Head);
}
