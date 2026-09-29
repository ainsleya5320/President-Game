#include "SWorldAtlas.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

namespace {
using V=FourYears::JsonValue;
FString Str(const V& J,const char* K){return UTF8_TO_TCHAR(J.Get(K).AsString().c_str());}
FVector2D Pt(const V& J){return FVector2D(J[0].NumberOr(0),J[1].NumberOr(0));}
FBox2D Box(const V& J){return FBox2D(Pt(J),FVector2D(J[2].NumberOr(0),J[3].NumberOr(0)));}
bool Contains(const TArray<FVector2D>& Ring,FVector2D P){
 bool Inside=false;for(int I=0,J=Ring.Num()-1;I<Ring.Num();J=I++){
  const auto A=Ring[I],B=Ring[J];
  if(((A.Y>P.Y)!=(B.Y>P.Y))&&(P.X<(B.X-A.X)*(P.Y-A.Y)/(B.Y-A.Y)+A.X))Inside=!Inside;
 }return Inside;
}
bool Overlap(const FSlateRect& A,const FSlateRect& B){return A.Left<B.Right&&A.Right>B.Left&&A.Top<B.Bottom&&A.Bottom>B.Top;}
const FLinearColor Gold(1,.76f,.32f),Ink(.93f,.96f,.96f),Dark(.014f,.035f,.053f),Teal(.16f,.88f,.73f),Coral(1,.34f,.27f);
}
void SWorldAtlas::Construct(const FArguments& Args){
 OnSelect=Args._OnSelect;SetClipping(EWidgetClipping::ClipToBoundsAlways);SetCursor(EMouseCursor::GrabHand);Load();
}
void SWorldAtlas::AddReferencedObjects(FReferenceCollector& C){C.AddReferencedObject(Political);C.AddReferencedObject(Terrain);}
bool SWorldAtlas::Load(){
 const FString Root=FPaths::ProjectContentDir()/TEXT("FourYears/Atlas");FString Json;V Data;std::string Why;
 if(!FFileHelper::LoadFileToString(Json,*(Root/TEXT("world.json")))||!V::Parse(TCHAR_TO_UTF8(*Json),Data,Why)){Error=TEXT("Atlas data could not be loaded.");return false;}
 Aspect=Data.Get("aspect").NumberOr(1.97165);
 for(const auto& C:Data.Get("countries").Items()){
  FAtlasCountry A;A.Id=Str(C,"id");A.Name=Str(C,"name");A.Short=Str(C,"short");A.Continent=Str(C,"continent");A.Region=Str(C,"region");A.Capital=Str(C,"capital");A.Label=Pt(C.Get("label"));A.Focus=Box(C.Get("focus"));A.Area=C.Get("area").NumberOr(0);
  for(const auto& P:C.Get("polygons").Items()){
   FAtlasPolygon Poly;Poly.Bounds=Box(P.Get("bounds"));
   for(const auto& R:P.Get("rings").Items()){TArray<FVector2D> Ring;for(const auto& Q:R.Items())Ring.Add(Pt(Q));Poly.Rings.Add(MoveTemp(Ring));}
   for(const auto& Q:P.Get("v").Items())Poly.Vertices.Add(Pt(Q));
   for(const auto& I:P.Get("i").Items())Poly.Indices.Add(static_cast<SlateIndex>(I.NumberOr(0)));
   A.Polygons.Add(MoveTemp(Poly));
  }Countries.Add(MoveTemp(A));
 }
 for(const auto& C:Data.Get("cities").Items()){
  FAtlasCity A;A.Name=Str(C,"name");A.Country=Str(C,"country");A.Hub=Str(C,"hub");A.Position=Pt(C.Get("p"));A.Capital=C.Get("capital").Truthy();A.Rank=int(C.Get("rank").NumberOr(0));Cities.Add(MoveTemp(A));
 }
 Cities.StableSort([](const FAtlasCity& A,const FAtlasCity& B){if(A.Hub.IsEmpty()!=B.Hub.IsEmpty())return !A.Hub.IsEmpty();if(A.Capital!=B.Capital)return A.Capital;return A.Rank<B.Rank;});
 for(const auto& R:Data.Get("grid").Items()){TArray<FVector2D> Line;for(const auto& P:R.Items())Line.Add(Pt(P));Grid.Add(MoveTemp(Line));}
 Political=FImageUtils::ImportFileAsTexture2D(Root/TEXT("political.png"));Terrain=FImageUtils::ImportFileAsTexture2D(Root/TEXT("terrain.png"));
 if(!Political||!Terrain){Error=TEXT("Atlas imagery could not be loaded.");return false;}
 PoliticalBrush.SetResourceObject(Political);PoliticalBrush.ImageSize=FVector2D(Political->GetSizeX(),Political->GetSizeY());PoliticalBrush.DrawAs=ESlateBrushDrawType::Image;
 TerrainBrush.SetResourceObject(Terrain);TerrainBrush.ImageSize=PoliticalBrush.ImageSize;TerrainBrush.DrawAs=ESlateBrushDrawType::Image;
 UE_LOG(LogTemp,Display,TEXT("World atlas loaded: %d geographic units, %d named cities, Robinson projection."),Countries.Num(),Cities.Num());return true;
}
const FAtlasCountry* SWorldAtlas::Country(const FString& Id) const{for(const auto& C:Countries)if(C.Id==Id)return &C;return nullptr;}
const FAtlasCountry* SWorldAtlas::HitCountry(FVector2D P) const{
 for(const auto& C:Countries)for(const auto& Poly:C.Polygons){
  if(!Poly.Bounds.IsInsideOrOn(P)||Poly.Rings.IsEmpty()||!Contains(Poly.Rings[0],P))continue;
  bool Hole=false;for(int I=1;I<Poly.Rings.Num();++I)if(Contains(Poly.Rings[I],P)){Hole=true;break;}
  if(!Hole)return &C;
 }return nullptr;
}
FVector2D SWorldAtlas::Extent(FVector2D S) const{double W=FMath::Min(S.X,S.Y*Aspect);return FVector2D(W,W/Aspect)*Zoom;}
FVector2D SWorldAtlas::Screen(FVector2D P,FVector2D S) const{return S*.5+(P-Center)*Extent(S);}
FVector2D SWorldAtlas::Map(FVector2D P,FVector2D S) const{return Center+(P-S*.5)/Extent(S);}
void SWorldAtlas::ClampCenter(){double H=.5/Zoom;Center.X=FMath::Clamp(Center.X,H,1-H);Center.Y=FMath::Clamp(Center.Y,H,1-H);}
void SWorldAtlas::Reset(){Zoom=1;Center=FVector2D(.5,.5);}
void SWorldAtlas::ZoomBy(double Factor){Zoom=FMath::Clamp(Zoom*Factor,1.,12.);ClampCenter();}
void SWorldAtlas::ZoomAt(double Factor,FVector2D P,FVector2D S){auto Before=Map(P,S);ZoomBy(Factor);Center+=Before-Map(P,S);ClampCenter();}
void SWorldAtlas::FocusCountry(const FString& Id){if(const auto* C=Country(Id)){auto D=C->Focus.GetSize();Zoom=FMath::Clamp(.65/FMath::Max(D.X,D.Y),1.5,10.);Center=C->Focus.GetCenter();ClampCenter();}}
bool SWorldAtlas::Search(const FString& Query){
 FString Q=Query.TrimStartAndEnd();if(Q.IsEmpty())return false;
 const FAtlasCountry* Found=nullptr;
 for(const auto& C:Countries)if(C.Id.Equals(Q,ESearchCase::IgnoreCase)||C.Name.Equals(Q,ESearchCase::IgnoreCase)||C.Short.Equals(Q,ESearchCase::IgnoreCase)){Found=&C;break;}
 if(!Found)for(const auto& C:Cities)if(C.Name.Equals(Q,ESearchCase::IgnoreCase)){Center=C.Position;Zoom=7;ClampCenter();Selected=C.Country;OnSelect.ExecuteIfBound(Selected);return true;}
 if(!Found)for(const auto& C:Countries)if(C.Name.Contains(Q)||C.Short.Contains(Q)){Found=&C;break;}
 if(Found){Selected=Found->Id;FocusCountry(Selected);OnSelect.ExecuteIfBound(Selected);return true;}
 for(const auto& C:Cities)if(C.Name.Contains(Q)){Center=C.Position;Zoom=7;ClampCenter();Selected=C.Country;OnSelect.ExecuteIfBound(Selected);return true;}return false;
}
FReply SWorldAtlas::OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){
 if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
 Press=Last=G.AbsoluteToLocal(E.GetScreenSpacePosition());Dragging=true;Moved=false;return FReply::Handled().CaptureMouse(SharedThis(this));
}
FReply SWorldAtlas::OnMouseButtonUp(const FGeometry& G,const FPointerEvent& E){
 if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
 if(Dragging&&!Moved){auto P=Map(G.AbsoluteToLocal(E.GetScreenSpacePosition()),G.GetLocalSize());if(const auto* C=HitCountry(P)){Selected=C->Id;UE_LOG(LogTemp,Display,TEXT("Atlas selected territory: %s"),*C->Name);OnSelect.ExecuteIfBound(Selected);}}
 Dragging=false;return FReply::Handled().ReleaseMouseCapture();
}
void SWorldAtlas::OnMouseCaptureLost(const FCaptureLostEvent&){Dragging=false;}
FReply SWorldAtlas::OnMouseButtonDoubleClick(const FGeometry& G,const FPointerEvent& E){
 if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
 if(const auto* C=HitCountry(Map(G.AbsoluteToLocal(E.GetScreenSpacePosition()),G.GetLocalSize()))){Selected=C->Id;FocusCountry(Selected);OnSelect.ExecuteIfBound(Selected);}return FReply::Handled();
}
FReply SWorldAtlas::OnMouseMove(const FGeometry& G,const FPointerEvent& E){
 auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
 if(Dragging&&HasMouseCapture()){if(FVector2D::Distance(P,Press)>4)Moved=true;if(Moved){Center-=(P-Last)/Extent(G.GetLocalSize());ClampCenter();}Last=P;return FReply::Handled();}
 const auto* C=HitCountry(Map(P,G.GetLocalSize()));Hovered=C?C->Id:TEXT("");HoverName=C?C->Name:TEXT("Ocean");
 SetToolTipText(FText::FromString(C?C->Name+TEXT("  |  Click to inspect; double-click to focus"):TEXT("Drag to pan. Scroll to zoom.")));return FReply::Handled();
}
FReply SWorldAtlas::OnMouseWheel(const FGeometry& G,const FPointerEvent& E){ZoomAt(FMath::Pow(1.22,E.GetWheelDelta()),G.AbsoluteToLocal(E.GetScreenSpacePosition()),G.GetLocalSize());return FReply::Handled();}

int32 SWorldAtlas::OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const{
 const auto S=G.GetLocalSize();const auto* White=FCoreStyle::Get().GetBrush("WhiteBrush");
 FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(),White,ESlateDrawEffect::None,FLinearColor(.010f,.028f,.043f));
 const auto E=Extent(S);const auto Origin=Screen(FVector2D::ZeroVector,S);
 FSlateDrawElement::MakeBox(Out,Layer+1,G.ToPaintGeometry(E,FSlateLayoutTransform(Origin)),Mode==1?&TerrainBrush:&PoliticalBrush,ESlateDrawEffect::None,FLinearColor::White);
 auto Lines=[&](const TArray<FVector2D>& P,FLinearColor Color,float Width,int L){FSlateDrawElement::MakeLines(Out,Layer+L,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color,true,Width);};
 auto Visible=[&](const FBox2D& B){auto A=Screen(B.Min,S),Z=Screen(B.Max,S);return Z.X>=0&&Z.Y>=0&&A.X<=S.X&&A.Y<=S.Y;};
 auto Fill=[&](const FAtlasPolygon& P,FLinearColor Color){
  TArray<FSlateVertex> Vertices;Vertices.Reserve(P.Vertices.Num());const auto C=Color.ToFColor(true);
  for(auto Vtx:P.Vertices)Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),FVector2f(Screen(Vtx,S)),FVector2f(.5f,.5f),C));
  FSlateDrawElement::MakeCustomVerts(Out,Layer+2,FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White),Vertices,P.Indices,nullptr,0,0);
 };
 for(const auto& C:Countries){
  const bool Active=C.Id==Selected,Hover=C.Id==Hovered;
  FLinearColor Tint=FLinearColor::Transparent;
  if(Mode==2&&!C.Region.IsEmpty()){
   const auto& R=World.Get("regions").Get(TCHAR_TO_UTF8(*C.Region));
   Tint=R.Get("influence").NumberOr(0)>=R.Get("rival").NumberOr(0)?Teal:Coral;Tint.A=.46f;
  }
  if(Active){Tint=Gold;Tint.A=.30f;}else if(Hover){Tint=FLinearColor::White;Tint.A=.2f;}
  for(const auto& P:C.Polygons){if(!Visible(P.Bounds))continue;
   if(Tint.A>0)Fill(P,Tint);
   if(Zoom>1.65||Active||Hover){for(const auto& R:P.Rings){TArray<FVector2D> Points;Points.Reserve(R.Num()+1);for(auto Pnt:R)Points.Add(Screen(Pnt,S));if(!Points.IsEmpty()){const FVector2D First=Points[0];Points.Add(First);}Lines(Points,Active?Gold:(Hover?Ink:FLinearColor(.12f,.20f,.22f,.72f)),Active?2.2f:(Hover?1.5f:.8f),Active?4:3);}}
  }
 }
 if(ShowGrid)for(const auto& R:Grid){TArray<FVector2D> Points;for(auto P:R)Points.Add(Screen(P,S));Lines(Points,FLinearColor(.53f,.72f,.77f,.18f),.7f,3);}
 // Routes connect actual capital cities; no marker ever represents a country.
 if(ShowRoutes){const FAtlasCity* Home=nullptr;for(const auto& C:Cities)if(C.Hub==TEXT("usa"))Home=&C;
  if(Home)for(const auto& C:Cities){if(C.Hub.IsEmpty()||!World.Get("regions").Get(TCHAR_TO_UTF8(*C.Hub)).Get("trade").Truthy())continue;
   auto A=Screen(Home->Position,S),B=Screen(C.Position,S);TArray<FVector2D> Route;
   for(int I=0;I<=48;++I){double T=I/48.;auto P=A*(1-T)+B*T;P.Y-=FMath::Sin(T*PI)*FMath::Min(90.,FVector2D::Distance(A,B)*.13);Route.Add(P);}Lines(Route,FLinearColor(.1f,.8f,.7f,.75f),1.7f,5);
  }
 }
 TArray<FSlateRect> Occupied;const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
 auto Label=[&](FVector2D P,const FString& Name,int Font,FLinearColor Color,bool Bold,bool Force,bool Centered){
  auto Style=FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Regular",Font);auto Size=Measure->Measure(Name,Style);if(Centered)P-=Size*.5;
  FSlateRect R(P.X-3,P.Y-2,P.X+Size.X+3,P.Y+Size.Y+2);
  if(R.Left<4||R.Top<4||R.Right>S.X-4||R.Bottom>S.Y-4)return false;
  if(!Force)for(const auto& O:Occupied)if(Overlap(R,O))return false;Occupied.Add(R);
  for(const auto Offset:{FVector2D(-1,0),FVector2D(1,0),FVector2D(0,-1),FVector2D(0,1)})FSlateDrawElement::MakeText(Out,Layer+7,G.ToPaintGeometry(Size,FSlateLayoutTransform(P+Offset)),Name,Style,ESlateDrawEffect::None,Dark);
  FSlateDrawElement::MakeText(Out,Layer+8,G.ToPaintGeometry(Size,FSlateLayoutTransform(P)),Name,Style,ESlateDrawEffect::None,Color);return true;
 };
 // City labels get priority so their dots always have an unambiguous meaning.
 if(ShowCities)for(const auto& C:Cities){
  bool Important=!C.Hub.IsEmpty();if(!Important&&Zoom<2.2)continue;if(!C.Capital&&!Important&&Zoom<4.0)continue;if(!Important&&C.Rank>4&&Zoom<4.5)continue;
  auto P=Screen(C.Position,S);if(P.X<9||P.Y<9||P.X>S.X-9||P.Y>S.Y-9)continue;
  if(!Label(P+FVector2D(7,-7),C.Name,Important?12:11,Important?Gold:Ink,Important,false,false))continue;
  TArray<FVector2D> Ring;for(int I=0;I<=16;++I){double A=I*2*PI/16;Ring.Add(P+FVector2D(FMath::Cos(A),FMath::Sin(A))*3.2);}Lines(Ring,Dark,3.5f,6);Lines(Ring,C.Capital?Gold:Ink,1.3f,6);
  if(C.Capital)FSlateDrawElement::MakeBox(Out,Layer+6,G.ToPaintGeometry(FVector2D(2,2),FSlateLayoutTransform(P-FVector2D(1,1))),White,ESlateDrawEffect::None,Gold);
 }
 // Geographic area labels contain no point symbol.
 for(const auto& C:Countries){
  const bool Active=C.Id==Selected;double A=C.Area*E.X*E.Y;
  if(!Active&&A<1400)continue;
  const int Font=FMath::Clamp(int(10+FMath::Loge(FMath::Max(1.,A/1200))*1.5),10,19);
  const FString Name=C.Id==TEXT("USA")?TEXT("UNITED STATES"):C.Short.ToUpper();
  const auto Anchor=Screen(C.Label,S);
  // Keep area names on their own territory, trying open space around city labels.
  // Checking this country's polygons avoids a whole-atlas hit test per candidate.
  for(const auto Offset:{FVector2D(0,0),FVector2D(0,-24),FVector2D(0,24),FVector2D(-30,0),FVector2D(30,0),FVector2D(-24,-24),FVector2D(24,-24),FVector2D(0,-45),FVector2D(0,45),FVector2D(-24,24),FVector2D(24,24)}){
   const auto Candidate=Anchor+Offset;const auto Geo=Map(Candidate,S);bool Interior=false;
   for(const auto& P:C.Polygons){if(!P.Bounds.IsInsideOrOn(Geo)||P.Rings.IsEmpty()||!Contains(P.Rings[0],Geo))continue;Interior=true;for(int I=1;I<P.Rings.Num();++I)if(Contains(P.Rings[I],Geo)){Interior=false;break;}if(Interior)break;}
   if(Interior&&Label(Candidate,Name,Font,Active?Gold:Ink,true,false,true))break;
  }
 }
 if(Zoom<1.45){Label(Screen(FVector2D(.19,.50),S),TEXT("PACIFIC OCEAN"),12,FLinearColor(.45f,.68f,.76f),false,false,true);Label(Screen(FVector2D(.43,.50),S),TEXT("ATLANTIC OCEAN"),12,FLinearColor(.45f,.68f,.76f),false,false,true);Label(Screen(FVector2D(.69,.64),S),TEXT("INDIAN OCEAN"),12,FLinearColor(.45f,.68f,.76f),false,false,true);}
 if(!Error.IsEmpty())Label(S*.5,Error,20,Coral,true,true,true);
 return Layer+8;
}
