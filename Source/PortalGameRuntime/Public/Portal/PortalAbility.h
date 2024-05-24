// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "PortalAbility.generated.h"

class APortalPlayerController;
class APortal;

/**
 * 
 */
UCLASS()
class PORTALGAMERUNTIME_API UPortalAbility : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:

	UPortalAbility();

protected:
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								const FGameplayAbilityActorInfo* ActorInfo,
								const FGameplayAbilityActivationInfo ActivationInfo,
								const FGameplayEventData* TriggerEventData) override;
	
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<APortal> PortalClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> PortalEffectClass;

	UFUNCTION(BlueprintCallable)
	void SpawnPortal(const FHitResult& HitResult, FGameplayTag EventTag, APortalPlayerController* Controller);

	void FinishSpawningPortal(APortal* Portal, FHitResult HitResult);

private:

	UPROPERTY()
	TObjectPtr<APortal> PortalA;

	UPROPERTY()
	TObjectPtr<APortal> PortalB;
};
