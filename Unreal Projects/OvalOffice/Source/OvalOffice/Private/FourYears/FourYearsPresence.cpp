#include "FourYears/FourYearsPresence.h"
#include "FourYears/FourYearsRoom.h"
#include "FourYears/FourYearsWalker.h"
#include "FourYears/FourYearsSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace {
const FLinearColor Cream(.74f,.66f,.49f),Wood(.12f,.045f,.022f),Gold(.55f,.34f,.09f),Navy(.018f,.036f,.07f),Leather(.10f,.045f,.02f),White(.85f,.82f,.73f);
UStaticMeshComponent* Shape(AActor* Owner,USceneComponent* Parent,const TCHAR* Mesh,FVector P,FVector Size,FLinearColor Color,FRotator R=FRotator::ZeroRotator,float Rough=.65f) {
 auto* C=NewObject<UStaticMeshComponent>(Owner);Owner->AddInstanceComponent(C);C->SetupAttachment(Parent);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Mesh,Mesh)));
 C->SetRelativeLocation(P);C->SetRelativeRotation(R);C->SetRelativeScale3D(Size/100.f);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetMobility(EComponentMobility::Movable);
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/CabinetRoom/Materials/M_CabinetSurface.M_CabinetSurface"));
 if(Base){auto* M=UMaterialInstanceDynamic::Create(Base,C);M->SetVectorParameterValue(TEXT("Tint"),Color);M->SetScalarParameterValue(TEXT("Roughness"),Rough);C->SetMaterial(0,M);}C->RegisterComponent();return C;
}
void Box(AActor* O,FVector P,FVector S,FLinearColor C,FRotator R=FRotator::ZeroRotator){Shape(O,O->GetRootComponent(),TEXT("Cube"),P,S,C,R);}
void Light(AActor* O,FVector P,FLinearColor C,float Power){auto* L=NewObject<UPointLightComponent>(O);O->AddInstanceComponent(L);L->SetupAttachment(O->GetRootComponent());L->SetRelativeLocation(P);L->SetMobility(EComponentMobility::Movable);L->SetIntensity(Power);L->SetLightColor(C);L->SetAttenuationRadius(1100);L->SetCastShadows(false);L->RegisterComponent();}
void Interaction(AFourYearsRoom* R,FVector2D P,const FString& Prompt,const FString& Target){FFourYearsRoomInteraction I;I.Position=P;I.Radius=110;I.Prompt=Prompt;I.Action=TEXT("presidency");I.Target=Target;R->Interactions.Add(I);}
void Lamp(AActor* O,FVector P){Box(O,P+FVector(0,0,35),FVector(5,5,70),Gold);Shape(O,O->GetRootComponent(),TEXT("Cylinder"),P+FVector(0,0,77),FVector(32,32,29),White);Light(O,P+FVector(0,0,73),FLinearColor(1,.73f,.4f),900);}
void Art(AActor* O,FVector P,int Theme){Box(O,P,FVector(140,6,100),Gold);Box(O,P+FVector(0,-4,0),FVector(128,2,88),FLinearColor(.16f+.04f*Theme,.30f,.38f));Box(O,P+FVector(0,-6,-20),FVector(125,2,35),FLinearColor(.10f,.19f+.025f*Theme,.13f));Shape(O,O->GetRootComponent(),TEXT("Sphere"),P+FVector(28,-8,18),FVector(23,2,23),FLinearColor(.90f,.63f,.26f));for(int I=0;I<4;++I)Box(O,P+FVector(-45+I*24,-8,-18+I*5),FVector(18,2,27+I*6),FLinearColor(.08f,.13f,.16f));}
}
AFourYearsStaff::AFourYearsStaff(){PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));Body=CreateDefaultSubobject<USceneComponent>(TEXT("Body"));Body->SetupAttachment(RootComponent);Head=CreateDefaultSubobject<USceneComponent>(TEXT("Head"));Head->SetupAttachment(Body);}
void AFourYearsStaff::Build(const FString& Name,const FString& Action,int Style,bool Seated){
 PersonName=Name;Interaction=Action;BaseYaw=GetActorRotation().Yaw;Time=Style;
 // Authored, UV-mapped Blender models replace the original primitive people.
 // Character identity is stable across maps, independent of their seat or dialogue action.
 struct FStaffModel {const TCHAR* Id;FVector StandingHead;FVector SeatedHead;};
 static const FStaffModel Models[]={
#include "PresidentialStaffCatalog.inl"
 };
 const FStaffModel* Model=&Models[0];
 for(const auto& Candidate:Models){if(Name.Contains(Candidate.Id)){Model=&Candidate;break;}}
 const FString BodyPath=FString::Printf(TEXT("/Game/PresidentialStaff/Meshes/SM_%s_%s.SM_%s_%s"),Model->Id,Seated?TEXT("Seated"):TEXT("Standing"),Model->Id,Seated?TEXT("Seated"):TEXT("Standing"));
 const FString HeadPath=FString::Printf(TEXT("/Game/PresidentialStaff/Meshes/SM_%s_Head.SM_%s_Head"),Model->Id,Model->Id);
 auto* BodyMesh=LoadObject<UStaticMesh>(nullptr,*BodyPath);auto* HeadMesh=LoadObject<UStaticMesh>(nullptr,*HeadPath);
 if(BodyMesh&&HeadMesh){
  auto Attach=[this](UStaticMesh* Mesh,USceneComponent* Parent){auto* C=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(Parent);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetMobility(EComponentMobility::Movable);C->SetCastShadow(true);C->RegisterComponent();};
  Body->SetRelativeLocation(FVector::ZeroVector);Head->SetRelativeLocation(Seated?Model->SeatedHead:Model->StandingHead);
  Attach(BodyMesh,Body);Attach(HeadMesh,Head);bDetailedModel=true;return;
 }
 UE_LOG(LogTemp,Warning,TEXT("Presidential staff assets missing for %s; using emergency placeholder"),*Name);
 const FLinearColor Skins[]={FLinearColor(.56f,.34f,.21f),FLinearColor(.83f,.61f,.43f),FLinearColor(.27f,.13f,.075f),FLinearColor(.66f,.44f,.28f)};
 const FLinearColor Skin=Skins[Style%4],Suit=Style%3==0?Navy:Style%3==1?FLinearColor(.13f,.14f,.16f):FLinearColor(.18f,.10f,.07f),Hair=Style%4==1?FLinearColor(.22f,.14f,.07f):FLinearColor(.025f,.018f,.015f);
 const float Hip=Seated?53:89;Body->SetRelativeLocation(FVector(0,0,Hip));
 Shape(this,Body,TEXT("Sphere"),FVector(0,0,22),FVector(38,49,61),Suit);
 Shape(this,Body,TEXT("Cube"),FVector(17,0,30),FVector(4,15,42),White);
 Shape(this,Body,TEXT("Cube"),FVector(20,0,29),FVector(2,5,35),Style%2?FLinearColor(.40f,.04f,.045f):FLinearColor(.06f,.19f,.32f));
 for(int Side:{-1,1}){
  Shape(this,Body,TEXT("Sphere"),FVector(0,Side*25,17),FVector(20,18,57),Suit,FRotator(0,0,Side*8));
  Shape(this,Body,TEXT("Sphere"),FVector(2,Side*28,-11),FVector(12,10,17),Skin);
  if(Seated){Shape(this,Body,TEXT("Cube"),FVector(21,Side*12,-6),FVector(52,17,18),Suit);Shape(this,Body,TEXT("Cube"),FVector(43,Side*12,-28),FVector(17,17,43),Suit);Shape(this,Body,TEXT("Cube"),FVector(49,Side*12,-48),FVector(28,16,9),Hair);}
  else{Shape(this,Body,TEXT("Cube"),FVector(0,Side*12,-43),FVector(19,18,82),Suit);Shape(this,Body,TEXT("Cube"),FVector(8,Side*12,-84),FVector(31,18,10),Hair);}
 }
 Head->SetRelativeLocation(FVector(0,0,70));
 Shape(this,Head,TEXT("Sphere"),FVector(0,0,0),FVector(26,25,33),Skin);
 Shape(this,Head,TEXT("Sphere"),FVector(-3,0,9),FVector(28,27,20),Hair);
 for(int Side:{-1,1}){Shape(this,Head,TEXT("Sphere"),FVector(12,Side*6,2),FVector(4,5,3),White);Shape(this,Head,TEXT("Sphere"),FVector(14,Side*6,2),FVector(1.5f,2,2.5f),FLinearColor(.025f,.02f,.015f));Shape(this,Head,TEXT("Sphere"),FVector(0,Side*13,0),FVector(7,4,10),Skin);}
 Shape(this,Head,TEXT("Sphere"),FVector(14,0,-2),FVector(7,5,7),Skin);
 Shape(this,Head,TEXT("Cube"),FVector(12.5f,0,-10),FVector(1.5f,10,1.6f),FLinearColor(.25f,.09f,.07f));
 if(Style==0||Style==3){Shape(this,Head,TEXT("Sphere"),FVector(-8,0,-4),FVector(18,29,31),Hair);}
 Shape(this,Body,TEXT("Cube"),FVector(19,-14,43),FVector(2,5,3),Gold);
}
void AFourYearsStaff::Tick(float Dt){
 Super::Tick(Dt);Time+=Dt;
 // Restrained breathing and attention shifts; no bobble-head or full-body rocking.
 Body->SetRelativeRotation(FRotator(0,0,FMath::Sin(Time*.85f)*(bDetailedModel?.10f:.6f)));
 auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0);if(!Pawn)return;
 const FVector Delta=Pawn->GetActorLocation()-GetActorLocation();
 const bool Nearby=Delta.Size2D()<320;const float Limit=bDetailedModel?27.f:45.f;
 const float Want=Nearby?FMath::Clamp(FRotator::NormalizeAxis(Delta.Rotation().Yaw-BaseYaw),-Limit,Limit):FMath::Sin(Time*.28f)*3.f;
 const FVector EyeDelta=Pawn->GetActorLocation()-(Head->GetComponentLocation()+FVector(0,0,14));
 const float Pitch=Nearby?FMath::Clamp(EyeDelta.Rotation().Pitch,-5.f,5.f):FMath::Sin(Time*.43f)*.7f;
 Head->SetRelativeRotation(FMath::RInterpTo(Head->GetRelativeRotation(),FRotator(Pitch,Want,0),Dt,1.8f));
}
AFourYearsPresence::AFourYearsPresence(){RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));}
AFourYearsStaff* AFourYearsPresence::Person(const FString& Name,const FString& Action,const FString& Place,FVector P,float Yaw,int Style,bool Seated){auto* A=GetWorld()->SpawnActor<AFourYearsStaff>(P,FRotator(0,Yaw,0));A->Place=Place;A->Build(Name,Action,Style,Seated);Staff.Add(A);return A;}
void AFourYearsPresence::Build(){
 const FString Map=GetWorld()->GetMapName();CurrentPlace=Map.Contains(TEXT("CabinetRoom"))?TEXT("cabinet"):Map.Contains(TEXT("AirForceOne"))?TEXT("aircraft"):TEXT("oval");
 for(TActorIterator<AFourYearsRoom> It(GetWorld());It;++It){MainRoom=*It;break;}
 if(!MainRoom)MainRoom=GetWorld()->SpawnActor<AFourYearsRoom>();MainRoom->RoomId=CurrentPlace;Rooms.Add(MainRoom);
 if(CurrentPlace==TEXT("oval")){
  Person(TEXT("Maya Chen"),TEXT("adviser:chen"),CurrentPlace,FVector(115,-235,0),115,0);
  Person(TEXT("Daniel Brooks"),TEXT("adviser:brooks"),CurrentPlace,FVector(115,215,0),-120,1);
  Person(TEXT("Agent Morgan"),TEXT("travel"),CurrentPlace,FVector(100,345,0),-110,2);
 }else if(CurrentPlace==TEXT("cabinet")){
  Person(TEXT("Maya Chen"),TEXT("adviser:chen"),CurrentPlace,FVector(-200,180,0),-90,0,true);
  Person(TEXT("Elena Ruiz"),TEXT("adviser:ruiz"),CurrentPlace,FVector(200,180,0),-90,3,true);
  Person(TEXT("Daniel Brooks"),TEXT("adviser:brooks"),CurrentPlace,FVector(-200,-180,0),90,1,true);
  Person(TEXT("Samira Bell"),TEXT("adviser:bell"),CurrentPlace,FVector(200,-180,0),90,2,true);
  Person(TEXT("Speaker Evelyn Mercer"),TEXT("relations"),CurrentPlace,FVector(-500,-190,0),40,1);
  Person(TEXT("Senator Luis Ortega"),TEXT("relations"),CurrentPlace,FVector(480,230,0),-140,0);
 }else{
  MainRoom->RoomLabel=TEXT("AIR FORCE ONE");MainRoom->WalkableAreas={FBox2D(FVector2D(25,-305),FVector2D(1290,323))};MainRoom->bUseWorldCollision=true;MainRoom->ArrivalPosition=FVector(365,95,165);MainRoom->ArrivalYaw=-130;
  Person(TEXT("Elena Ruiz"),TEXT("world"),CurrentPlace,FVector(1060,-250,0),90,3);
  Interaction(MainRoom,FVector2D(360,275),TEXT("E: presidential travel desk"),TEXT("travel"));
 }
 Residence();Limo();
 if(auto* W=Cast<AFourYearsWalker>(UGameplayStatics::GetPlayerPawn(this,0)))W->UseRoom(MainRoom,false);
 if(auto* Game=GetWorld()->GetGameInstance()->GetSubsystem<UFourYearsSubsystem>()){Game->GetState()["presidency"].Set("place",FourYears::JsonValue::String(TCHAR_TO_UTF8(*CurrentPlace)));Game->SetLocation(CurrentPlace==TEXT("aircraft")?TEXT("aircraft"):TEXT("oval"));}
}
void AFourYearsPresence::Residence(){
 const FVector O(8500,0,0);ResidenceRoom=GetWorld()->SpawnActor<AFourYearsRoom>();auto* R=ResidenceRoom.Get();R->RoomId=TEXT("residence");R->RoomLabel=TEXT("EXECUTIVE RESIDENCE / PRIVATE SITTING ROOM");R->WalkableAreas={FBox2D(FVector2D(8180,-265),FVector2D(8820,265))};R->ArrivalPosition=O+FVector(-265,0,165);R->ArrivalYaw=0;Rooms.Add(R);
 Box(this,O+FVector(0,0,-8),FVector(700,600,16),Wood);Box(this,O+FVector(0,0,0),FVector(515,425,2),FLinearColor(.24f,.28f,.24f));
 Box(this,O+FVector(0,0,310),FVector(700,600,18),Cream);
 for(int Side:{-1,1}){Box(this,O+FVector(0,Side*300,155),FVector(700,15,310),Cream);Box(this,O+FVector(Side*350,0,155),FVector(15,600,310),Cream);Box(this,O+FVector(0,Side*285,42),FVector(690,12,84),White);Box(this,O+FVector(0,Side*280,87),FVector(690,9,8),Gold);Box(this,O+FVector(0,Side*275,290),FVector(690,18,18),White);}
 // Sofa, upholstered arms and a substantial coffee table, all with matching walking footprints.
 Box(this,O+FVector(0,211,30),FVector(240,82,55),Wood);Box(this,O+FVector(0,212,58),FVector(222,72,22),FLinearColor(.49f,.43f,.32f));Box(this,O+FVector(0,247,94),FVector(240,18,76),FLinearColor(.43f,.39f,.29f));
 for(int S:{-1,1}){Box(this,O+FVector(S*120,211,68),FVector(20,85,65),FLinearColor(.43f,.39f,.29f));Box(this,O+FVector(S*78,217,78),FVector(48,45,15),FLinearColor(.26f,.33f,.35f));Lamp(this,O+FVector(S*192,210,35));}
 Box(this,O+FVector(0,66,47),FVector(155,72,10),Wood);for(int X:{-1,1})for(int Y:{-1,1})Box(this,O+FVector(X*60,66+Y*24,22),FVector(9,9,45),Wood);
 for(int I=0;I<3;++I)Box(this,O+FVector(-30+I*22,66,54+I*2),FVector(40,26,4),FLinearColor(.10f+.08f*I,.16f,.22f));
 Box(this,O+FVector(245,-168,77),FVector(124,70,9),Wood);for(int X:{-1,1})for(int Y:{-1,1})Box(this,O+FVector(245+X*49,-168+Y*25,36),FVector(8,8,72),Wood);Lamp(this,O+FVector(265,-173,82));
 Box(this,O+FVector(221,-160,83),FVector(35,28,3),White);
 // A wide evening window with mullions, curtain folds and garden silhouettes.
 Box(this,O+FVector(340,50,177),FVector(4,235,153),FLinearColor(.13f,.23f,.33f));for(int S:{-1,1}){Box(this,O+FVector(332,50+S*125,177),FVector(12,18,170),Gold);for(int I=0;I<4;++I)Shape(this,RootComponent,TEXT("Cylinder"),O+FVector(317+I%2*4,50+S*(145+I*7),156),FVector(16,16,260),FLinearColor(.18f,.25f,.27f));}Box(this,O+FVector(331,50,177),FVector(9,7,155),White);Box(this,O+FVector(331,50,177),FVector(9,235,6),White);
 for(int I=0;I<7;++I)Shape(this,RootComponent,TEXT("Sphere"),O+FVector(335,-45+I*30,116),FVector(3,45,30+I%3*12),FLinearColor(.05f,.11f,.07f));
 Art(this,O+FVector(-150,284,180),1);Art(this,O+FVector(135,284,180),3);
 Box(this,O+FVector(-342,0,111),FVector(10,108,220),Wood);Box(this,O+FVector(-334,-38,108),FVector(5,7,7),Gold);
 R->Obstacles={FBox2D(FVector2D(8350,151),FVector2D(8650,274)),FBox2D(FVector2D(8405,12),FVector2D(8595,120)),FBox2D(FVector2D(8670,-224),FVector2D(8820,-110))};
 Interaction(R,FVector2D(8245,0),TEXT("E: leave the residence / travel"),TEXT("travel"));Interaction(R,FVector2D(8720,-85),TEXT("E: private papers and tonight's chapter"),TEXT("story"));
 Person(TEXT("Samira Bell"),TEXT("adviser:bell"),TEXT("residence"),O+FVector(52,198,0),-90,2,true);Person(TEXT("Jordan Ellis / residence aide"),TEXT("rest"),TEXT("residence"),O+FVector(-190,-180,0),40,1);
 Light(this,O+FVector(0,0,265),FLinearColor(1,.82f,.59f),11000);Light(this,O+FVector(240,30,205),FLinearColor(.55f,.7f,1),5000);
}
void AFourYearsPresence::Limo(){
 const FVector O(5000,0,0);LimoRoom=GetWorld()->SpawnActor<AFourYearsRoom>();auto* R=LimoRoom.Get();R->RoomId=TEXT("limo");R->RoomLabel=TEXT("PRESIDENTIAL MOTORCADE / SECURE CABIN");R->WalkableAreas={FBox2D(FVector2D(4760,-48),FVector2D(5190,48))};R->ArrivalPosition=O+FVector(-170,0,145);R->ArrivalYaw=0;R->CameraHeight=145;Rooms.Add(R);
 Box(this,O+FVector(0,0,-7),FVector(560,275,14),Navy);Box(this,O+FVector(0,0,207),FVector(560,275,16),Navy);Box(this,O+FVector(-275,0,100),FVector(15,275,210),Navy);Box(this,O+FVector(245,0,100),FVector(18,275,210),Navy);
 for(int Side:{-1,1}){
  Box(this,O+FVector(0,Side*131,196),FVector(550,16,28),Navy);
  Box(this,O+FVector(0,Side*133,46),FVector(550,12,95),Navy);Box(this,O+FVector(0,Side*131,140),FVector(530,8,102),FLinearColor(.12f,.2f,.28f));Box(this,O+FVector(0,Side*123,92),FVector(530,9,12),Wood);
  for(int I=-1;I<=1;++I)Box(this,O+FVector(I*175,Side*119,143),FVector(10,16,120),Navy);
  Box(this,O+FVector(-5,Side*96,48),FVector(330,53,30),Leather);Box(this,O+FVector(-5,Side*120,83),FVector(340,15,83),Leather);
  for(int I=-2;I<=2;++I)Box(this,O+FVector(I*60,Side*95,66),FVector(55,50,10),FLinearColor(.18f,.09f,.045f));
  Box(this,O+FVector(-200,Side*113,105),FVector(38,6,5),Gold);
  // Abstract avenue beyond the protective glass: recognizable architectural silhouettes.
  for(int I=0;I<8;++I){Box(this,O+FVector(-235+I*67,Side*125,110+I%3*15),FVector(48,3,100+I%3*25),FLinearColor(.16f+I%2*.07f,.20f,.22f));for(int K=0;K<3;++K)Box(this,O+FVector(-228+I*67,Side*122,110+K*20),FVector(7,2,8),FLinearColor(.6f,.46f,.19f));}
 }
 Box(this,O+FVector(227,0,133),FVector(3,133,58),FLinearColor(.035f,.065f,.09f));Box(this,O+FVector(217,0,83),FVector(25,190,28),Wood);Box(this,O+FVector(201,-44,101),FVector(14,35,18),Navy);Box(this,O+FVector(0,0,198),FVector(360,16,3),White);
 Person(TEXT("Maya Chen"),TEXT("adviser:chen"),TEXT("limo"),O+FVector(60,88,0),-110,0,true);
 Interaction(R,FVector2D(4810,0),TEXT("E: motorcade destinations"),TEXT("travel"));Interaction(R,FVector2D(5160,0),TEXT("E: secure phone / Congress"),TEXT("relations"));
 Light(this,O+FVector(-120,0,173),FLinearColor(1,.8f,.57f),3400);Light(this,O+FVector(145,0,165),FLinearColor(.67f,.80f,1),2800);
}
AFourYearsStaff* AFourYearsPresence::Nearby(const FVector& P)const{AFourYearsStaff* Best=nullptr;float Distance=145;for(const auto& A:Staff)if(A&&A->Place==CurrentPlace){const float D=FVector::Dist2D(P,A->GetActorLocation());if(D<Distance){Distance=D;Best=A.Get();}}return Best;}
bool AFourYearsPresence::Enter(const FString& Place){
 AFourYearsRoom* R=Place==TEXT("residence")?ResidenceRoom.Get():Place==TEXT("limo")?LimoRoom.Get():Place==MainRoom->RoomId?MainRoom.Get():nullptr;
 if(!R)return false;auto* W=Cast<AFourYearsWalker>(UGameplayStatics::GetPlayerPawn(this,0));if(!W)return false;
 CurrentPlace=Place;W->UseRoom(R,true);if(auto* Game=GetWorld()->GetGameInstance()->GetSubsystem<UFourYearsSubsystem>()){Game->GetState()["presidency"].Set("place",FourYears::JsonValue::String(TCHAR_TO_UTF8(*Place)));Game->SetLocation(Place==TEXT("aircraft")?TEXT("aircraft"):TEXT("oval"));}return true;
}
