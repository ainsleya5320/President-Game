#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FourYearsPresence.generated.h"
class UStaticMeshComponent;
class USceneComponent;
class AFourYearsRoom;
class AFourYearsWalker;
class UMaterialInstanceDynamic;

UCLASS()
class OVALOFFICE_API AFourYearsStaff : public AActor {
 GENERATED_BODY()
public:
 AFourYearsStaff();
 void Build(const FString& Name,const FString& Action,int Style,bool Seated=false);
 virtual void Tick(float DeltaTime) override;
 UPROPERTY(BlueprintReadOnly) FString PersonName;
 UPROPERTY(BlueprintReadOnly) FString Interaction;
 UPROPERTY(BlueprintReadOnly) FString Place;
private:
 UPROPERTY() TObjectPtr<USceneComponent> Head;
 UPROPERTY() TObjectPtr<USceneComponent> Body;
 float Time=0;
 float BaseYaw=0;
 bool bDetailedModel=false;
};

// Lightweight, authored scene additions. Existing imported levels remain untouched.
UCLASS()
class OVALOFFICE_API AFourYearsPresence : public AActor {
 GENERATED_BODY()
public:
 AFourYearsPresence();
 void Build();
 bool Enter(const FString& Place);
 AFourYearsStaff* Nearby(const FVector& Point) const;
 FString CurrentPlace=TEXT("oval");
 UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<AFourYearsStaff>> Staff;
 UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<AFourYearsRoom>> Rooms;
private:
 void Residence();
 void Limo();
 AFourYearsStaff* Person(const FString& Name,const FString& Action,const FString& Place,FVector Position,float Yaw,int Style,bool Seated=false);
 UPROPERTY() TObjectPtr<AFourYearsRoom> MainRoom;
 UPROPERTY() TObjectPtr<AFourYearsRoom> ResidenceRoom;
 UPROPERTY() TObjectPtr<AFourYearsRoom> LimoRoom;
};
