#include "Input/CEInputSubsystem.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace
{
	enum class EMod : uint8 { None, Negate, Swizzle, NegateSwizzle };

	void Map(UInputMappingContext* Context, UInputAction* Action, FKey Key, EMod Mod = EMod::None, float Scale = 1.f)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (Mod == EMod::Negate || Mod == EMod::NegateSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
		if (Mod == EMod::Swizzle || Mod == EMod::NegateSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (!FMath::IsNearlyEqual(Scale, 1.f))
		{
			UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(Context);
			Scalar->Scalar = FVector(Scale);
			Mapping.Modifiers.Add(Scalar);
		}
	}

	void MapStick(UInputMappingContext* Context, UInputAction* Action, FKey Key, float Scale, bool bPerSecond = false)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		Mapping.Modifiers.Add(NewObject<UInputModifierDeadZone>(Context));
		if (bPerSecond)
		{
			// Stick deflection is a rate: convert to a per-frame delta like the mouse delivers.
			Mapping.Modifiers.Add(NewObject<UInputModifierScaleByDeltaTime>(Context));
		}
		if (!FMath::IsNearlyEqual(Scale, 1.f))
		{
			UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(Context);
			Scalar->Scalar = FVector(Scale);
			Mapping.Modifiers.Add(Scalar);
		}
	}
}

void UCEInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BuildDefaults();
}

void UCEInputSubsystem::BuildDefaults()
{
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, FName(Name));
		Action->ValueType = Type;
		return Action;
	};

	IA_Move       = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	IA_Look       = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	IA_Jump       = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	IA_Sprint     = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	IA_Crouch     = MakeAction(TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	IA_Interact   = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	IA_Aim        = MakeAction(TEXT("IA_Aim"), EInputActionValueType::Boolean);
	IA_Fire       = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	IA_Reload     = MakeAction(TEXT("IA_Reload"), EInputActionValueType::Boolean);
	IA_WeaponSlot = MakeAction(TEXT("IA_WeaponSlot"), EInputActionValueType::Axis1D);

	// ---- IMC_Default ----
	IMC_Default = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));
	// IA_Move value: X = right, Y = forward
	Map(IMC_Default, IA_Move, EKeys::W, EMod::Swizzle);
	Map(IMC_Default, IA_Move, EKeys::S, EMod::NegateSwizzle);
	Map(IMC_Default, IA_Move, EKeys::A, EMod::Negate);
	Map(IMC_Default, IA_Move, EKeys::D);
	MapStick(IMC_Default, IA_Move, EKeys::Gamepad_Left2D, 1.f);
	// IA_Look value in degrees per frame: X = yaw right, Y = pitch up (mouse up is +Y).
	// The engine already scales Mouse2D by its axis sensitivity (0.07 in BaseInput.ini);
	// overall feel is tuned with ACEBaseCharacter::LookSensitivity.
	Map(IMC_Default, IA_Look, EKeys::Mouse2D);
	MapStick(IMC_Default, IA_Look, EKeys::Gamepad_Right2D, 150.f, true);   // 150 deg/s at full deflection
	Map(IMC_Default, IA_Jump, EKeys::SpaceBar);
	Map(IMC_Default, IA_Jump, EKeys::Gamepad_FaceButton_Bottom);
	Map(IMC_Default, IA_Sprint, EKeys::LeftShift);
	Map(IMC_Default, IA_Sprint, EKeys::Gamepad_LeftThumbstick);
	Map(IMC_Default, IA_Crouch, EKeys::LeftControl);
	Map(IMC_Default, IA_Crouch, EKeys::C);
	Map(IMC_Default, IA_Crouch, EKeys::Gamepad_FaceButton_Right);
	Map(IMC_Default, IA_Interact, EKeys::E);
	Map(IMC_Default, IA_Interact, EKeys::Gamepad_FaceButton_Top);

	// ---- IMC_Combat ----
	IMC_Combat = NewObject<UInputMappingContext>(this, TEXT("IMC_Combat"));
	Map(IMC_Combat, IA_Aim, EKeys::RightMouseButton);
	Map(IMC_Combat, IA_Aim, EKeys::Gamepad_LeftTrigger);
	Map(IMC_Combat, IA_Fire, EKeys::LeftMouseButton);
	Map(IMC_Combat, IA_Fire, EKeys::Gamepad_RightTrigger);
	Map(IMC_Combat, IA_Reload, EKeys::R);
	Map(IMC_Combat, IA_Reload, EKeys::Gamepad_FaceButton_Left);
	// One Axis1D action for all four slots: the Scalar modifier turns the key into its slot index.
	Map(IMC_Combat, IA_WeaponSlot, EKeys::One, EMod::None, 1.f);
	Map(IMC_Combat, IA_WeaponSlot, EKeys::Two, EMod::None, 2.f);
	Map(IMC_Combat, IA_WeaponSlot, EKeys::Three, EMod::None, 3.f);
	Map(IMC_Combat, IA_WeaponSlot, EKeys::Four, EMod::None, 4.f);
	Map(IMC_Combat, IA_WeaponSlot, EKeys::Gamepad_DPad_Left, EMod::None, 1.f);
	Map(IMC_Combat, IA_WeaponSlot, EKeys::Gamepad_DPad_Up, EMod::None, 2.f);
	Map(IMC_Combat, IA_WeaponSlot, EKeys::Gamepad_DPad_Right, EMod::None, 3.f);
	Map(IMC_Combat, IA_WeaponSlot, EKeys::Gamepad_DPad_Down, EMod::None, 4.f);

	// ---- IMC_Vehicle ----
	IMC_Vehicle = NewObject<UInputMappingContext>(this, TEXT("IMC_Vehicle"));
	Map(IMC_Vehicle, IA_Interact, EKeys::E);
	Map(IMC_Vehicle, IA_Interact, EKeys::F);
	Map(IMC_Vehicle, IA_Interact, EKeys::Gamepad_FaceButton_Top);

	IA_VehicleThrottle = MakeAction(TEXT("IA_VehicleThrottle"), EInputActionValueType::Axis1D);
	IA_VehicleSteer    = MakeAction(TEXT("IA_VehicleSteer"), EInputActionValueType::Axis1D);
	IA_VehicleBrake    = MakeAction(TEXT("IA_VehicleBrake"), EInputActionValueType::Boolean);
	IA_VehicleLook     = MakeAction(TEXT("IA_VehicleLook"), EInputActionValueType::Axis2D);
	for (UInputAction* Action : { IA_VehicleThrottle.Get(), IA_VehicleSteer.Get(), IA_VehicleBrake.Get(), IA_VehicleLook.Get() })
	{
		Action->bConsumeInput = false;
	}
	Map(IMC_Vehicle, IA_VehicleThrottle, EKeys::W);
	Map(IMC_Vehicle, IA_VehicleThrottle, EKeys::S, EMod::Negate);
	Map(IMC_Vehicle, IA_VehicleThrottle, EKeys::Gamepad_RightTriggerAxis);
	Map(IMC_Vehicle, IA_VehicleThrottle, EKeys::Gamepad_LeftTriggerAxis, EMod::Negate);
	Map(IMC_Vehicle, IA_VehicleSteer, EKeys::D);
	Map(IMC_Vehicle, IA_VehicleSteer, EKeys::A, EMod::Negate);
	MapStick(IMC_Vehicle, IA_VehicleSteer, EKeys::Gamepad_LeftX, 1.f);
	Map(IMC_Vehicle, IA_VehicleBrake, EKeys::SpaceBar);
	Map(IMC_Vehicle, IA_VehicleBrake, EKeys::Gamepad_FaceButton_Bottom);
	Map(IMC_Vehicle, IA_VehicleLook, EKeys::Mouse2D);
	MapStick(IMC_Vehicle, IA_VehicleLook, EKeys::Gamepad_Right2D, 150.f, true);
}

void UCEInputSubsystem::ApplyOnFootContexts()
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Input)
	{
		return;
	}
	Input->RemoveMappingContext(IMC_Vehicle);
	Input->AddMappingContext(IMC_Default, 0);
	Input->AddMappingContext(IMC_Combat, 1);
}

void UCEInputSubsystem::ApplyVehicleContexts()
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Input)
	{
		return;
	}
	Input->RemoveMappingContext(IMC_Default);
	Input->RemoveMappingContext(IMC_Combat);
	Input->AddMappingContext(IMC_Vehicle, 2);
}
