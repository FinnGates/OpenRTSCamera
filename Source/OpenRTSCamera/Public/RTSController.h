// Copyright 2024 Bright Helm Studios Ltd All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTSController.generated.h"

/** Forward declaration to improve compiling times */
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;

UCLASS()
class OPENRTSCAMERA_API ARTSController : public APlayerController
{
	GENERATED_BODY()
public:
	ARTSController();
	
};
