// First-person presence in the Oval Office. The imported room meshes have no collision, so the walker
// keeps to the floor and to the walkable area of the browser game: inside the oval and clear of the desk,
// sofas and armchairs.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FourYearsWalker.generated.h"

class UCameraComponent;
class AFourYearsRoom;

UCLASS()
class OVALOFFICE_API AFourYearsWalker : public APawn
{
	GENERATED_BODY()

public:
	AFourYearsWalker();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Within reach of the Resolute Desk, where the quarterly briefing opens.
	bool IsNearDesk() const;

	// By the sofas and the fireplace, where advisers meet the president.
	bool IsNearSittingArea() const;
	AFourYearsRoom* GetRoom() const { return Room; }
	UFUNCTION(BlueprintPure, Category = "Four Years")
	bool CanStand(const FVector2D& Position) const;

	// Movement pauses while a screen is open.
	void SetMovementEnabled(bool bEnabled) { bMovementEnabled = bEnabled; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Four Years")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	float WalkSpeed = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	float EyeHeight = 165.f;

	// Half-lengths of the oval floor in centimetres (long axis along X).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	FVector2D RoomRadii = FVector2D(505.f, 408.f);

	// Fraction of the oval that stays walkable, keeping the camera off the walls.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	float WalkableFraction = .94f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	FVector2D DeskPosition = FVector2D(175.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	float DeskReach = 210.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	FVector2D SittingAreaPosition = FVector2D(-315.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	float SittingAreaReach = 170.f;

	// Furniture footprints the walker cannot enter.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	TArray<FBox2D> Furniture;

private:
	UPROPERTY() TObjectPtr<AFourYearsRoom> Room;
	bool bApplyArrivalView = false;
	bool bMovementEnabled = true;
};
