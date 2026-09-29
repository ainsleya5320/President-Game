#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FourYearsPlayerController.generated.h"

class SFourYearsScreen;
class SFourYearsIntro;
class SFourYearsDiplomacy;
class STextBlock;
class SWidget;

// Mouse look, the on-screen prompt, and the president's screen: E at the Resolute Desk opens the
// briefing, E by the sofas opens the advisers, P opens the policies and C opens Congress anywhere.
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
	void OpenPolicies();

	UFUNCTION(BlueprintCallable, Category = "Four Years")
	void OpenAdvisers();

	UFUNCTION(BlueprintCallable, Category = "Four Years")
	void OpenCongress();

	UFUNCTION(BlueprintCallable, Category = "Four Years|Diplomacy")
	void OpenWorldMap();

	UFUNCTION(BlueprintCallable, Category = "Four Years")
	void Interact();

	UFUNCTION(BlueprintPure, Category = "Four Years")
	FString GetInteractionPrompt() const;

	UFUNCTION(BlueprintCallable, Category = "Four Years")
	void CloseScreen();

	UFUNCTION(BlueprintPure, Category = "Four Years")
	bool IsScreenOpen() const { return Screen.IsValid() || Intro.IsValid() || WorldMap.IsValid(); }

	UFUNCTION(BlueprintCallable, Category = "Four Years|Introduction")
	void OpenInauguration();
	UFUNCTION(BlueprintCallable, Category = "Four Years|Introduction")
	void SkipInauguration();
	UFUNCTION(BlueprintCallable, Category = "Four Years|Introduction")
	void FinishInauguration();
	UFUNCTION(BlueprintPure, Category = "Four Years|Introduction")
	int32 GetInaugurationStage() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	float LookSensitivity = .12f;

private:
	void OpenScreen(uint8 Page);
	void ShowWorldMap();

	TSharedPtr<SWidget> Prompt;
	TSharedPtr<STextBlock> PromptText;
	TSharedPtr<SFourYearsScreen> Screen;
	TSharedPtr<SFourYearsIntro> Intro;
	TSharedPtr<SFourYearsDiplomacy> WorldMap;
	bool bWorldMapRequested = false;
};
