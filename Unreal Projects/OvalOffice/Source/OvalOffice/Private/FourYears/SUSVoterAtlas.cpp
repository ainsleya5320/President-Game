#include "SUSVoterAtlas.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
namespace {
using V=FourYears::JsonValue;
FString Str(const V& J,const char* K){return UTF8_TO_TCHAR(J.Get(K).AsString().c_str());}
FVector2D Pt(const V& J){return FVector2D(J[0].NumberOr(0),J[1].NumberOr(0));}
FBox2D Box(const V& J){return FBox2D(Pt(J),FVector2D(J[2].NumberOr(0),J[3].NumberOr(0)));}
bool Inside(const TArray<FVector2D>& Ring,FVector2D P){bool B=false;for(int I=0,J=Ring.Num()-1;I<Ring.Num();J=I++){const auto A=Ring[I],Z=Ring[J];if(((A.Y>P.Y)!=(Z.Y>P.Y))&&P.X<(Z.X-A.X)*(P.Y-A.Y)/(Z.Y-A.Y)+A.X)B=!B;}return B;}
bool Overlap(const FSlateRect& A,const FSlateRect& B){return A.Left<B.Right&&A.Right>B.Left&&A.Top<B.Bottom&&A.Bottom>B.Top;}
FLinearColor Hex(const FString& S){return FLinearColor::FromSRGBColor(FColor::FromHex(S));}
FLinearColor Mix(FLinearColor A,FLinearColor B,double T){return FMath::Lerp(A,B,float(FMath::Clamp(T,0.,1.)));}
}
void SUSVoterAtlas::Construct(const FArguments& Args){
 OnSelect=Args._OnSelect;SetClipping(EWidgetClipping::ClipToBoundsAlways);SetCursor(EMouseCursor::GrabHand);
 FString Json;V Data;std::string Why;
 if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("FourYears/Electorate/us.json")))||!V::Parse(TCHAR_TO_UTF8(*Json),Data,Why)){Error=TEXT("U.S. atlas could not be loaded.");return;}
 auto Load=[&](const char* Key,TArray<FAtlasCountry>& Out){for(const auto& C:Data.Get(Key).Items()){
  FAtlasCountry A;A.Id=Str(C,"id");A.Name=Str(C,"name");A.Short=Str(C,"state");A.Label=Pt(C.Get("label"));A.Focus=Box(C.Get("focus"));
  for(const auto& P:C.Get("polygons").Items()){FAtlasPolygon Poly;Poly.Bounds=Box(P.Get("bounds"));for(const auto& R:P.Get("rings").Items()){TArray<FVector2D> Ring;for(const auto& Q:R.Items())Ring.Add(Pt(Q));Poly.Rings.Add(MoveTemp(Ring));}for(const auto& Q:P.Get("v").Items())Poly.Vertices.Add(Pt(Q));for(const auto& I:P.Get("i").Items())Poly.Indices.Add(static_cast<SlateIndex>(I.NumberOr(0)));A.Polygons.Add(MoveTemp(Poly));}Out.Add(MoveTemp(A));
 }};Load("counties",Counties);Load("states",States);
 for(const auto& C:Data.Get("cities").Items()){FAtlasCity A;A.Name=Str(C,"name");A.Position=Pt(C.Get("p"));A.Rank=int(C.Get("rank").NumberOr(0));A.Capital=C.Get("capital").Truthy();Cities.Add(MoveTemp(A));}
 Cities.StableSort([](const FAtlasCity& A,const FAtlasCity& B){return A.Capital!=B.Capital?A.Capital:A.Rank<B.Rank;});
 UE_LOG(LogTemp,Display,TEXT("US voter atlas loaded: %d counties, %d states/DC, %d named cities."),Counties.Num(),States.Num(),Cities.Num());
}
void SUSVoterAtlas::SetData(const V& D,const V& View,int Layer){
 Colors.Reset();Descriptions.Reset();TMap<FString,const V*> Results;for(const auto& R:View.Get("counties").Items())Results.Add(Str(R,"id"),&R);
 TMap<FString,FLinearColor> Culture;for(const auto& C:D.Get("cultures").Items())Culture.Add(Str(C,"id"),Hex(Str(C,"color")));
 const bool Republican=View.Get("party").AsString()=="republican";
 for(const auto& C:D.Get("counties").Items()){
  const auto Id=Str(C,"id");const auto* Found=Results.Find(Id);if(!Found)continue;const auto& R=**Found;FLinearColor Color;
  if(Layer==1){Color=FLinearColor(0,0,0,1);for(std::size_t I=0;I<C.Get("cultures").Size();++I){const auto* Hue=Culture.Find(UTF8_TO_TCHAR(C.Get("cultures").Keys()[I].c_str()));if(Hue)Color+=*Hue*float(C.Get("cultures").Values()[I].AsNumber());}Color.A=1;}
  else if(Layer==2)Color=Mix(Hex(TEXT("344558")),Hex(TEXT("b6e3b5")),(R.Get("turnout").AsNumber()-35)/45);
  else if(Layer==3)Color=Mix(Hex(TEXT("415170")),Hex(TEXT("e6c478")),(C.Get("income").AsNumber()-30000)/80000);
  else if(Layer==4)Color=Mix(Hex(TEXT("434864")),Hex(TEXT("8bd5cc")),C.Get("college").AsNumber()/.65);
  else if(Layer==5)Color=Mix(Hex(TEXT("547c87")),Hex(TEXT("e9a061")),C.Get("poverty").AsNumber()/35);
  else if(Layer==6)Color=Mix(Hex(TEXT("72505d")),Hex(TEXT("88ccba")),(R.Get("approval").AsNumber()-30)/40);
  else {const double Dem=Layer==7?C.Get("rawDem2024").AsNumber():(Republican?100-R.Get("support").AsNumber():R.Get("support").AsNumber());Color=Dem>=50?Mix(Hex(TEXT("c0c9d6")),Hex(TEXT("397ac1")),(Dem-50)/35):Mix(Hex(TEXT("d8c5c4")),Hex(TEXT("bc4e5e")),(50-Dem)/35);}
  Colors.Add(Id,Color);Descriptions.Add(Id,FString::Printf(TEXT("%s, %s  |  Approval %.1f%%  |  Your vote %.1f%%"),*Str(C,"fullName"),*Str(C,"state"),R.Get("approval").AsNumber(),R.Get("support").AsNumber()));
 }
 Invalidate(EInvalidateWidgetReason::Paint);
}
const FAtlasCountry* SUSVoterAtlas::County(const FString& Id)const{for(const auto& C:Counties)if(C.Id==Id)return &C;return nullptr;}
const FAtlasCountry* SUSVoterAtlas::Hit(FVector2D P)const{for(const auto& C:Counties)for(const auto& A:C.Polygons){if(!A.Bounds.IsInsideOrOn(P)||A.Rings.IsEmpty()||!Inside(A.Rings[0],P))continue;bool Hole=false;for(int I=1;I<A.Rings.Num();++I)if(Inside(A.Rings[I],P)){Hole=true;break;}if(!Hole)return &C;}return nullptr;}
FVector2D SUSVoterAtlas::Extent(FVector2D S)const{const double W=FMath::Min(S.X,S.Y*1.58);return FVector2D(W,W/1.58)*Zoom;}
FVector2D SUSVoterAtlas::Screen(FVector2D P,FVector2D S)const{return S*.5+(P-Center)*Extent(S);}
FVector2D SUSVoterAtlas::Map(FVector2D P,FVector2D S)const{return Center+(P-S*.5)/Extent(S);}
void SUSVoterAtlas::ClampCenter(){const double H=.5/Zoom;Center.X=FMath::Clamp(Center.X,H,1-H);Center.Y=FMath::Clamp(Center.Y,H,1-H);}
void SUSVoterAtlas::ZoomBy(double F){Zoom=FMath::Clamp(Zoom*F,1.,18.);ClampCenter();}
void SUSVoterAtlas::Focus(const FString& Id){const FAtlasCountry* C=County(Id);if(!C)for(const auto& S:States)if(S.Id==Id){C=&S;break;}if(C){const auto D=C->Focus.GetSize();Zoom=FMath::Clamp(.7/FMath::Max(D.X,D.Y),1.2,16.);Center=C->Focus.GetCenter();ClampCenter();}}
bool SUSVoterAtlas::Search(const FString& Query){
 FString Q=Query.TrimStartAndEnd();if(Q.IsEmpty())return false;
 for(const auto& S:States)if(S.Id.Equals(Q,ESearchCase::IgnoreCase)||S.Name.Equals(Q,ESearchCase::IgnoreCase)){Focus(S.Id);for(const auto& C:Counties)if(C.Short==S.Id){Selected=C.Id;OnSelect.ExecuteIfBound(Selected);break;}return true;}
 for(const auto& C:Cities)if(C.Name.Equals(Q,ESearchCase::IgnoreCase)){Center=C.Position;Zoom=6;ClampCenter();if(const auto* A=Hit(C.Position)){Selected=A->Id;OnSelect.ExecuteIfBound(Selected);}return true;}
 for(const auto& C:Counties)if(C.Id==Q||C.Name.Equals(Q,ESearchCase::IgnoreCase)||(C.Name+TEXT(", ")+C.Short).Equals(Q,ESearchCase::IgnoreCase)){Selected=C.Id;Focus(C.Id);OnSelect.ExecuteIfBound(Selected);return true;}return false;
}
FReply SUSVoterAtlas::OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();Press=Last=G.AbsoluteToLocal(E.GetScreenSpacePosition());Dragging=true;Moved=false;return FReply::Handled().CaptureMouse(SharedThis(this));}
FReply SUSVoterAtlas::OnMouseButtonUp(const FGeometry& G,const FPointerEvent& E){if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();if(Dragging&&!Moved)if(const auto* C=Hit(Map(G.AbsoluteToLocal(E.GetScreenSpacePosition()),G.GetLocalSize()))){Selected=C->Id;OnSelect.ExecuteIfBound(Selected);}Dragging=false;return FReply::Handled().ReleaseMouseCapture();}
FReply SUSVoterAtlas::OnMouseButtonDoubleClick(const FGeometry& G,const FPointerEvent& E){if(E.GetEffectingButton()==EKeys::LeftMouseButton)if(const auto* C=Hit(Map(G.AbsoluteToLocal(E.GetScreenSpacePosition()),G.GetLocalSize()))){Selected=C->Id;Focus(C->Short);OnSelect.ExecuteIfBound(Selected);}return FReply::Handled();}
FReply SUSVoterAtlas::OnMouseMove(const FGeometry& G,const FPointerEvent& E){const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());if(Dragging&&HasMouseCapture()){if(FVector2D::Distance(P,Press)>4)Moved=true;if(Moved){Center-=(P-Last)/Extent(G.GetLocalSize());ClampCenter();}Last=P;return FReply::Handled();}const auto* C=Hit(Map(P,G.GetLocalSize()));Hovered=C?C->Id:TEXT("");const auto* T=Descriptions.Find(Hovered);SetToolTipText(FText::FromString(T?*T:TEXT("Scroll to zoom; drag to pan; click a county.")));return FReply::Handled();}
FReply SUSVoterAtlas::OnMouseWheel(const FGeometry& G,const FPointerEvent& E){const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());const auto Before=Map(P,G.GetLocalSize());ZoomBy(FMath::Pow(1.22,E.GetWheelDelta()));Center+=Before-Map(P,G.GetLocalSize());ClampCenter();return FReply::Handled();}
int32 SUSVoterAtlas::OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const{
 const auto S=G.GetLocalSize();const auto* White=FCoreStyle::Get().GetBrush("WhiteBrush");const auto Gold=Hex(TEXT("f2ca7f")),Ink=Hex(TEXT("f3f2df")),Ocean=Hex(TEXT("101f30"));
 FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(),White,ESlateDrawEffect::None,Ocean);
 auto Visible=[&](const FBox2D& B){auto A=Screen(B.Min,S),Z=Screen(B.Max,S);return Z.X>=0&&Z.Y>=0&&A.X<=S.X&&A.Y<=S.Y;};
 auto Lines=[&](const TArray<FVector2D>& R,FLinearColor C,float W,int L,bool Closed=true){TArray<FVector2D> Points;for(const auto P:R)Points.Add(Screen(P,S));if(Closed&&!Points.IsEmpty()){const FVector2D First=Points[0];Points.Add(First);}FSlateDrawElement::MakeLines(Out,Layer+L,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,C,true,W);};
 // Batch filled meshes into bounded index buffers, instead of thousands of draw calls.
 TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;const auto Resource=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White);
 auto Flush=[&](){if(!Indices.IsEmpty())FSlateDrawElement::MakeCustomVerts(Out,Layer+1,Resource,Vertices,Indices,nullptr,0,0);Vertices.Reset();Indices.Reset();};
 for(const auto& C:Counties){const auto* Hue=Colors.Find(C.Id);FLinearColor Color=Hue?*Hue:Hex(TEXT("7c8fa3"));if(C.Id==Hovered)Color=Mix(Color,Ink,.22);const FColor Tint=Color.ToFColor(true);
  for(const auto& P:C.Polygons){if(!Visible(P.Bounds))continue;if(Vertices.Num()+P.Vertices.Num()>60000)Flush();const int Offset=Vertices.Num();for(const auto Q:P.Vertices)Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),FVector2f(Screen(Q,S)),FVector2f(.5f,.5f),Tint));for(const auto I:P.Indices)Indices.Add(static_cast<SlateIndex>(Offset+I));}
 }Flush();
 for(const auto& C:Counties)for(const auto& P:C.Polygons){if(!Visible(P.Bounds))continue;const bool SelectedCounty=C.Id==Selected;if(Zoom>1.7||SelectedCounty||C.Id==Hovered)for(const auto& R:P.Rings)Lines(R,SelectedCounty?Gold:FLinearColor(.015f,.028f,.04f,.4f),SelectedCounty?2.3f:.6f,SelectedCounty?4:2);}
 for(const auto& C:States)for(const auto& P:C.Polygons)if(Visible(P.Bounds))for(const auto& R:P.Rings)Lines(R,FLinearColor(.84f,.87f,.86f,.82f),Zoom>3?1.7f:1.05f,3);
 TArray<FSlateRect> Occupied;const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
 auto Label=[&](FVector2D P,const FString& T,int Size,FLinearColor Color,bool Centered){auto Font=FCoreStyle::GetDefaultFontStyle("Bold",Size);const auto D=Measure->Measure(T,Font);if(Centered)P-=D*.5;FSlateRect R(P.X-3,P.Y-2,P.X+D.X+3,P.Y+D.Y+2);if(R.Left<3||R.Top<3||R.Right>S.X-3||R.Bottom>S.Y-3)return false;for(const auto& O:Occupied)if(Overlap(R,O))return false;Occupied.Add(R);FSlateDrawElement::MakeText(Out,Layer+6,G.ToPaintGeometry(D,FSlateLayoutTransform(P+FVector2D(1,1))),T,Font,ESlateDrawEffect::None,Ocean);FSlateDrawElement::MakeText(Out,Layer+7,G.ToPaintGeometry(D,FSlateLayoutTransform(P)),T,Font,ESlateDrawEffect::None,Color);return true;};
 if(CitiesOn&&Zoom>1.6)for(const auto& C:Cities){if(C.Rank>3&&Zoom<3.5)continue;if(C.Rank>5&&Zoom<6)continue;auto P=Screen(C.Position,S);if(Label(P+FVector2D(6,-6),C.Name,11,Ink,false)){FSlateDrawElement::MakeBox(Out,Layer+5,G.ToPaintGeometry(FVector2D(4,4),FSlateLayoutTransform(P-FVector2D(2,2))),White,ESlateDrawEffect::None,Gold);}}
 for(const auto& C:States){if(!Visible(C.Focus))continue;Label(Screen(C.Label,S),Zoom>3?C.Name.ToUpper():C.Id,Zoom>2?14:11,Ink,true);}
 if(Zoom<1.5){Label(Screen(FVector2D(.185,.975),S),TEXT("ALASKA  /  INSET"),10,Gold,true);Label(Screen(FVector2D(.40,.975),S),TEXT("HAWAII  /  INSET"),10,Gold,true);Label(Screen(FVector2D(.61,.9),S),TEXT("GULF OF MEXICO"),11,Hex(TEXT("647f98")),true);}
 if(!Error.IsEmpty())Label(S*.5,Error,18,Gold,true);return Layer+7;
}
