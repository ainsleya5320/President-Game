#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FourYearsPlayerController.generated.h"

class SFourYearsScreen;
class STextBlock;
class SWidget;

// Mouse look, the on-screen prompt, and the president's screen: E at the Resolute Desk opens the
// briefing, E by the sofas opens the advisers, and P opens the policies anywhere.
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
	void CloseScreen();

	UFUNCTION(BlueprintPure, Category = "Four Years")
	bool IsScreenOpen() const { return Screen.IsValid(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Four Years")
	float LookSensitivity = .12f;

private:
	void OpenScreen(uint8 Page);

	TSharedPtr<SWidget> Prompt;
	TSharedPtr<STextBlock> PromptText;
	TSharedPtr<SFourYearsScreen> Screen;
};
