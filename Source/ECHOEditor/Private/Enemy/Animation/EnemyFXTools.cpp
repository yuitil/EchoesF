#include "Enemy/Animation/EnemyFXTools.h"
#include "Enemy/Combat/EnemyBomb.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_Self.h"
//
int32 UEnemyFXTools::SetSpriteMaterial(UNiagaraSystem* system, UMaterialInterface* material)
{
    if (!system || !material) return 0;
    int32 count=0;
    for (const auto& handle : system->GetEmitterHandles())
        if (auto* data=handle.GetInstance().GetEmitterData())
            for (auto* renderer : data->GetRenderers())
                if (auto* sprite=Cast<UNiagaraSpriteRendererProperties>(renderer); sprite && sprite->GetOutermost()==system->GetOutermost())
                { sprite->Modify(); sprite->Material=material; sprite->SubImageSize=FVector2D(1.f,1.f); ++count; }
    system->MarkPackageDirty();
    return count;
}
bool UEnemyFXTools::SetupBombFX(UBlueprint* blueprint, UNiagaraSystem* trail, FVector scale)
{
    if (!blueprint || !trail || !blueprint->GeneratedClass->IsChildOf(AEnemyBomb::StaticClass())) return false;
    blueprint->Modify();
    auto* scs=blueprint->SimpleConstructionScript.Get();
    USCS_Node* trailNode=nullptr;
    for (auto* node:scs->GetAllNodes()) if (node->GetVariableName()==TEXT("CurseTrail")) trailNode=node;
    if (!trailNode)
    {
        trailNode=scs->CreateNode(UNiagaraComponent::StaticClass(),TEXT("CurseTrail"));
        scs->AddNode(trailNode);
        trailNode->SetParent(Cast<AActor>(blueprint->GeneratedClass->GetDefaultObject())->GetRootComponent());
    }
    auto* component=Cast<UNiagaraComponent>(trailNode->ComponentTemplate);
    component->SetAsset(trail); component->SetRelativeScale3D(scale); component->SetAutoActivate(true);
    UEdGraph* graph=blueprint->UbergraphPages.IsEmpty() ? nullptr : blueprint->UbergraphPages[0];
    if (!graph)
    {
        graph=FBlueprintEditorUtils::CreateNewGraph(blueprint,TEXT("EventGraph"),UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddUbergraphPage(blueprint,graph);
    }
    for(const auto& node:graph->Nodes)
        if(auto* event=Cast<UK2Node_Event>(node);event && event->EventReference.GetMemberName()==TEXT("BurstAt"))
            return false;
    auto* event=NewObject<UK2Node_Event>(graph);
    event->EventReference.SetExternalMember(TEXT("BurstAt"),AEnemyBomb::StaticClass());
    event->bOverrideFunction=true;graph->AddNode(event);event->CreateNewGuid();event->AllocateDefaultPins();event->NodePosY=400;
    auto* spawn=NewObject<UK2Node_CallFunction>(graph);
    spawn->SetFromFunction(UNiagaraFunctionLibrary::StaticClass()->FindFunctionByName(TEXT("SpawnSystemAtLocation")));
    graph->AddNode(spawn);spawn->CreateNewGuid();spawn->AllocateDefaultPins();spawn->NodePosX=500;spawn->NodePosY=400;
    auto* self=NewObject<UK2Node_Self>(graph);graph->AddNode(self);self->CreateNewGuid();self->AllocateDefaultPins();self->NodePosX=200;self->NodePosY=700;
    const auto* schema=GetDefault<UEdGraphSchema_K2>();
    bool ok=schema->TryCreateConnection(event->FindPinChecked(UEdGraphSchema_K2::PN_Then),spawn->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
    ok&=schema->TryCreateConnection(event->FindPinChecked(TEXT("point")),spawn->FindPinChecked(TEXT("Location")));
    if(auto* context=spawn->FindPin(TEXT("WorldContextObject"))) ok&=schema->TryCreateConnection(self->FindPinChecked(UEdGraphSchema_K2::PN_Self),context);
    for (const auto& pair : TArray<TPair<FName,FName>>{{TEXT("m_explosion"),TEXT("SystemTemplate")},{TEXT("m_fxScale"),TEXT("Scale")}})
    {
        auto* get=NewObject<UK2Node_VariableGet>(graph);get->VariableReference.SetSelfMember(pair.Key);graph->AddNode(get);get->CreateNewGuid();get->AllocateDefaultPins();get->NodePosX=200;get->NodePosY=800+(pair.Key==TEXT("m_fxScale") ? 100 : 0);
        ok&=schema->TryCreateConnection(get->FindPinChecked(pair.Key),spawn->FindPinChecked(pair.Value));
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);
    FKismetEditorUtilities::CompileBlueprint(blueprint);
    blueprint->MarkPackageDirty();
    return ok && blueprint->Status!=BS_Error;
}
//


//
#include "Materials/Material.h"
#include "Materials/MaterialExpressionGetMaterialAttributes.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "MaterialEditingLibrary.h"
bool UEnemyFXTools::AddWarningGlow(UMaterial* material)
{
    if (!material || !material->bUseMaterialAttributes) return false;
    for (const auto& e:material->GetExpressionCollection().Expressions)
        if (auto* p=Cast<UMaterialExpressionScalarParameter>(e);p && p->ParameterName==TEXT("CurseWarn")) return true;
    const auto original=material->GetEditorOnlyData()->MaterialAttributes;
    if (!original.Expression) return false;
    auto create=[material](UClass* type,int x,int y){return UMaterialEditingLibrary::CreateMaterialExpression(material,type,x,y);};
    auto* get=Cast<UMaterialExpressionGetMaterialAttributes>(create(UMaterialExpressionGetMaterialAttributes::StaticClass(),-600,500));
    get->MaterialAttributes=original;
    const int32 output=get->CreateOrGetOutputAttribute(MP_EmissiveColor);
    auto* amount=Cast<UMaterialExpressionScalarParameter>(create(UMaterialExpressionScalarParameter::StaticClass(),-600,750));
    amount->ParameterName=TEXT("CurseWarn");amount->DefaultValue=0.f;
    auto* color=Cast<UMaterialExpressionVectorParameter>(create(UMaterialExpressionVectorParameter::StaticClass(),-600,900));
    color->ParameterName=TEXT("WarningColor");color->DefaultValue=FLinearColor(.8f,.05f,.3f,1.f);
    auto* multiply=Cast<UMaterialExpressionMultiply>(create(UMaterialExpressionMultiply::StaticClass(),-350,750));
    multiply->A.Connect(0,amount);multiply->B.Connect(0,color);
    auto* add=Cast<UMaterialExpressionAdd>(create(UMaterialExpressionAdd::StaticClass(),-150,500));
    add->A.Connect(output,get);add->B.Connect(0,multiply);
    auto* set=Cast<UMaterialExpressionSetMaterialAttributes>(create(UMaterialExpressionSetMaterialAttributes::StaticClass(),50,500));
    set->Inputs[0]=original;set->ConnectInputAttribute(MP_EmissiveColor,add);
    material->GetEditorOnlyData()->MaterialAttributes.Connect(0,set);
    UMaterialEditingLibrary::RecompileMaterial(material);
    material->MarkPackageDirty();return true;
}
//
//
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Stateless/Modules/NiagaraStatelessModule_SpriteFacingAndAlignment.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
bool UEnemyFXTools::ConfigureGroundRing(UNiagaraSystem* system, UMaterialInterface* material)
{
    if (!system || !material) return false;
    FNiagaraVariable size(FNiagaraTypeDefinition::GetVec2Def(),TEXT("User.WarningSize"));size.SetValue(FVector2f(360.f));system->GetExposedParameters().AddParameter(size);
    FNiagaraVariable normal(FNiagaraTypeDefinition::GetVec3Def(),TEXT("User.GroundNormal"));normal.SetValue(FVector3f::ZAxisVector);system->GetExposedParameters().AddParameter(normal);
    bool found=false;
    for (const auto& handle:system->GetEmitterHandles())
    {
        auto* emitter=handle.GetStatelessEmitter(); if (!emitter) continue;
        emitter->Modify();
        for (const auto& module:emitter->GetModules()) module->SetIsModuleEnabled(false);
        auto* init=Cast<UNiagaraStatelessModule_InitializeParticle>(emitter->GetModule(UNiagaraStatelessModule_InitializeParticle::StaticClass()));
        auto* facing=Cast<UNiagaraStatelessModule_SpriteFacingAndAlignment>(emitter->GetModule(UNiagaraStatelessModule_SpriteFacingAndAlignment::StaticClass()));
        if (!init || !facing) return false;
        init->SetIsModuleEnabled(true);init->LifetimeDistribution=FNiagaraDistributionRangeFloat(30.f);
        init->SpriteSizeDistribution.Mode=ENiagaraDistributionMode::Binding;init->SpriteSizeDistribution.ParameterBinding=size;
        facing->SetIsModuleEnabled(true);facing->bSpriteFacingEnabled=true;
        facing->SpriteFacing.Mode=ENiagaraDistributionMode::Binding;facing->SpriteFacing.ParameterBinding=normal;
        while(emitter->GetNumSpawnInfos()) { FGuid id=emitter->GetSpawnInfoByIndex(0)->SourceId; emitter->RemoveSpawnInfoBySourceId(id); }
        auto& spawn=emitter->AddSpawnInfo();spawn.Type=ENiagaraStatelessSpawnInfoType::Burst;spawn.Amount=FNiagaraDistributionRangeInt(1);spawn.bLoopCountLimitEnabled=true;spawn.LoopCountLimit=FNiagaraDistributionRangeInt(1);
        for(auto* renderer:emitter->GetRenderers()) if(auto* sprite=Cast<UNiagaraSpriteRendererProperties>(renderer))
        {sprite->Modify();sprite->Material=material;sprite->FacingMode=ENiagaraSpriteFacingMode::CustomFacingVector;sprite->Alignment=ENiagaraSpriteAlignment::Unaligned;sprite->SubImageSize=FVector2D(1.f);}
        emitter->PostEditChange();found=true;
    }
    system->PostEditChange();system->MarkPackageDirty();return found;
}
bool UEnemyFXTools::SetupGroundFX(UBlueprint* bp, UNiagaraSystem* system)
{
    if(!bp || !system || !bp->GeneratedClass->IsChildOf(AEnemyBomb::StaticClass())) return false;
    auto* scs=bp->SimpleConstructionScript.Get();USCS_Node* node=nullptr;
    for(auto* n:scs->GetAllNodes()) if(n->GetVariableName()==TEXT("GroundWarning")) node=n;
    if(!node){node=scs->CreateNode(UNiagaraComponent::StaticClass(),TEXT("GroundWarning"));scs->AddNode(node);node->SetParent(Cast<AActor>(bp->GeneratedClass->GetDefaultObject())->GetRootComponent());}
    auto* component=Cast<UNiagaraComponent>(node->ComponentTemplate);component->SetAsset(system);component->SetAutoActivate(false);component->SetAbsolute(false,true,true);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(bp);FKismetEditorUtilities::CompileBlueprint(bp);
    auto* graph=bp->UbergraphPages[0].Get();
    for(const auto& n:graph->Nodes) if(auto* e=Cast<UK2Node_Event>(n);e && e->EventReference.GetMemberName()==TEXT("Armed")) return false;
    const auto* schema=GetDefault<UEdGraphSchema_K2>();bool ok=true;
    auto add=[graph](UEdGraphNode* n,int x,int y){graph->AddNode(n);n->CreateNewGuid();n->AllocateDefaultPins();n->NodePosX=x;n->NodePosY=y;};
    auto call=[&](UClass* cls,FName name,int x,int y){auto* n=NewObject<UK2Node_CallFunction>(graph);n->SetFromFunction(cls->FindFunctionByName(name));add(n,x,y);return n;};
    auto get=[&](FName name,int x,int y){auto* n=NewObject<UK2Node_VariableGet>(graph);n->VariableReference.SetSelfMember(name);add(n,x,y);return n->FindPinChecked(name);};
    auto link=[&](UEdGraphPin* a,UEdGraphPin* b){ok &= schema->TryCreateConnection(a,b);};
    auto* e=NewObject<UK2Node_Event>(graph);e->EventReference.SetExternalMember(TEXT("Armed"),AEnemyBomb::StaticClass());e->bOverrideFunction=true;add(e,0,1400);
    auto* ref=get(TEXT("GroundWarning"),0,1600);
    auto* pos=call(USceneComponent::StaticClass(),TEXT("K2_SetWorldLocation"),300,1400);
    auto* rot=call(USceneComponent::StaticClass(),TEXT("K2_SetWorldRotation"),650,1400);
    auto* scale=call(USceneComponent::StaticClass(),TEXT("SetWorldScale3D"),1000,1400);
    auto* activate=call(UActorComponent::StaticClass(),TEXT("Activate"),1350,1400);
    UEdGraphPin* exec=e->FindPinChecked(UEdGraphSchema_K2::PN_Then);
    for(auto* n:{pos,rot,scale,activate}){link(exec,n->FindPinChecked(UEdGraphSchema_K2::PN_Execute));link(ref,n->FindPinChecked(UEdGraphSchema_K2::PN_Self));exec=n->FindPinChecked(UEdGraphSchema_K2::PN_Then);}
    link(get(TEXT("m_groundPoint"),0,1800),pos->FindPinChecked(TEXT("NewLocation")));
    auto* makeRot=call(UKismetMathLibrary::StaticClass(),TEXT("MakeRotFromZ"),300,1900);link(get(TEXT("m_groundNormal"),0,2000),makeRot->FindPinChecked(TEXT("Z")));link(makeRot->GetReturnValuePin(),rot->FindPinChecked(TEXT("NewRotation")));
    auto* divide=call(UKismetMathLibrary::StaticClass(),TEXT("Divide_DoubleDouble"),300,2200);link(get(TEXT("m_radius"),0,2200),divide->FindPinChecked(TEXT("A")));schema->TrySetDefaultValue(*divide->FindPinChecked(TEXT("B")),TEXT("50.0"));
    auto* vector=call(UKismetMathLibrary::StaticClass(),TEXT("MakeVector"),650,2200);for(FName p:{FName("X"),FName("Y"),FName("Z")})link(divide->GetReturnValuePin(),vector->FindPinChecked(p));link(vector->GetReturnValuePin(),scale->FindPinChecked(TEXT("NewScale")));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(bp);FKismetEditorUtilities::CompileBlueprint(bp);bp->MarkPackageDirty();return ok && bp->Status!=BS_Error;
}
bool UEnemyFXTools::StoneSurface(UMaterial* mat)
{
    if(!mat || !mat->bUseMaterialAttributes) return false;
    for(const auto& n:mat->GetExpressionCollection().Expressions) if(auto* p=Cast<UMaterialExpressionScalarParameter>(n);p && p->ParameterName==TEXT("StoneRoughness")) return true;
    auto create=[mat](UClass* cls,int x,int y){return UMaterialEditingLibrary::CreateMaterialExpression(mat,cls,x,y);};
    auto* set=Cast<UMaterialExpressionSetMaterialAttributes>(create(UMaterialExpressionSetMaterialAttributes::StaticClass(),800,600));set->Inputs[0]=mat->GetEditorOnlyData()->MaterialAttributes;
    auto scalar=[&](FName name,float value,int y){auto* n=Cast<UMaterialExpressionScalarParameter>(create(UMaterialExpressionScalarParameter::StaticClass(),300,y));n->ParameterName=name;n->DefaultValue=value;return n;};
    set->ConnectInputAttribute(MP_Metallic,scalar(TEXT("StoneMetallic"),0.f,650));set->ConnectInputAttribute(MP_Roughness,scalar(TEXT("StoneRoughness"),.93f,800));set->ConnectInputAttribute(MP_Specular,scalar(TEXT("StoneSpecular"),.12f,950));
    auto* color=Cast<UMaterialExpressionVectorParameter>(create(UMaterialExpressionVectorParameter::StaticClass(),0,1100));color->ParameterName=TEXT("StoneTint");color->DefaultValue=FLinearColor(.12f,.10f,.075f);
    auto* noise=Cast<UMaterialExpressionNoise>(create(UMaterialExpressionNoise::StaticClass(),-400,1300));noise->Scale=.2f;noise->Levels=3;
    auto* lerp=Cast<UMaterialExpressionLinearInterpolate>(create(UMaterialExpressionLinearInterpolate::StaticClass(),0,1300));lerp->ConstA=.55f;lerp->ConstB=1.f;lerp->Alpha.Connect(0,noise);
    auto* mul=Cast<UMaterialExpressionMultiply>(create(UMaterialExpressionMultiply::StaticClass(),400,1100));mul->A.Connect(0,color);mul->B.Connect(0,lerp);set->ConnectInputAttribute(MP_BaseColor,mul);
    mat->GetEditorOnlyData()->MaterialAttributes.Connect(0,set);UMaterialEditingLibrary::RecompileMaterial(mat);mat->MarkPackageDirty();return true;
}
//
//
#include "Materials/MaterialExpressionDesaturation.h"
bool UEnemyFXTools::AmberEye(UMaterial* mat)
{
    if(!mat || !mat->bUseMaterialAttributes) return false;
    auto create=[mat](UClass* c,int x,int y){return UMaterialEditingLibrary::CreateMaterialExpression(mat,c,x,y);};
    auto* get=Cast<UMaterialExpressionGetMaterialAttributes>(create(UMaterialExpressionGetMaterialAttributes::StaticClass(),700,0));get->MaterialAttributes=mat->GetEditorOnlyData()->MaterialAttributes;
    auto* gray=Cast<UMaterialExpressionDesaturation>(create(UMaterialExpressionDesaturation::StaticClass(),900,0));gray->Input.Connect(get->CreateOrGetOutputAttribute(MP_EmissiveColor),get);
    auto* color=Cast<UMaterialExpressionVectorParameter>(create(UMaterialExpressionVectorParameter::StaticClass(),900,200));color->ParameterName=TEXT("EyeAmber");color->DefaultValue=FLinearColor(2.f,.35f,.012f);
    auto* mul=Cast<UMaterialExpressionMultiply>(create(UMaterialExpressionMultiply::StaticClass(),1100,0));mul->A.Connect(0,gray);mul->B.Connect(0,color);
    auto* set=Cast<UMaterialExpressionSetMaterialAttributes>(create(UMaterialExpressionSetMaterialAttributes::StaticClass(),1400,0));set->Inputs[0]=mat->GetEditorOnlyData()->MaterialAttributes;set->ConnectInputAttribute(MP_EmissiveColor,mul);mat->GetEditorOnlyData()->MaterialAttributes.Connect(0,set);
    UMaterialEditingLibrary::RecompileMaterial(mat);mat->MarkPackageDirty();return true;
}
bool UEnemyFXTools::UpdateGroundBindings(UBlueprint* bp)
{
    if(!bp)return false;auto* graph=bp->UbergraphPages[0].Get();auto* schema=GetDefault<UEdGraphSchema_K2>();UK2Node_CallFunction* activate=nullptr;
    for(const auto& n:graph->Nodes) if(auto* f=Cast<UK2Node_CallFunction>(n);f && f->FunctionReference.GetMemberName()==TEXT("Activate"))activate=f;
    if(!activate)return false;bool ok=true;
    auto add=[graph](UEdGraphNode* n,int x,int y){graph->AddNode(n);n->CreateNewGuid();n->AllocateDefaultPins();n->NodePosX=x;n->NodePosY=y;};
    auto call=[&](UClass* cls,FName name,int x,int y){auto* n=NewObject<UK2Node_CallFunction>(graph);n->SetFromFunction(cls->FindFunctionByName(name));add(n,x,y);return n;};
    auto get=[&](FName name,int x,int y){auto* n=NewObject<UK2Node_VariableGet>(graph);n->VariableReference.SetSelfMember(name);add(n,x,y);return n->FindPinChecked(name);};
    auto link=[&](UEdGraphPin* a,UEdGraphPin* b){ok &= schema->TryCreateConnection(a,b);};
    auto* size=call(UNiagaraComponent::StaticClass(),TEXT("SetVariableVec2"),1300,1400);schema->TrySetDefaultValue(*size->FindPinChecked(TEXT("InVariableName")),TEXT("User.WarningSize"));
    auto* normal=call(UNiagaraComponent::StaticClass(),TEXT("SetVariableVec3"),1700,1400);schema->TrySetDefaultValue(*normal->FindPinChecked(TEXT("InVariableName")),TEXT("User.GroundNormal"));
    auto* ex=activate->FindPinChecked(UEdGraphSchema_K2::PN_Execute);if(ex->LinkedTo.Num()!=1)return false;auto* prev=ex->LinkedTo[0];schema->BreakSinglePinLink(prev,ex);
    link(prev,size->FindPinChecked(UEdGraphSchema_K2::PN_Execute));link(size->FindPinChecked(UEdGraphSchema_K2::PN_Then),normal->FindPinChecked(UEdGraphSchema_K2::PN_Execute));link(normal->FindPinChecked(UEdGraphSchema_K2::PN_Then),ex);activate->NodePosX=2100;
    auto* ref=get(TEXT("GroundWarning"),1300,1700);link(ref,size->FindPinChecked(UEdGraphSchema_K2::PN_Self));link(ref,normal->FindPinChecked(UEdGraphSchema_K2::PN_Self));link(get(TEXT("m_groundNormal"),1500,1900),normal->FindPinChecked(TEXT("InValue")));
    auto* multiply=call(UKismetMathLibrary::StaticClass(),TEXT("Multiply_DoubleDouble"),1100,2300);link(get(TEXT("m_radius"),800,2300),multiply->FindPinChecked(TEXT("A")));schema->TrySetDefaultValue(*multiply->FindPinChecked(TEXT("B")),TEXT("2.0"));auto* vector=call(UKismetMathLibrary::StaticClass(),TEXT("MakeVector2D"),1400,2200);for(FName p:{FName("X"),FName("Y")})link(multiply->GetReturnValuePin(),vector->FindPinChecked(p));link(vector->GetReturnValuePin(),size->FindPinChecked(TEXT("InValue")));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(bp);FKismetEditorUtilities::CompileBlueprint(bp);bp->MarkPackageDirty();return ok && bp->Status!=BS_Error;
}
//
//
FString UEnemyFXTools::CheckExplosion(UBlueprint* blueprint, bool repair)
{
    FString result;
    if (!blueprint) return result;
    for (const auto& graph : blueprint->UbergraphPages)
        for (const auto& node : graph->Nodes)
            if (auto* call=Cast<UK2Node_CallFunction>(node); call && call->FunctionReference.GetMemberName()==TEXT("SpawnSystemAtLocation"))
            {
                for (auto* pin : call->Pins)
                    result+=FString::Printf(TEXT("%s=%s links=%d; "),*pin->PinName.ToString(),*pin->DefaultValue,pin->LinkedTo.Num());
                if (repair)
                    for (const auto& pair : TArray<TPair<FName,FString>>{{TEXT("bAutoActivate"),TEXT("true")},{TEXT("bAutoDestroy"),TEXT("true")},{TEXT("bPreCullCheck"),TEXT("false")}})
                        if (auto* pin=call->FindPin(pair.Key)) GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*pin,pair.Value);
            }
    if (repair)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(blueprint);
        FKismetEditorUtilities::CompileBlueprint(blueprint);
        blueprint->MarkPackageDirty();
    }
    return result;
}
//
