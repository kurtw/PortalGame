// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal/Portal.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Character/LyraCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/PortalPlayerController.h"

APortal::APortal()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");
	SetRootComponent(RootComponent);

	PortalPlane = CreateDefaultSubobject<UStaticMeshComponent>("PortalPlane");
	PortalPlane->SetCollisionObjectType(ECC_WorldStatic);
	PortalPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalPlane->SetupAttachment(RootComponent);

	PortalCollider = CreateDefaultSubobject<UBoxComponent>("PortalCollider");
	PortalCollider->SetBoxExtent(FVector(25, 60, 100));
	PortalCollider->SetupAttachment(RootComponent);

	// Settings for optimization of PortalCaptureComponent
	FPostProcessSettings CaptureSettings;
	CaptureSettings.bOverride_AmbientOcclusionQuality = true;
	CaptureSettings.bOverride_MotionBlurAmount = true;
	CaptureSettings.bOverride_SceneFringeIntensity = true;
	CaptureSettings.bOverride_FilmGrainIntensity = true;
	CaptureSettings.bOverride_ScreenSpaceReflectionQuality = true;
	
	CaptureSettings.AmbientOcclusionQuality = 0.0f;
	CaptureSettings.MotionBlurAmount = 0.0f;
	CaptureSettings.SceneFringeIntensity = 0.0f;
	CaptureSettings.FilmGrainIntensity = 0.0f;
	CaptureSettings.ScreenSpaceReflectionQuality = 0.0f;
	// end Settings for optimization of PortalCaptureComponent

	PortalCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>("PortalCaptureComponent");
	PortalCaptureComponent->SetupAttachment(RootComponent);
	PortalCaptureComponent->bCaptureEveryFrame = false; // If false, must update CaptureComponent in Tick()
	PortalCaptureComponent->bCaptureOnMovement = false;
	/*
	 * If false, when a Niagara System in active / visible (i.e. NS_GunPad):
	 * WIN --> LogRenderer: Display: Recreating Shadow.Virtual.PhysicalPagePool due to size or flag changes. This will also drop and cached pages.
	 * macOS --> No change to visible flickering. 
	*/
	PortalCaptureComponent->bAlwaysPersistRenderingState = true;
	PortalCaptureComponent->TextureTarget = nullptr;
	PortalCaptureComponent->bEnableClipPlane = true;
	PortalCaptureComponent->bUseCustomProjectionMatrix = true;
	PortalCaptureComponent->LODDistanceFactor = 3.f;
	// PortalCaptureComponent->FOVAngle = 120.f;
	PortalCaptureComponent->CaptureSource = SCS_SceneColorSceneDepth;
	PortalCaptureComponent->PostProcessSettings = CaptureSettings;

	// Initialize
	InstigatingPawn = nullptr;
	bIsExitPortal = false;
	PortalEffectSpecHandle = nullptr;
	ClipModifier = 1.5f;
	PortalPlane = nullptr;
	PortalCollider = nullptr;
	PortalCaptureComponent = nullptr;
	PortalRender = nullptr;
	InitialPortalMaterial = nullptr;
	PortalMaterial = nullptr;
	PortalA = nullptr;
	PortalB = nullptr;
	OwningController = nullptr;
	PortalColor = FColor::Red;
	CurrentPortal  = nullptr;
	ConnectedPortal = nullptr;
	NewRenderRotation = FRotator::ZeroRotator;
	PortalSurface = nullptr;
	Velocity = FVector::ZeroVector;
	SavedVelocity = FVector::ZeroVector;
	FinalVelocity = FVector::ZeroVector;
	LastTeleportedActor = nullptr;
	FinalPlayerRotation = FRotator::ZeroRotator;
	DeltaRotator = FRotator::ZeroRotator;
	// end Initialize
}

void APortal::BeginPlay()
{
	Super::BeginPlay();

	// Must be set if the portal mesh is not a plane, otherwise the back of the portals will block the view
	PortalPlane->bHiddenInSceneCapture = true;

	PortalRender = NewObject<UTextureRenderTarget2D>(this, UTextureRenderTarget2D::StaticClass(), FName("PortalRender"));
	PortalRender->InitAutoFormat(1920, 1080);
	PortalRender->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	
	if (InitialPortalMaterial)
	{
		PortalMaterial = UMaterialInstanceDynamic::Create(InitialPortalMaterial, this);
		PortalMaterial->SetTextureParameterValue(FName("PortalRenderTexture"), PortalRender);
		PortalPlane->SetMaterial(0, PortalMaterial);
	}
	else
	{
		UE_LOG(LogTemp, Type::Error, TEXT("Check that InitialPortalMaterial is set in the portal blueprint."));
	}

	OwningController = Cast<APortalPlayerController>(GetOwner());
	SetPortalColor(PortalColor);

	PortalCollider->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnPortalOverlap);

}

void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentPortal && ConnectedPortal)
	{
		UpdateDeltaRotator();
		UpdatePortalRenderRotation();
	}
}

void APortal::OnPortalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!PortalEffectSpecHandle.Data.IsValid() || PortalEffectSpecHandle.Data->GetEffectContext().GetEffectCauser() != OtherActor)
	{
		return;
	}
	if (HasAuthority())
	{
		if (UAbilitySystemComponent* OtherActorAsc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			OtherActorAsc->ApplyGameplayEffectSpecToSelf(*PortalEffectSpecHandle.Data.Get());
		}
	}
}

void APortal::UpdateDeltaRotator()
{
	if (ConnectedPortal)
	{
		FRotator NewPortalRotation = ConnectedPortal->GetActorRotation() + FRotator(0, -180, 0);
		DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(CurrentPortal->GetActorRotation(), NewPortalRotation);
	}
}

void APortal::UpdatePortalRenderRotation()
{
	if (OwningController)
	{
		if (CurrentPortal && ConnectedPortal)
		{
			if (GetConnectedPortal()->PortalCaptureComponent != nullptr || this->PortalCaptureComponent != nullptr)
			{
				/// TODO: Fix portal render issue produced from the commented code.
				/// FVector NewLocation = TargetLocation + NewDirection; does not work as intended.
				/// The player can rotate the camera or move far enough away from the portal such
				/// that they can see under the floor.
				/// The current solution is to only use FVector NewLocation = TargetLocation;
				/// with PortalCaptureComponent->FOVAngle = 120.f; added to the constructor,
				/// but the view from the portals magnified in a way that does not look good.
				
				// FVector Direction = OwningController->GetPawn()->GetActorLocation() - this->GetActorLocation();
				FVector TargetLocation = GetConnectedPortal()->GetActorLocation();
				
				// FVector DotProduct;
				// DotProduct.X = FVector::DotProduct(Direction, this->GetActorForwardVector());
				// DotProduct.Y = FVector::DotProduct(Direction, this->GetActorRightVector());
				// DotProduct.Z = FVector::DotProduct(Direction, this->GetActorUpVector());
				//
				// FVector NewDirection = DotProduct.X * -GetConnectedPortal()->GetActorForwardVector()
				// 	+ DotProduct.Y * -GetConnectedPortal()->GetActorRightVector()
				// 	+ DotProduct.Z * GetConnectedPortal()->GetActorUpVector();
				
				// FVector NewLocation = TargetLocation + NewDirection;
				FVector NewLocation = TargetLocation;
				
				FVector PortalForward = this->GetActorForwardVector();
				FVector ConnectedForward = GetConnectedPortal()->GetActorForwardVector();
				
				FQuat RotationQuat = FQuat::FindBetweenNormals(PortalForward, ConnectedForward).GetNormalized();
				FQuat NewWorldQuat = RotationQuat * this->GetActorQuat().GetNormalized();
				
				if (GetConnectedPortal()->GetPortalSurface() != nullptr && this->GetPortalSurface() != nullptr)
				{
					float ConnectedDeltaRoll = GetConnectedPortal()->GetPortalSurface()->GetActorRotation().Roll - GetConnectedPortal()->PortalCaptureComponent->GetComponentRotation().Roll;
					float CurrentDeltaRoll = this->GetPortalSurface()->GetActorRotation().Roll - this->PortalCaptureComponent->GetComponentRotation().Roll;
					if (CurrentDeltaRoll != 0.f || ConnectedDeltaRoll != 0.f)
					{
						NewRenderRotation = FRotator(NewWorldQuat.Rotator().Pitch, NewWorldQuat.Rotator().Yaw, 0.f);
				
					}
					else
					{
						NewRenderRotation = NewWorldQuat.Rotator();
					}
				}
				
				GetConnectedPortal()->PortalCaptureComponent->SetWorldLocationAndRotation(NewLocation, NewRenderRotation.Quaternion());
				UpdatePortalClipPlane(this, GetConnectedPortal());
						
				if (OwningController->IsLocalPlayerController())
				{
					GetConnectedPortal()->PortalCaptureComponent->CustomProjectionMatrix = OwningController->GetCameraProjectionMatrix();
				}
				/// TODO: Maybe not needed? Check in multiplayer first
				GetConnectedPortal()->PortalCaptureComponent->TextureTarget = this->PortalRender;
				GetConnectedPortal()->PortalCaptureComponent->CaptureScene();
			}
		}
	}
}

void APortal::UpdatePortalClipPlane(APortal* Current, APortal* Connected)
{
	Connected->PortalCaptureComponent->ClipPlaneNormal = Connected->GetActorForwardVector();
	Connected->PortalCaptureComponent->ClipPlaneBase = Connected->GetActorLocation() + (Current->PortalCaptureComponent->ClipPlaneNormal * -ClipModifier);
}

void APortal::TeleportActor(AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	SavedVelocity = FVector::ZeroVector;
	FinalVelocity = FVector::ZeroVector;
	
	if (ConnectedPortal)
	{
		if (ALyraCharacter* Character = Cast<ALyraCharacter>(OtherComp->GetOwner()))
		{
			bool bIsLastActorOfConnectedPortal = ConnectedPortal->LastTeleportedActor != OtherComp->GetOwner();
			bool bIsLastActorOfThisPortal = CurrentPortal->LastTeleportedActor != OtherComp->GetOwner();
			if (bIsLastActorOfConnectedPortal && bIsLastActorOfThisPortal)
			{
				TeleportPlayer(Character);
			}
		}
	}
}

void APortal::TeleportPlayer(ALyraCharacter* Character)
{
	ConnectedPortal->LastTeleportedActor = Character;
	SavedVelocity = Character->GetCharacterMovement()->Velocity;
	Character->SetActorLocation(ConnectedPortal->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	
	UpdatePlayerVelocity(Character);
	UpdatePlayerRotation(Character);
	UpdateLastTeleportedActor();
}

void APortal::UpdatePlayerVelocity(ALyraCharacter* Character)
{
	if (SavedVelocity.Length() > 600.f)
	{
		FVector ThisXVel = FVector::ZeroVector;
		FVector ThisYVel = FVector::ZeroVector;
		FVector ThisZVel = FVector::ZeroVector;
		UKismetMathLibrary::BreakRotIntoAxes(GetActorRotation(), ThisXVel, ThisYVel, ThisZVel);
		
		FRotator RotationInverse = UKismetMathLibrary::NegateRotator(UKismetMathLibrary::MakeRotationFromAxes(ThisXVel*-1.f, ThisYVel*-1.f, ThisZVel));
		FRotator NewRotation = UKismetMathLibrary::ComposeRotators(RotationInverse, ConnectedPortal->GetRootComponent()->GetComponentRotation());
		FinalVelocity = UKismetMathLibrary::GreaterGreater_VectorRotator(SavedVelocity, NewRotation);
		
		Character->GetCharacterMovement()->Velocity = FinalVelocity;
	}

	// Keep minimum player velocity as default, 600.f.
	float SelectVelocity = UKismetMathLibrary::SelectFloat(SavedVelocity.Length(), 600.f, SavedVelocity.Length() > 600.f);
	FVector ConnectedPortalForward = UKismetMathLibrary::Normal(ConnectedPortal->GetActorForwardVector());
	FinalVelocity = UKismetMathLibrary::Multiply_VectorFloat(ConnectedPortalForward, SelectVelocity);
	Character->GetCharacterMovement()->Velocity = FinalVelocity;
}

void APortal::UpdatePlayerRotation(ALyraCharacter* Character)
{
	if (Character)
	{
		if (Character->GetController())
		{
			FVector CurrentPortalForward = GetRootComponent()->GetForwardVector();
			FVector NewPortalForward = ConnectedPortal->GetRootComponent()->GetForwardVector();
	
			if (FMath::Abs(FVector::DotProduct(CurrentPortalForward, NewPortalForward)) > 0.95f) // Close to parallel
			{
				FinalPlayerRotation = FRotator(
					Character->GetController()->GetControlRotation().Pitch,
					Character->GetController()->GetControlRotation().Yaw - DeltaRotator.Yaw,
					Character->GetController()->GetControlRotation().Roll);
			}
			else // Not parallel
			{
				FinalPlayerRotation = FRotator(
					Character->GetController()->GetControlRotation().Pitch - DeltaRotator.Pitch,
					Character->GetController()->GetControlRotation().Yaw - DeltaRotator.Yaw,
					Character->GetController()->GetControlRotation().Roll);
			}

			Character->GetController()->SetControlRotation(FinalPlayerRotation);
		}
	}
}

void APortal::UpdateLastTeleportedActor()
{
	if (ConnectedPortal)
	{
		ConnectedPortal->LastTeleportedActor = nullptr;
	}
}

void APortal::SetPortalA(APortal* Portal, bool IsExitPortal)
{
	if (Portal != nullptr)
	{
		PortalA = Portal;
		bIsExitPortal = IsExitPortal;
		if (ALyraPlayerController* PC = Cast<ALyraPlayerController>(GetOwner()))
		{
			InstigatingPawn = PC->GetPawn();
			SetInstigator(InstigatingPawn);
		}
	}
}

void APortal::SetPortalB(APortal* Portal, bool IsExitPortal)
{
	if (Portal != nullptr)
	{
		PortalB = Portal;
		bIsExitPortal = IsExitPortal;
		if (ALyraPlayerController* PC = Cast<ALyraPlayerController>(GetOwner()))
		{
			InstigatingPawn = PC->GetPawn();
			SetInstigator(InstigatingPawn);
		}
	}
}

void APortal::SetConnectedPortal(APortal* Portal)
{
	if (Portal != nullptr)
	{
		ConnectedPortal = Portal;
	}
}

APortal* APortal::GetConnectedPortal()
{
	return this->ConnectedPortal.Get();
}

void APortal::SetPortalSurface(AActor* Surface)
{
	PortalSurface = Surface;
}

AActor* APortal::GetPortalSurface()
{
	return this->PortalSurface.Get();
}

void APortal::SetPortalColor(FColor Color)
{
	if (HasAuthority())
	{
		PortalColor = Color;
		OnRep_PortalColor();
		MulticastSetPortalColor(Color);
	}
}

void APortal::MulticastSetPortalColor_Implementation(FColor Color)
{
	PortalColor = Color;
	OnRep_PortalColor();
}

void APortal::SetPortalRender(APortal* Current, APortal* Connected)
{
	if (Current && Connected)
	{
		Current->CurrentPortal = Current;
		Current->ConnectedPortal = Connected;
		Current->PortalCaptureComponent->TextureTarget = Connected->PortalRender;
	
		Connected->CurrentPortal = Connected;
		Connected->ConnectedPortal = Current;
		Connected->PortalCaptureComponent->TextureTarget = Current->PortalRender;
	}
}

void APortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(APortal, PortalColor);
	DOREPLIFETIME(APortal, CurrentPortal);
	DOREPLIFETIME(APortal, ConnectedPortal);
	DOREPLIFETIME(APortal, NewRenderRotation);
	DOREPLIFETIME(APortal, DeltaRotator);
	DOREPLIFETIME(APortal, bIsExitPortal);
	DOREPLIFETIME(APortal, InstigatingPawn);

}

void APortal::OnRep_PortalColor()
{
	if (PortalMaterial)
	{
		PortalMaterial->SetVectorParameterValue(FName("Color"), PortalColor);
	}
}