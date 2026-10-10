// Fill out your copyright notice in the Description page of Project Settings.


#include "PointCloudRenderer.h"

constexpr int32 MIN_FRAME_POINTS = 1; // Can be used to only render frames with atleast a certain number of points

// Sets default values
APointCloudRenderer::APointCloudRenderer()
{

	textureWidth = 2000;
	textureHeight = 2000;
	pointCount = 20000;

	colors = new uint8_t[textureWidth * textureHeight * 4];
	positions = new float[textureWidth * textureHeight * 4];

	region = FUpdateTextureRegion2D(0, 0, 0, 0, textureWidth, textureHeight);

	UDPReceiverRef = CreateDefaultSubobject<UUDPReceiver>(TEXT("UDPReceiver"));
}

void SetNiagaraVariableTexture(class UNiagaraComponent* niagara, FString variableName, UTexture* texture) {
	if (!niagara || !texture)
		return;

	FNiagaraUserRedirectionParameterStore& overrideParameters = niagara->GetOverrideParameters();
	FNiagaraVariable niagaraVariable = FNiagaraVariable(FNiagaraTypeDefinition(UNiagaraDataInterfaceTexture::StaticClass()), *variableName);

	UNiagaraDataInterfaceTexture* dataInterface = (UNiagaraDataInterfaceTexture*)overrideParameters.GetDataInterface(niagaraVariable);

	dataInterface->SetTexture(texture);
}

// Called when the game starts or when spawned
void APointCloudRenderer::BeginPlay()
{
	Super::BeginPlay();

	rendererInstance = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		PointCloudRenderer,
		FVector(0, 0, 130),//position of object
		FRotator(0),//rotation, can use to account for lidar rotation
		FVector(1),
		true,
		true,
		ENCPoolMethod::AutoRelease,
		true
	);

	positionTexture = UTexture2D::CreateTransient(textureWidth, textureHeight, PF_A32B32G32R32F, "PositionData");
	positionTexture->Filter = TF_Nearest;
	positionTexture->UpdateResource();

	colorTexture = UTexture2D::CreateTransient(textureWidth, textureHeight, PF_B8G8R8A8, "ColorTexture");
	colorTexture->Filter = TF_Nearest;
	colorTexture->UpdateResource();

	SetNiagaraVariableTexture(rendererInstance, "User.PositionTexture", positionTexture);
	SetNiagaraVariableTexture(rendererInstance, "User.ColorTexture", colorTexture);

	rendererInstance->SetVariableInt("User.TextureWidth", textureWidth);
	rendererInstance->SetVariableInt("User.TextureHeight", textureHeight);
	rendererInstance->SetVariableInt("User.PointCount", pointCount);

	UDPReceiverRef->OnFrameReady.AddUObject(this, &APointCloudRenderer::HandleNewFrame);

	for (int32_t i = 0; i < 20000; i++) {

		colors[i * 4 + 0] = 0;   // R
		colors[i * 4 + 1] = 0;   // G
		colors[i * 4 + 2] = 255;   // B
		colors[i * 4 + 3] = 255; // A (opaque)

	}

}

// Called every frame ------- THIS ONE UPDATES YESYESYESYESYESYESYSEYSEYSE
void APointCloudRenderer::HandleNewFrame(const TArray<FLidarPoint>& FramePoints)
{

	if (FramePoints.Num() > MIN_FRAME_POINTS) {
		UE_LOG(LogTemp, Warning, TEXT("Rendering"));


		for (int32_t i = 0; i < FramePoints.Num(); i++) {

			const FLidarPoint& P = FramePoints[i];

			positions[i * 4 + 0] = P.X * 100.0f;
			positions[i * 4 + 1] = P.Y * 100.0f;
			positions[i * 4 + 2] = -P.Z * 100.0f; //-50 to account for floor position
			positions[i * 4 + 3] = 1.f;

			FColor pointColor = IntensityToColor(P.Intensity); // Map intensity to heatmap
			colors[i * 4 + 0] = pointColor.R;
			colors[i * 4 + 1] = pointColor.G;
			colors[i * 4 + 2] = pointColor.B;
			colors[i * 4 + 3] = pointColor.A;

		}

		for (int32 i = FramePoints.Num(); i < static_cast<int>(pointCount); i++)
		{
			positions[i * 4 + 0] = 99999.f;
			positions[i * 4 + 1] = 99999.f;
			positions[i * 4 + 2] = 99999.f;
			positions[i * 4 + 3] = 1.f;

			colors[i * 4 + 0] = 0;
			colors[i * 4 + 1] = 0;
			colors[i * 4 + 2] = 0;
			colors[i * 4 + 3] = 0;
		}

		float minIntensity = FramePoints[0].Intensity;
		float maxIntensity = FramePoints[0].Intensity;

		for (const FLidarPoint& P : FramePoints)
		{
			if (P.Intensity < minIntensity) minIntensity = P.Intensity;
			if (P.Intensity > maxIntensity) maxIntensity = P.Intensity;
		}

		UE_LOG(LogTemp, Warning, TEXT("Frame received: %d points | Intensity Min: %f | Max: %f"),
			FramePoints.Num(), minIntensity, maxIntensity);

		UE_LOG(LogTemp, Warning, TEXT("Received --- Num: %d"), FramePoints.Num());

		positionTexture->UpdateTextureRegions(0, 1, &region, textureWidth * 16, 16, (uint8*)positions);
		colorTexture->UpdateTextureRegions(0, 1, &region, textureWidth * 4, 4, (uint8*)colors);
	}

}

FColor APointCloudRenderer::IntensityToColor(float Intensity) const
{
	/* 
	// Attempt 1
	// Clamp intensity between 0 and 1
	Intensity = FMath::Clamp(Intensity, 0.f, 1.f);

	if (Intensity < 0.33f) // Blue → Green
	{
		float t = Intensity / 0.33f;
		return FColor(
			0,
			static_cast<uint8>(t * 255),
			static_cast<uint8>((1.f - t) * 255),
			255
		);
	}
	else if (Intensity < 0.66f) // Green → Yellow
	{
		float t = (Intensity - 0.33f) / 0.33f;
		return FColor(
			static_cast<uint8>(t * 255),
			255,
			0,
			255
		);
	}
	else // Yellow → Red
	{
		float t = (Intensity - 0.66f) / 0.34f;
		return FColor(
			255,
			static_cast<uint8>((1.f - t) * 255),
			0,
			255
		);
	}
	*/
	


	// Attempt 2
	// Map 0-255 to 0-1
	float normalized = Intensity / 255.f;

	// Simple blue → green → yellow → red gradient
	if (normalized < 0.33f)
	{
		float t = normalized / 0.33f;
		return FColor(
			0,
			static_cast<uint8>(t * 255),
			static_cast<uint8>((1.f - t) * 255),
			255
		);
	}
	else if (normalized < 0.66f)
	{
		float t = (normalized - 0.33f) / 0.33f;
		return FColor(
			static_cast<uint8>(t * 255),
			255,
			0,
			255
		);
	}
	else
	{
		float t = (normalized - 0.66f) / 0.34f;
		return FColor(
			255,
			static_cast<uint8>((1.f - t) * 255),
			0,
			255
		);
	}
}