// Copyright 2022 Jesus Bracho All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "RTSCamera.generated.h"

/**
 * We use these commands so that move camera inputs can be tied to the tick rate of the game.
 * https://github.com/HeyZoos/OpenRTSCamera/issues/27
 */
USTRUCT()
struct FMoveCameraCommand
{
	GENERATED_BODY()
	UPROPERTY()
	float X;
	UPROPERTY()
	float Y;
	UPROPERTY()
	float Scale;
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OPENRTSCAMERA_API URTSCamera : public UActorComponent
{
	GENERATED_BODY()

public:
	URTSCamera();

	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void FollowTarget(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void UnFollowTarget();

	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void SetActiveCamera() const;

protected:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;
	
	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void JumpTo(FVector Position) const;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Zoom Settings")
	float MinimumZoomLength;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Zoom Settings")
	float MaximumZoomLength;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Zoom Settings")
	float ZoomCatchupSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Zoom Settings")
	float ZoomSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Zoom Settings")
	float MinimumZoomedSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float StartingYAngle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float StartingZAngle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float FinishingYAngle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float FinishingZAngle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float MinimumMoveSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float MaximumMoveSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float MinimumRotateSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float MaximumRotateSpeed;
	
	/**
	 * Controls how fast the drag will move the camera.
	 * Higher values will make the camera move more slowly.
	 * The drag speed is calculated as follows:
	 *	DragSpeed = MousePositionDelta / (ViewportExtents * DragExtent)
	 * If the drag extent is small, the drag speed will hit the "max speed" of `this->MoveSpeed` more quickly.
	 */
	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Category = "RTSCamera",
		meta = (ClampMin = "0.0", ClampMax = "1.0")
	)
	float DragExtent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	bool EnableCameraLag;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	bool EnableCameraRotationLag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Dynamic Camera Height Settings")
	bool EnableDynamicCameraHeight;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Dynamic Camera Height Settings")
	TEnumAsByte<ECollisionChannel> CollisionChannel;
	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Category = "RTSCamera - Dynamic Camera Height Settings",
		meta=(EditCondition="EnableDynamicCameraHeight")
	)
	float FindGroundTraceLength;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Edge Scroll Settings")
	bool EnableEdgeScrolling;
	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Category = "RTSCamera - Edge Scroll Settings",
		meta=(EditCondition="EnableEdgeScrolling")
	)
	float DistanceFromEdgeThreshold;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputMappingContext* InputMappingContext;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* RotateCameraAxis;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* TurnCameraLeft;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* TurnCameraRight;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* MoveCameraYAxis;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* MoveCameraXAxis;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* DragCamera;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* ZoomCamera;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* MaxZoomCamera;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* MinZoomCamera;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* JumpCamera;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* ChangeMoveSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* ChangeRotateSpeed;

	virtual void BeginPlay() override;

	void OnZoomCamera(const FInputActionValue& Value);
	void OnMaxZoomCamera(const FInputActionValue& Value);
	void OnMinZoomCamera(const FInputActionValue& Value);
	void OnRotateCamera(const FInputActionValue& Value);
	void OnTurnCameraLeft(const FInputActionValue& Value);
	void OnTurnCameraRight(const FInputActionValue& Value);
	void OnMoveCameraYAxis(const FInputActionValue& Value);
	void OnMoveCameraXAxis(const FInputActionValue& Value);
	void OnChangeMoveSpeed(const FInputActionValue& Value);
	void OnChangeRotateSpeed(const FInputActionValue& Value);
	void OnJumpCamera(const FInputActionValue& Value);
	void OnDragCamera(const FInputActionValue& Value);

	void RequestMoveCamera(float X, float Y, float Scale);
	void ApplyMoveCameraCommands();

	UPROPERTY()
	AActor* Owner;
	UPROPERTY()
	USceneComponent* Root;
	UPROPERTY()
	UCameraComponent* Camera;
	UPROPERTY()
	USpringArmComponent* SpringArm;
	UPROPERTY()
	APlayerController* PlayerController;
	UPROPERTY()
	AActor* BoundaryVolume;
	UPROPERTY()
	float DesiredZoomLength;

private:
	void CollectComponentDependencyReferences();
	void ConfigureSpringArm();
	void ConfigureSpeeds();
	void TryToFindBoundaryVolumeReference();
	void ConditionallyEnableEdgeScrolling() const;
	void CheckForEnhancedInputComponent() const;
	void BindInputMappingContext() const;
	void BindInputActions();

	void ConditionallyPerformEdgeScrolling() const;
	void EdgeScrollLeft() const;
	void EdgeScrollRight() const;
	void EdgeScrollUp() const;
	void EdgeScrollDown() const;

	void FollowTargetIfSet() const;
	void SmoothTargetArmLengthToDesiredZoom() const;
	void ApplyCameraRotationToDesiredZoom() const;
	void ConditionallyKeepCameraAtDesiredZoomAboveGround();
	void ConditionallyApplyCameraBounds() const;

	UPROPERTY()
	FName CameraBlockingVolumeTag;
	UPROPERTY()
	AActor* CameraFollowTarget;
	UPROPERTY()
	float DeltaSeconds;
	UPROPERTY()
	bool IsCameraOutOfBoundsErrorAlreadyDisplayed;
	UPROPERTY()
	bool IsDragging;
	UPROPERTY()
	FVector2D DragStartLocation;
	UPROPERTY()
	TArray<FMoveCameraCommand> MoveCameraCommands;
	UPROPERTY()
	float ZoomRatio;
	UPROPERTY()
	float MoveSpeed;
	UPROPERTY()
	float RotateSpeed;
};
