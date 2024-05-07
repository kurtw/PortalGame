// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PortalPlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Portal/Portal.h"

void APortalPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

APortal* APortalPlayerController::SpawnPortalA(FHitResult HitResult)
{
	if (HasAuthority()) // Server
	{
		if (CtrlPortalA != nullptr)
		{
			CtrlPortalB->SetConnectedPortal(nullptr);
			CtrlPortalA->Destroy();
		}
		CtrlPortalA = SpawnPortal(CtrlPortalB, HitResult, FColor::Blue);
	}
	else // Client
	{
		ServerSpawnPortalA(HitResult);
		return nullptr;
	}
	CtrlPortalA->SetPortalA(CtrlPortalA, false);
	return CtrlPortalA;
}

void APortalPlayerController::ServerSpawnPortalA_Implementation(FHitResult HitResult)
{
	SpawnPortalA(HitResult);
}

APortal* APortalPlayerController::SpawnPortalB(FHitResult HitResult)
{
	if (HasAuthority()) // Server
	{
		if (CtrlPortalB != nullptr)
		{
			CtrlPortalA->SetConnectedPortal(nullptr);
			CtrlPortalB->Destroy();
		}
		CtrlPortalB = SpawnPortal(CtrlPortalA, HitResult, FColor::Orange);
	}
	else // Client
	{
		ServerSpawnPortalB(HitResult);
		return nullptr;
	}
	CtrlPortalB->SetPortalB(CtrlPortalB, true);
	return CtrlPortalB;
}

void APortalPlayerController::ServerSpawnPortalB_Implementation(FHitResult HitResult)
{
	SpawnPortalB(HitResult);
}

APortal* APortalPlayerController::SpawnPortal(APortal* ConnectedPortal, FHitResult HitResult, FColor Color)
{
	APortal* Portal = nullptr;
	
	if (PortalClass != nullptr)
	{
		if (HitResult.bBlockingHit)
		{
			FRotator Rotation = UKismetMathLibrary::MakeRotFromXZ(HitResult.Normal, HitResult.Normal);
			const FTransform SpawnTransform(Rotation, HitResult.Location + HitResult.ImpactNormal);
	
			Portal = GetWorld()->SpawnActorDeferred<APortal>(
					PortalClass,
					SpawnTransform,
					this,
					Cast<APawn>(this),
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			
			Portal->SetPortalColor(Color);
			Portal->SetPortalSurface(HitResult.GetActor());
			
			if (ConnectedPortal != nullptr)
			{
				Portal->SetConnectedPortal(ConnectedPortal);
				ConnectedPortal->SetConnectedPortal(Portal);
				
				Portal->SetPortalRender(Portal, ConnectedPortal);
			}
		}
	}
	return Portal;
}

FHitResult APortalPlayerController::LineTrace()
{
	FHitResult OutHit;
	FVector StartLocation = PlayerCameraManager->GetCameraLocation();
	FVector EndLocation = StartLocation + PlayerCameraManager->GetActorForwardVector() * PortalDistanceModifier;
	GetWorld()->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility);

	TArray<AActor*> ActorsToIgnore;
	ETraceTypeQuery TraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	UKismetSystemLibrary::LineTraceSingle(
		this,
		StartLocation,EndLocation,TraceChannel,
		false, ActorsToIgnore,
		EDrawDebugTrace::None,
		OutHit,true);
	
	return OutHit;
}

void APortalPlayerController::SendGameplayEventWithPayload(FGameplayTag EventTag, FGameplayEventData& OutPayload)
{
	FGameplayEventData Payload;
	Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(LineTrace());
	Payload.Instigator = GetPawn();
	Payload.EventTag = EventTag;

	if (!HasAuthority())
	{
		ServerSendGameplayEventWithPayload(GetPawn(), Payload.EventTag, Payload);
	}
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), Payload.EventTag, Payload);
	
	OutPayload = Payload;
}

void APortalPlayerController::ServerSendGameplayEventWithPayload_Implementation(AActor* OwningActor, FGameplayTag EventTag,
	FGameplayEventData Payload)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwningActor, EventTag, Payload);
}

FMatrix APortalPlayerController::GetCameraProjectionMatrix()
{
	FMatrix ProjectionMatrix;
	if (GetLocalPlayer() != nullptr)
	{
		FSceneViewProjectionData ProjectionData;
		GetLocalPlayer()->GetProjectionData(GetLocalPlayer()->ViewportClient->Viewport,ProjectionData);
		
		ProjectionMatrix = ProjectionData.ProjectionMatrix;
	}
	return ProjectionMatrix;
}