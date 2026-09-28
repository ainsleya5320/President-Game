#include "FourYears/FourYearsGameMode.h"

#include "FourYears/FourYearsPlayerController.h"
#include "FourYears/FourYearsWalker.h"

AFourYearsGameMode::AFourYearsGameMode()
{
	DefaultPawnClass = AFourYearsWalker::StaticClass();
	PlayerControllerClass = AFourYearsPlayerController::StaticClass();
}
