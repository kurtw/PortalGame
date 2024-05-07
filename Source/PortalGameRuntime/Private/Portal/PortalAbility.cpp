// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal/PortalAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Player/PortalPlayerController.h"

void UPortalAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData)
	{
		FGameplayTag EventTagInput = TriggerEventData->EventTag;
		APortalPlayerController* OwningController = Cast<APortalPlayerController>(TriggerEventData->Instigator->GetOwner());
		FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TriggerEventData->TargetData, 0);
		
		SpawnPortal(HitResult, EventTagInput, OwningController);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot activate ability. Check that TriggerEventData is not NULL."));
	}
}

/*
 * At least in UE5.1, there was an issue with OnAbilityRemoved not being properly called in blueprint.
 * The issue was that an ability instance would be marked for garbage collection before
 * the blueprint function K2_OnAbilityRemoved is called, which resulted in the blueprint event
 * OnAbilityRemoved never being called.
 *
 * This issue was reported by various users as resolved by release of UE5.2, but I encountered it during development.
 * In an attempt to resolve the issue, the below code will delay garbage collection by one tick.
 */
void UPortalAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (!IsValidChecked(this)){
		const UWorld* const World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
		if (World){
			this->ClearGarbage();
			FTimerManager& TimerManager = World->GetTimerManager();
			TimerManager.SetTimerForNextTick(FTimerDelegate::CreateLambda([this]() {
				this->MarkAsGarbage();
			}));
		}
	}
	
	K2_OnAbilityRemoved();
	
	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UPortalAbility::SpawnPortal(const FHitResult& HitResult, FGameplayTag EventTag, APortalPlayerController* Controller)
{
	FGameplayTag TagPortalA = FGameplayTag::RequestGameplayTag("Event.PortalA");
	FGameplayTag TagPortalB = FGameplayTag::RequestGameplayTag("Event.PortalB");
	
	if (EventTag == TagPortalA && GetAvatarActorFromActorInfo()->HasAuthority())
	{
		PortalA = Controller->SpawnPortalA(HitResult);
		FinishSpawningPortal(PortalA, HitResult);
	}
	if (EventTag == TagPortalB && GetAvatarActorFromActorInfo()->HasAuthority())
	{
		PortalB = Controller->SpawnPortalB(HitResult);
		FinishSpawningPortal(PortalB, HitResult);
	}
}

void UPortalAbility::FinishSpawningPortal(APortal* Portal, FHitResult HitResult)
{
	const UAbilitySystemComponent* SourceAsc = GetAbilitySystemComponentFromActorInfo();
	FGameplayEffectContextHandle EffectContextHandle = SourceAsc->MakeEffectContext();
	
	EffectContextHandle.AddHitResult(HitResult);
	const FGameplayEffectSpecHandle SpecHandle = SourceAsc->MakeOutgoingSpec(PortalEffectClass, GetAbilityLevel(), EffectContextHandle);

	if (Portal)
	{
		Portal->PortalEffectSpecHandle = SpecHandle;
		Portal->FinishSpawning(Portal->GetTransform());

		if (Portal->GetConnectedPortal() && GetAvatarActorFromActorInfo()->HasAuthority())
		{
			Portal->SetPortalRender(Portal, Portal->GetConnectedPortal());
		}
	}
}