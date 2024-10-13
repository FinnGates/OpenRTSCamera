// Copyright 2024 Bright Helm Studios Ltd All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "CameraBlockingBox.generated.h"

class UBoxComponent;

UCLASS()
class OPENRTSCAMERA_API ACameraBlockingBox : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACameraBlockingBox();

	void SetBlockingBoxSize(const FVector& NewSize) const {BlockingBox->SetBoxExtent(NewSize, true);}
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(EditAnywhere)
	UBoxComponent* BlockingBox;
};
