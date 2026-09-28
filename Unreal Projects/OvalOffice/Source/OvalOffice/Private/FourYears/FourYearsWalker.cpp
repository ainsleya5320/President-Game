#include "FourYears/FourYearsWalker.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "FourYears/FourYearsRoom.h"
#include "EngineUtils.h"

AFourYearsWalker::AFourYearsWalker()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(75.f);
	// Footprints from the browser game's Oval Office (converted to Unreal centimetres): sofas and coffee
	// table, the Resolute Desk, and the two groups of armchairs.
	Furniture = {
		FBox2D(FVector2D(-250.f, -123.f), FVector2D(-52.f, 123.f)),
		FBox2D(FVector2D(205.f, -165.f), FVector2D(345.f, 165.f)),
		FBox2D(FVector2D(-265.f, -400.f), FVector2D(-18.f, -170.f)),
		FBox2D(FVector2D(-265.f, 170.f), FVector2D(-18.f, 400.f)),
	};
}

void AFourYearsWalker::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AFourYearsRoom> It(GetWorld()); It; ++It) { Room = *It; break; }
	if (Room && Room->WalkableAreas.Num()) Furniture = Room->Obstacles;
	FVector Start = GetActorLocation();
	if (Room && GetWorld()->URL.HasOption(TEXT("FromRoom")))
	{
		Start = Room->ArrivalPosition;
		bApplyArrivalView = true;
	}
	if (!CanStand(FVector2D(Start.X, Start.Y)))
	{
		Start.X = -400.f;
		Start.Y = 0.f;
	}
	Start.Z = EyeHeight;
	SetActorLocation(Start);
}

bool AFourYearsWalker::CanStand(const FVector2D& Position) const
{
	const double X = Position.X / RoomRadii.X, Y = Position.Y / RoomRadii.Y;
	bool bInside = X * X + Y * Y <= WalkableFraction;
	if (Room && Room->WalkableAreas.Num())
	{
		bInside = false;
		for (const FBox2D& Area : Room->WalkableAreas) bInside |= Area.IsInside(Position);
	}
	if (!bInside)
	{
		return false;
	}
	for (const FBox2D& Box : Furniture)
	{
		if (Position.X > Box.Min.X && Position.X < Box.Max.X && Position.Y > Box.Min.Y && Position.Y < Box.Max.Y)
		{
			return false;
		}
	}
	return true;
}

bool AFourYearsWalker::IsNearDesk() const
{
	if (Room && Room->WalkableAreas.Num()) return false;
	return FVector2D::Distance(FVector2D(GetActorLocation().X, GetActorLocation().Y), DeskPosition) < DeskReach;
}

bool AFourYearsWalker::IsNearSittingArea() const
{
	if (Room && Room->WalkableAreas.Num()) return false;
	return FVector2D::Distance(FVector2D(GetActorLocation().X, GetActorLocation().Y), SittingAreaPosition) < SittingAreaReach;
}

void AFourYearsWalker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (bApplyArrivalView && PlayerController && Room)
	{
		PlayerController->SetControlRotation(FRotator(0, Room->ArrivalYaw, 0));
		bApplyArrivalView = false;
	}
	if (!bMovementEnabled || !PlayerController)
	{
		return;
	}
	const auto Axis = [PlayerController](const FKey& Positive, const FKey& Negative) {
		return (PlayerController->IsInputKeyDown(Positive) ? 1.f : 0.f) - (PlayerController->IsInputKeyDown(Negative) ? 1.f : 0.f);
	};
	const float Forward = Axis(EKeys::W, EKeys::S), Right = Axis(EKeys::D, EKeys::A);
	if (Forward == 0.f && Right == 0.f)
	{
		return;
	}
	const FRotator Yaw(0.f, PlayerController->GetControlRotation().Yaw, 0.f);
	FVector Direction = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X) * Forward + FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y) * Right;
	Direction.Z = 0.f;
	const FVector Step = Direction.GetSafeNormal() * WalkSpeed * DeltaSeconds;
	const FVector Current = GetActorLocation();
	// Slide along walls and furniture by trying each axis on its own when the full step is blocked.
	for (const FVector& Candidate : {Current + Step, Current + FVector(Step.X, 0.f, 0.f), Current + FVector(0.f, Step.Y, 0.f)})
	{
		if (CanStand(FVector2D(Candidate.X, Candidate.Y)))
		{
			SetActorLocation(FVector(Candidate.X, Candidate.Y, EyeHeight));
			return;
		}
	}
}
