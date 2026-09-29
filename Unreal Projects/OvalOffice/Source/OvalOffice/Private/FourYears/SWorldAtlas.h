#pragma once
#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "UObject/GCObject.h"
#include "FourYears/Core/FourYearsJson.h"

class UTexture2D;
struct FAtlasPolygon
{
 TArray<TArray<FVector2D>> Rings;
 TArray<FVector2D> Vertices;
 TArray<SlateIndex> Indices;
 FBox2D Bounds;
};
struct FAtlasCountry
{
 FString Id, Name, Short, Continent, Region, Capital;
 FVector2D Label;
 FBox2D Focus;
 double Area=0;
 TArray<FAtlasPolygon> Polygons;
};
struct FAtlasCity
{
 FString Name, Country, Hub;
 FVector2D Position;
 bool Capital=false;
 int Rank=0;
};

// All screen coordinates derive from the same Robinson projection as the raster.
// Country selection uses polygon interiors, including islands and enclave holes.
class SWorldAtlas : public SLeafWidget, public FGCObject
{
public:
 SLATE_BEGIN_ARGS(SWorldAtlas){} SLATE_EVENT(TDelegate<void(FString)>,OnSelect) SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 void AddReferencedObjects(FReferenceCollector& Collector) override;
 FString GetReferencerName() const override{return TEXT("SWorldAtlas");}
 FVector2D ComputeDesiredSize(float) const override{return FVector2D(960,560);}
 int32 OnPaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool) const override;
 FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
 FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&) override;
 FReply OnMouseButtonDoubleClick(const FGeometry&,const FPointerEvent&) override;
 FReply OnMouseMove(const FGeometry&,const FPointerEvent&) override;
 FReply OnMouseWheel(const FGeometry&,const FPointerEvent&) override;
 void OnMouseCaptureLost(const FCaptureLostEvent&) override;
 void SetWorld(const FourYears::JsonValue& Value){World=Value;}
 void Select(const FString& Id){Selected=Id;}
 const FAtlasCountry* Country(const FString& Id) const;
 const FAtlasCountry* HitCountry(FVector2D Point) const;
 void Reset();
 void ZoomBy(double Factor);
 void FocusCountry(const FString& Id);
 bool Search(const FString& Query);
 int Mode=0;
 bool ShowCities=true,ShowGrid=false,ShowRoutes=true;
 FString Error;
 double GetZoom() const{return Zoom;}
 FString GetHover() const{return HoverName;}
private:
 bool Load();
 FVector2D Extent(FVector2D Size) const;
 FVector2D Screen(FVector2D Point,FVector2D Size) const;
 FVector2D Map(FVector2D Point,FVector2D Size) const;
 void ZoomAt(double Factor,FVector2D Point,FVector2D Size);
 void ClampCenter();
 TArray<FAtlasCountry> Countries;
 TArray<FAtlasCity> Cities;
 TArray<TArray<FVector2D>> Grid;
 TObjectPtr<UTexture2D> Political=nullptr;
 TObjectPtr<UTexture2D> Terrain=nullptr;
 FSlateBrush PoliticalBrush,TerrainBrush;
 FourYears::JsonValue World;
 TDelegate<void(FString)> OnSelect;
 FString Selected=TEXT("SAU"),Hovered,HoverName;
 FVector2D Center=FVector2D(.5,.5),Press,Last;
 double Aspect=1.97165,Zoom=1;
 bool Dragging=false,Moved=false;
};
