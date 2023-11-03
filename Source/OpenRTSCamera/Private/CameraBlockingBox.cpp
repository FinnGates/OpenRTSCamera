// Copyright Finn Gates. All Rights Reserved.


#include "CameraBlockingBox.h"

#include "Components/BoxComponent.h"

// Sets default values
ACameraBlockingBox::ACameraBlockingBox()
{
	this->Tags.Add("OpenRTSCamera#CameraBounds");
    
	if (UPrimitiveComponent* PrimitiveComponent = this->FindComponentByClass<UPrimitiveComponent>())
	{
		PrimitiveComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName, false);
	}
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	BlockingBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingBox"));
	SetRootComponent(BlockingBox);
	BlockingBox->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	BlockingBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	BlockingBox->SetCollisionResponseToChannel(ECC_Camera, ECollisionResponse::ECR_Block);
	BlockingBox->SetBoxExtent(FVector::OneVector);
	BlockingBox->SetLineThickness(1.f);
}

// Called when the game starts or when spawned
void ACameraBlockingBox::BeginPlay()
{
	Super::BeginPlay();
	
}

