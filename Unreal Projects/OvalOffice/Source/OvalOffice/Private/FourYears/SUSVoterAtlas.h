#pragma once
#include "SWorldAtlas.h"
// Census county geometry in Albers equal-area, with separate Alaska/Hawaii insets.
class SUSVoterAtlas : public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SUSVoterAtlas){} SLATE_EVENT(TDelegate<void(FString)>,OnSelect) SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 FVector2D ComputeDesiredSize(float)const override{return FVector2D(900,570);}
 int32 OnPaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool)const override;
 FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent&)override;
 FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&)override;
 FReply OnMouseButtonDoubleClick(const FGeometry&,const FPointerEvent&)override;
 FReply OnMouseMove(const FGeometry&,const FPointerEvent&)override;
 FReply OnMouseWheel(const FGeometry&,const FPointerEvent&)override;
 void OnMouseCaptureLost(const FCaptureLostEvent&)override{Dragging=false;}
 void SetData(const FourYears::JsonValue& Data,const FourYears::JsonValue& View,int Layer);
 void Reset(){Zoom=1;Center=FVector2D(.5,.5);}
 void ZoomBy(double Factor);
 void Focus(const FString& Id);
 bool Search(const FString& Query);
 void Select(const FString& Id){Selected=Id;}
 const FAtlasCountry* County(const FString& Id)const;
 const FAtlasCountry* Hit(FVector2D Point)const;
 double GetZoom()const{return Zoom;}
 bool CitiesOn=true;
 FString Error;
private:
 FVector2D Extent(FVector2D S)const;
 FVector2D Screen(FVector2D P,FVector2D S)const;
 FVector2D Map(FVector2D P,FVector2D S)const;
 void ClampCenter();
 TArray<FAtlasCountry> Counties,States;
 TArray<FAtlasCity> Cities;
 TMap<FString,FLinearColor> Colors;
 TMap<FString,FString> Descriptions;
 TDelegate<void(FString)> OnSelect;
 FString Selected=TEXT("17031"),Hovered;
 FVector2D Center=FVector2D(.5,.5),Press,Last;
 double Zoom=1;
 bool Dragging=false,Moved=false;
};
