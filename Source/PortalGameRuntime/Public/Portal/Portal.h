// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

class ALyraCharacter;
class APortalPlayerController;
class UBoxComponent;

UCLASS()
class PORTALGAMERUNTIME_API APortal : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	APawn* InstigatingPawn;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	bool bIsExitPortal;
	
	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn=true))
	FGameplayEffectSpecHandle PortalEffectSpecHandle;

	UPROPERTY(EditAnywhere)
	float ClipModifier;

	APortal();

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable)
	void SetPortalA(APortal* Portal, bool IsExitPortal);

	UFUNCTION(BlueprintCallable)
	void SetPortalB(APortal* Portal, bool IsExitPortal);
	
	UFUNCTION(BlueprintCallable)
	void SetConnectedPortal(APortal* Portal);

	UFUNCTION(BlueprintCallable)
	APortal* GetConnectedPortal();

	UFUNCTION()
	void SetPortalSurface(AActor* Surface);

	UFUNCTION()
	AActor* GetPortalSurface();
	
	UFUNCTION()
	void SetPortalColor(FColor Color);
	
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSetPortalColor(FColor Color);

	UFUNCTION()
	void SetPortalRender(APortal* Current, APortal* Connected);

protected:

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> PortalPlane;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> PortalCollider;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<USceneCaptureComponent2D> PortalCaptureComponent;
	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UTextureRenderTarget2D> PortalRender;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterial> InitialPortalMaterial; // Must set in BP
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PortalMaterial;

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPortalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void UpdateDeltaRotator();

	UFUNCTION()
	void UpdatePortalRenderRotation();

	UFUNCTION()
	void UpdatePortalClipPlane(APortal* Current, APortal* Connected);

	UFUNCTION(BlueprintCallable)
	void TeleportActor(AActor* OtherActor, UPrimitiveComponent* OtherComp);
	
	UFUNCTION()
	void TeleportPlayer(ALyraCharacter* Character);

	UFUNCTION()
	void UpdatePlayerVelocity(ALyraCharacter* Character);

	UFUNCTION()
	void UpdatePlayerRotation(ALyraCharacter* Character);

	UFUNCTION()
	void UpdateLastTeleportedActor();

	UFUNCTION()
	void OnRep_PortalColor();

private:

	TObjectPtr<APortal> PortalA;
	TObjectPtr<APortal> PortalB;
	TObjectPtr<APortalPlayerController> OwningController;

	UPROPERTY(ReplicatedUsing ="OnRep_PortalColor")
	FColor PortalColor;
	
	UPROPERTY(VisibleAnywhere, Replicated)
	TObjectPtr<APortal> CurrentPortal;

	UPROPERTY(VisibleAnywhere, Replicated)
	TObjectPtr<APortal> ConnectedPortal;

	UPROPERTY(Replicated)
	FRotator NewRenderRotation = FRotator::ZeroRotator;

	UPROPERTY()
	TObjectPtr<AActor> PortalSurface;

	UPROPERTY()
	FVector Velocity;
	
	UPROPERTY()
	FVector SavedVelocity;
	
	UPROPERTY()
	FVector FinalVelocity;
	
	UPROPERTY()
	AActor* LastTeleportedActor;
	
	UPROPERTY()
	FRotator FinalPlayerRotation;
	
	UPROPERTY(Replicated)
	FRotator DeltaRotator;
	
};
