// Copyright Finn Gates. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraBlockingBox.generated.h"

UCLASS()
class OPENRTSCAMERA_API ACameraBlockingBox : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACameraBlockingBox();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(EditAnywhere)
	class UBoxComponent* BlockingBox;
};
