// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Portal/Portal.h"
#include "Player/LyraPlayerController.h"
#include "PortalPlayerController.generated.h"

struct FGameplayEventData;
struct FGameplayTag;
class APortal;

/**
 * 
 */
UCLASS()
class PORTALGAMERUNTIME_API APortalPlayerController : public ALyraPlayerController
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<APortal> PortalClass;
	
	UPROPERTY(EditAnywhere)
	float PortalDistanceModifier = 2500.f;

	UFUNCTION(BlueprintCallable)
	APortal* SpawnPortalA(FHitResult HitResult);

	UFUNCTION(Server, Unreliable, BlueprintCallable)
	void ServerSpawnPortalA(FHitResult HitResult);
	
	UFUNCTION(BlueprintCallable)
	APortal* SpawnPortalB(FHitResult HitResult);

	UFUNCTION(Server, Unreliable, BlueprintCallable)
	void ServerSpawnPortalB(FHitResult HitResult);

	UFUNCTION(BlueprintCallable)
	APortal* SpawnPortal(APortal* ConnectedPortal, FHitResult HitResult, FColor Color);

	UFUNCTION()
	FHitResult LineTrace();

	UFUNCTION(BlueprintCallable)
	void SendGameplayEventWithPayload(FGameplayTag EventTag, FGameplayEventData& OutPayload);
	
	UFUNCTION(Server, Unreliable)
	void ServerSendGameplayEventWithPayload(AActor* OwningActor, FGameplayTag EventTag, FGameplayEventData Payload);

	FMatrix GetCameraProjectionMatrix();

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY()
	TObjectPtr<APortal> CtrlPortalA;
	
	UPROPERTY()
	TObjectPtr<APortal> CtrlPortalB;
};
