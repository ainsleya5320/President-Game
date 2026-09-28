#include "FourYears/FourYearsRoom.h"
#include "Components/SceneComponent.h"

AFourYearsRoom::AFourYearsRoom()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

const FFourYearsRoomInteraction* AFourYearsRoom::FindInteraction(const FVector& Location) const
{
	const FFourYearsRoomInteraction* Nearest = nullptr;
	double Distance = TNumericLimits<double>::Max();
	for (const FFourYearsRoomInteraction& Interaction : Interactions)
	{
		const double Candidate = FVector2D::Distance(FVector2D(Location.X, Location.Y), Interaction.Position);
		if (Candidate < Interaction.Radius && Candidate < Distance)
		{
			Nearest = &Interaction;
			Distance = Candidate;
		}
	}
	return Nearest;
}
