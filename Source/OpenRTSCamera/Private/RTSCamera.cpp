// Copyright 2022 Jesus Bracho All Rights Reserved.

#include "RTSCamera.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

URTSCamera::URTSCamera()
{
	PrimaryComponentTick.bCanEverTick = true;
	CameraBlockingVolumeTag = FName("OpenRTSCamera#CameraBounds");
	CollisionChannel = ECC_WorldStatic;
	DragExtent = 0.6f;
	DistanceFromEdgeThreshold = 0.1f;
	EnableCameraLag = true;
	EnableCameraRotationLag = true;
	EnableDynamicCameraHeight = true;
	EnableEdgeScrolling = true;
	FindGroundTraceLength = 100000;
	MaximumZoomLength = 5000;
	MinimumZoomLength = 500;
	MoveSpeed = 50;
	RotateSpeed = 45;
	StartingYAngle = -45.0f;
	StartingZAngle = 0;
	ZoomCatchupSpeed = 4;
	ZoomSpeed = -200;
}

void URTSCamera::BeginPlay()
{
	Super::BeginPlay();

	const auto NetMode = GetNetMode();
	if (NetMode != NM_DedicatedServer)
	{
		CollectComponentDependencyReferences();
		ConfigureSpeeds();
		ConfigureSpringArm();
		TryToFindBoundaryVolumeReference();
		ConditionallyEnableEdgeScrolling();
		CheckForEnhancedInputComponent();
		BindInputMappingContext();
		BindInputActions();
	}
}

void URTSCamera::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const auto NetMode = GetNetMode();
	if (NetMode != NM_DedicatedServer && PlayerController->GetViewTarget() == Owner)
	{
		DeltaSeconds = DeltaTime;
		SmoothTargetArmLengthToDesiredZoom();
		ZoomRatio = (SpringArm->TargetArmLength- MinimumZoomLength)/(MaximumZoomLength - MinimumZoomLength);
		ApplyMoveCameraCommands();
		ConditionallyPerformEdgeScrolling();
		ConditionallyKeepCameraAtDesiredZoomAboveGround();
		ApplyCameraRotationToDesiredZoom();
		FollowTargetIfSet();
		ConditionallyApplyCameraBounds();
	}
}

void URTSCamera::FollowTarget(AActor* Target)
{
	CameraFollowTarget = Target;
}

void URTSCamera::UnFollowTarget()
{
	CameraFollowTarget = nullptr;
}

void URTSCamera::OnZoomCamera(const FInputActionValue& Value)
{
	DesiredZoomLength = FMath::Clamp(
		DesiredZoomLength + Value.Get<float>() * ZoomSpeed,
		MinimumZoomLength,
		MaximumZoomLength
	);
}

void URTSCamera::OnMaxZoomCamera(const FInputActionValue& Value)
{
	DesiredZoomLength = MaximumZoomLength;
}

void URTSCamera::OnMinZoomCamera(const FInputActionValue& Value)
{
	DesiredZoomLength = MinimumZoomLength;
}

void URTSCamera::OnRotateCamera(const FInputActionValue& Value)
{
	const FRotator WorldRotation = Root->GetComponentRotation();
	Root->SetWorldRotation(
		FRotator::MakeFromEuler(
			FVector(
				WorldRotation.Euler().X,
				WorldRotation.Euler().Y,
				WorldRotation.Euler().Z + (Value.Get<float>() * RotateSpeed * DeltaSeconds)
			)
		)
	);
}

void URTSCamera::OnTurnCameraLeft(const FInputActionValue&)
{
	const auto WorldRotation = Root->GetRelativeRotation();
	Root->SetRelativeRotation(
		FRotator::MakeFromEuler(
			FVector(
				WorldRotation.Euler().X,
				WorldRotation.Euler().Y,
				WorldRotation.Euler().Z - 180.f
			)
		)
	);
}

void URTSCamera::OnTurnCameraRight(const FInputActionValue&)
{
	const auto WorldRotation = Root->GetRelativeRotation();
	Root->SetRelativeRotation(
		FRotator::MakeFromEuler(
			FVector(
				WorldRotation.Euler().X,
				WorldRotation.Euler().Y,
				WorldRotation.Euler().Z + 180.f
			)
		)
	);
}

void URTSCamera::OnMoveCameraYAxis(const FInputActionValue& Value)
{
	RequestMoveCamera(
		SpringArm->GetForwardVector().X,
		SpringArm->GetForwardVector().Y,
		Value.Get<float>()
	);
}

void URTSCamera::OnMoveCameraXAxis(const FInputActionValue& Value)
{
	RequestMoveCamera(
		SpringArm->GetRightVector().X,
		SpringArm->GetRightVector().Y,
		Value.Get<float>()
	);
}

void URTSCamera::OnDragCamera(const FInputActionValue& Value)
{
	if (!IsDragging && Value.Get<bool>())
	{
		IsDragging = true;
		DragStartLocation = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	}

	else if (IsDragging && Value.Get<bool>())
	{
		const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
		auto DragExtents = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
		DragExtents *= DragExtent;

		auto Delta = MousePosition - DragStartLocation;
		Delta.X = FMath::Clamp(Delta.X, -DragExtents.X, DragExtents.X) / DragExtents.X;
		Delta.Y = FMath::Clamp(Delta.Y, -DragExtents.Y, DragExtents.Y) / DragExtents.Y;

		RequestMoveCamera(
			SpringArm->GetRightVector().X,
			SpringArm->GetRightVector().Y,
			Delta.X
		);

		RequestMoveCamera(
			SpringArm->GetForwardVector().X,
			SpringArm->GetForwardVector().Y,
			Delta.Y * -1
		);
	}

	else if (IsDragging && !Value.Get<bool>())
	{
		IsDragging = false;
	}
}

void URTSCamera::OnJumpCamera(const FInputActionValue& Value)
{
	FHitResult HitResult;
	if (PlayerController->GetHitResultUnderCursor(CollisionChannel, true, HitResult))
	{
		JumpTo(HitResult.ImpactPoint);
	}
}

void URTSCamera::OnChangeMoveSpeed(const FInputActionValue& Value)
{
	const float DeltaMoveSpeed = FMath::RoundToFloat(Value.Get<float>()*100);
	MoveSpeed = FMath::Clamp(MoveSpeed + DeltaMoveSpeed, MinimumMoveSpeed, MaximumMoveSpeed);
}

void URTSCamera::OnChangeRotateSpeed(const FInputActionValue& Value)
{
	const float DeltaRotateSpeed = FMath::RoundToFloat(Value.Get<float>()*10);
	RotateSpeed = FMath::Clamp(RotateSpeed + DeltaRotateSpeed, MinimumRotateSpeed, MaximumRotateSpeed);
}

void URTSCamera::RequestMoveCamera(const float X, const float Y, const float Scale)
{
	FMoveCameraCommand MoveCameraCommand;
	MoveCameraCommand.X = X;
	MoveCameraCommand.Y = Y;
	MoveCameraCommand.Scale = Scale;
	MoveCameraCommands.Push(MoveCameraCommand);
}

void URTSCamera::ApplyMoveCameraCommands()
{
	for (const auto& [X, Y, Scale] : MoveCameraCommands)
	{
		auto Movement = FVector2D(X, Y);
		Movement.Normalize();
		Movement *= MoveSpeed * Scale * DeltaSeconds * FMath::Clamp(ZoomRatio, MinimumZoomedSpeed, ZoomRatio);
		Root->SetWorldLocation(
			Root->GetComponentLocation() + FVector(Movement.X, Movement.Y, 0.0f)
		);
	}

	MoveCameraCommands.Empty();
}

void URTSCamera::CollectComponentDependencyReferences()
{
	Owner = GetOwner();
	Root = Owner->GetRootComponent();
	Camera = Cast<UCameraComponent>(Owner->GetComponentByClass(UCameraComponent::StaticClass()));
	SpringArm = Cast<USpringArmComponent>(Owner->GetComponentByClass(USpringArmComponent::StaticClass()));
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
}

void URTSCamera::ConfigureSpringArm()
{
	DesiredZoomLength = MaximumZoomLength;
	SpringArm->TargetArmLength = DesiredZoomLength;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = EnableCameraLag;
	SpringArm->bEnableCameraRotationLag = EnableCameraRotationLag;
	SpringArm->SetRelativeRotation(
			FRotator::MakeFromEuler(
				FVector(
					0.0,
					StartingYAngle,
					StartingZAngle
				)
			)
		);
}

void URTSCamera::ConfigureSpeeds()
{
	const float MoveSpeedDiff = MaximumMoveSpeed - MinimumMoveSpeed;
	MoveSpeed = MinimumMoveSpeed + (MoveSpeedDiff/2);
	const float RotateSpeedDiff = MaximumRotateSpeed - MinimumRotateSpeed;
	RotateSpeed = MinimumRotateSpeed + (RotateSpeedDiff/2);
}

void URTSCamera::TryToFindBoundaryVolumeReference()
{
	TArray<AActor*> BlockingVolumes;
	UGameplayStatics::GetAllActorsOfClassWithTag(
		GetWorld(),
		AActor::StaticClass(),
		CameraBlockingVolumeTag,
		BlockingVolumes
	);

	if (BlockingVolumes.Num() > 0)
	{
		BoundaryVolume = BlockingVolumes[0];
	}
}

void URTSCamera::ConditionallyEnableEdgeScrolling() const
{
	if (EnableEdgeScrolling)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
	}
}

void URTSCamera::CheckForEnhancedInputComponent() const
{
	if (Cast<UEnhancedInputComponent>(PlayerController->InputComponent) == nullptr)
	{
		UKismetSystemLibrary::PrintString(
			GetWorld(),
			TEXT("Set Edit > Project Settings > Input > Default Classes to Enhanced Input Classes"), true, true,
			FLinearColor::Red,
			100
		);

		UKismetSystemLibrary::PrintString(
			GetWorld(),
			TEXT("Keyboard inputs will probably not function."), true, true,
			FLinearColor::Red,
			100
		);

		UKismetSystemLibrary::PrintString(
			GetWorld(),
			TEXT("Error: Enhanced input component not found."), true, true,
			FLinearColor::Red,
			100
		);
	}
}

void URTSCamera::BindInputMappingContext() const
{
	PlayerController->bShowMouseCursor = true;
	const auto Subsystem = PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	Subsystem->AddMappingContext(InputMappingContext, 0);
}

void URTSCamera::BindInputActions()
{
	if (const auto EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
	{
		EnhancedInputComponent->BindAction(
			ZoomCamera,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnZoomCamera
		);

		EnhancedInputComponent->BindAction(
			MaxZoomCamera,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnMaxZoomCamera
		);
		
		EnhancedInputComponent->BindAction(
			MinZoomCamera,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnMinZoomCamera
		);

		EnhancedInputComponent->BindAction(
			RotateCameraAxis,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnRotateCamera
		);

		EnhancedInputComponent->BindAction(
			TurnCameraLeft,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnTurnCameraLeft
		);

		EnhancedInputComponent->BindAction(
			TurnCameraRight,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnTurnCameraRight
		);

		EnhancedInputComponent->BindAction(
			MoveCameraXAxis,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnMoveCameraXAxis
		);

		EnhancedInputComponent->BindAction(
			MoveCameraYAxis,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnMoveCameraYAxis
		);

		EnhancedInputComponent->BindAction(
			JumpCamera,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnJumpCamera
		);

		EnhancedInputComponent->BindAction(
			ChangeRotateSpeed,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnChangeRotateSpeed
		);

		EnhancedInputComponent->BindAction(
			ChangeMoveSpeed,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnChangeMoveSpeed
		);

		EnhancedInputComponent->BindAction(
			DragCamera,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnDragCamera
		);
	}
}

void URTSCamera::SetActiveCamera() const
{
	PlayerController->SetViewTarget(GetOwner());
}

void URTSCamera::JumpTo(const FVector Position) const
{
	Root->SetWorldLocation(Position);
}

void URTSCamera::ConditionallyPerformEdgeScrolling() const
{
	if (EnableEdgeScrolling && !IsDragging)
	{
		EdgeScrollLeft();
		EdgeScrollRight();
		EdgeScrollUp();
		EdgeScrollDown();
	}
}

void URTSCamera::EdgeScrollLeft() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = 1 - UKismetMathLibrary::NormalizeToRange(
		MousePosition.X,
		0.0f,
		ViewportSize.X * 0.05f
	);

	const auto Movement = UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);

	// Root->AddRelativeLocation(
	// 	-1 * Root->GetRightVector() * Movement * EdgeScrollSpeed * DeltaSeconds
	// );

	const auto WorldRotation = Root->GetComponentRotation();
	Root->SetWorldRotation(
		FRotator::MakeFromEuler(
			FVector(
				WorldRotation.Euler().X,
				WorldRotation.Euler().Y,
				WorldRotation.Euler().Z + Movement * RotateSpeed * DeltaSeconds
			)
		)
	);
}

void URTSCamera::EdgeScrollRight() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = UKismetMathLibrary::NormalizeToRange(
		MousePosition.X,
		ViewportSize.X * 0.95f,
		ViewportSize.X
	);

	 const auto Movement = UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);

	const auto WorldRotation = Root->GetComponentRotation();
	Root->SetWorldRotation(
		FRotator::MakeFromEuler(
			FVector(
				WorldRotation.Euler().X,
				WorldRotation.Euler().Y,
				WorldRotation.Euler().Z - Movement * RotateSpeed * DeltaSeconds
			)
		)
	);
}

void URTSCamera::EdgeScrollUp() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = UKismetMathLibrary::NormalizeToRange(
		MousePosition.Y,
		0.0f,
		ViewportSize.Y * 0.05f
	);

	const auto Movement = 1 - UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);
	Root->AddRelativeLocation(
		Root->GetForwardVector() * Movement * MoveSpeed * DeltaSeconds * FMath::Clamp(ZoomRatio, MinimumZoomedSpeed, ZoomRatio)
	);
}

void URTSCamera::EdgeScrollDown() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = UKismetMathLibrary::NormalizeToRange(
		MousePosition.Y,
		ViewportSize.Y * 0.95f,
		ViewportSize.Y
	);

	const auto Movement = UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);
	Root->AddRelativeLocation(
		-1 * Root->GetForwardVector() * Movement * MoveSpeed * DeltaSeconds * FMath::Clamp(ZoomRatio, MinimumZoomedSpeed, ZoomRatio)
	);
}

void URTSCamera::FollowTargetIfSet() const
{
	if (CameraFollowTarget != nullptr)
	{
		Root->SetWorldLocation(CameraFollowTarget->GetActorLocation());
	}
}

void URTSCamera::SmoothTargetArmLengthToDesiredZoom() const
{
	SpringArm->TargetArmLength = FMath::FInterpTo(
		SpringArm->TargetArmLength,
		DesiredZoomLength,
		DeltaSeconds,
		ZoomCatchupSpeed
	);
}

void URTSCamera::ApplyCameraRotationToDesiredZoom() const
{
	if (StartingYAngle != FinishingYAngle)
	{
		const float TotalAngleDiff = StartingYAngle - FinishingYAngle;
		const float NewAngle = FinishingYAngle + (TotalAngleDiff * ZoomRatio);
		SpringArm->SetRelativeRotation(
			FRotator::MakeFromEuler(
				FVector(
					0.0,
					NewAngle,
					SpringArm->GetRelativeRotation().Vector().Z
				)
			)
		);
	}
	if (StartingZAngle != FinishingZAngle)
	{
		const float TotalAngleDiff =  StartingZAngle - FinishingZAngle;
		const float NewAngle = FinishingZAngle + (TotalAngleDiff * ZoomRatio);
		SpringArm->SetRelativeRotation(
			FRotator::MakeFromEuler(
				FVector(
					0.0,
					SpringArm->GetRelativeRotation().Vector().Y,
					NewAngle
				)
			)
		);
	}
}

void URTSCamera::ConditionallyKeepCameraAtDesiredZoomAboveGround()
{
	if (EnableDynamicCameraHeight)
	{
		const auto RootWorldLocation = Root->GetComponentLocation();
		const TArray<AActor*> ActorsToIgnore;

		auto HitResult = FHitResult();
		auto DidHit = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(),
			FVector(RootWorldLocation.X, RootWorldLocation.Y, RootWorldLocation.Z + FindGroundTraceLength),
			FVector(RootWorldLocation.X, RootWorldLocation.Y, RootWorldLocation.Z - FindGroundTraceLength),
			UEngineTypes::ConvertToTraceType(CollisionChannel),
			true,
			ActorsToIgnore,
			EDrawDebugTrace::Type::None,
			HitResult,
			true
		);

		if (DidHit)
		{
			Root->SetWorldLocation(
				FVector(
					HitResult.Location.X,
					HitResult.Location.Y,
					HitResult.Location.Z
				)
			);
		}

		else if (!IsCameraOutOfBoundsErrorAlreadyDisplayed)
		{
			IsCameraOutOfBoundsErrorAlreadyDisplayed = true;

			UKismetSystemLibrary::PrintString(
				GetWorld(),
				"Or add a `RTSCameraBoundsVolume` actor to the scene.",
				true,
				true,
				FLinearColor::Red,
				100
			);

			UKismetSystemLibrary::PrintString(
				GetWorld(),
				"Increase trace length or change the starting position of the parent actor for the spring arm.",
				true,
				true,
				FLinearColor::Red,
				100
			);

			UKismetSystemLibrary::PrintString(
				GetWorld(),
				"Error: AC_RTSCamera needs to be placed on the ground!",
				true,
				true,
				FLinearColor::Red,
				100
			);
		}
	}
}

void URTSCamera::ConditionallyApplyCameraBounds() const
{
	if (BoundaryVolume != nullptr)
	{
		const auto RootWorldLocation = Root->GetComponentLocation();
		FVector Origin;
		FVector Extents;
		BoundaryVolume->GetActorBounds(false, Origin, Extents);
		Root->SetWorldLocation(
			FVector(
				UKismetMathLibrary::Clamp(RootWorldLocation.X, Origin.X - Extents.X, Origin.X + Extents.X),
				UKismetMathLibrary::Clamp(RootWorldLocation.Y, Origin.Y - Extents.Y, Origin.Y + Extents.Y),
				RootWorldLocation.Z
			)
		);
	}
}
