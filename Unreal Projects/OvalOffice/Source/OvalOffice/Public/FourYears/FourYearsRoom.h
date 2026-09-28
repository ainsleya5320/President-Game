#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FourYearsRoom.generated.h"

// Authored with each level, so new rooms do not require changing the walker.
USTRUCT(BlueprintType)
struct FFourYearsRoomInteraction
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Position;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Radius = 110.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Prompt;
	// travel, adviser, or briefing
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Action;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Target;
};

UCLASS()
class OVALOFFICE_API AFourYearsRoom : public AActor
{
	GENERATED_BODY()
public:
	AFourYearsRoom();
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString RoomLabel = TEXT("THE OVAL OFFICE");
	// Empty retains the original Oval Office floor plan. Rectangles form a union.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FBox2D> WalkableAreas;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FBox2D> Obstacles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FFourYearsRoomInteraction> Interactions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector ArrivalPosition = FVector(70, 310, 165);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ArrivalYaw = -90.f;
	const FFourYearsRoomInteraction* FindInteraction(const FVector& Location) const;
};
