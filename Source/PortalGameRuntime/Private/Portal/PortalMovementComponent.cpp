// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal/PortalMovementComponent.h"
#include "AbilitySystemGlobals.h"
#include "Portal/PortalAttributeSet.h"

UPortalMovementComponent::UPortalMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

float UPortalMovementComponent::GetMaxSpeed() const
{
	if (UAbilitySystemComponent* Asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (MovementMode == MOVE_Walking)
		{
			if (Asc->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
			{
				return 0;
			}

			const float MaxSpeedFromAttribute = Asc->GetNumericAttribute(UPortalAttributeSet::GetMovementSpeedAttribute());
			if (MaxSpeedFromAttribute > 0.0f)
			{
				return MaxSpeedFromAttribute;
			}
		}
	}

	return Super::GetMaxSpeed();
}
