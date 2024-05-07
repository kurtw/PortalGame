// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraAttributeSet.h"
#include "PortalAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class PORTALGAMERUNTIME_API UPortalAttributeSet : public ULyraAttributeSet
{
	GENERATED_BODY()
	
public:
	
	UPortalAttributeSet();

	ATTRIBUTE_ACCESSORS(ThisClass, MovementSpeed);

	//~UAttributeSet interface
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	//~End of UAttributeSet interface

protected:

	UFUNCTION()
	void OnRep_MovementSpeed(const FGameplayAttributeData& OldValue);

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing="OnRep_MovementSpeed", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MovementSpeed;
};
