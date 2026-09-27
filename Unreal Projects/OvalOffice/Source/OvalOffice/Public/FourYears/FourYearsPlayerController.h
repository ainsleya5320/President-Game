#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FourYearsPlayerController.generated.h"

class SFourYearsBriefing;
class STextBlock;
class SWidget;

// Mouse look, the desk prompt, and the quarterly briefing screen.
UCLASS()
class OVALOFFICE_API AFourYearsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Four Years")
	void OpenBriefing();

	UFUNCTION(BlueprintCallable, Category = "Four Years")
	void CloseBriefing();

	UFUNCTION(BlueprintPure, Category = "Four Years")
	bool IsBriefingOpen() const { return Briefing.IsValid(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	float LookSensitivity = .12f;

private:
	TSharedPtr<SWidget> Prompt;
	TSharedPtr<STextBlock> PromptText;
	TSharedPtr<SFourYearsBriefing> Briefing;
};
