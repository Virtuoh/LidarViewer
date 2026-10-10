// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceTexture.h"
#include "Engine/Texture2D.h"
#include "UUDPReceiver.h"
#include "PointCloudRenderer.generated.h"

UCLASS()
class UNREALLIDARVIEWER_API APointCloudRenderer : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APointCloudRenderer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* PointCloudRenderer;

private:

	UNiagaraComponent* rendererInstance;

	UTexture2D* colorTexture;
	UTexture2D* positionTexture;

	uint32_t textureWidth;
	uint32_t textureHeight;
	uint32_t pointCount;

	float* positions;
	uint8_t* colors;

	FUpdateTextureRegion2D region;

	UPROPERTY()
	UUDPReceiver* UDPReceiverRef;

	void HandleNewFrame(const TArray<FLidarPoint>& FramePoints);

	FColor IntensityToColor(float Intensity) const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
