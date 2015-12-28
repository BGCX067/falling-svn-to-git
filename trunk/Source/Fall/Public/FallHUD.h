// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once 
#include "Fall.h"
#include "GameFramework/HUD.h"
#include "FallHUD.generated.h"

UCLASS()
class AFallHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFallHUD(const FObjectInitializer& ObjectInitializer);

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

