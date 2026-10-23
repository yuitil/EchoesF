#include "Enemy/Animation/EnemyAnimBlueprintTools.h"
#include "Animation/AnimInstance.h"
#include "Animation/BlendSpace.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "UObject/UnrealType.h"
//
FString UEnemyAnimBlueprintTools::InspectMovement(UAnimInstance* instance)
{
    FString result;
    if(instance) for(TFieldIterator<FStructProperty> it(instance->GetClass());it;++it)
        if(it->Struct->GetFName()==FName(TEXT("AnimNode_BlendSpacePlayer")))
        {
            auto* node=it->ContainerPtrToValuePtr<FAnimNode_BlendSpacePlayer>(instance);
            result+=FString::Printf(TEXT("%s time=%f input=%s rate=%f loop=%d"),*GetNameSafe(node->GetBlendSpace()),node->GetCurrentAssetTime(),*node->GetPosition().ToString(),node->GetPlayRate(),node->IsLooping());
        }
    return result;
}
#if WITH_EDITOR
#include "BehaviorTree/BehaviorTree.h"
#include "Enemy/AI/BossPeformAction.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/BlendSpace.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "Animation/AnimSequence.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimationStateMachineGraph.h"
#include "AnimStateNode.h"
#include "AnimStateEntryNode.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Enemy/Animation/BossAnimation.h"
#endif
//
bool UEnemyAnimBlueprintTools::SetMovementUpperPose(UAnimBlueprint* blueprint,UAnimSequence* pose,FName bone)
{
    if(!blueprint || !pose || blueprint->TargetSkeleton!=pose->GetSkeleton()) return false;
    TArray<UEdGraph*> graphs; blueprint->GetAllGraphs(graphs);
    for(auto* graph:graphs)
    {
        UAnimGraphNode_BlendSpacePlayer* move=nullptr;
        for(UEdGraphNode* node:graph->Nodes)
        {
            if(Cast<UAnimGraphNode_LayeredBoneBlend>(node)) return false;
            if(auto* candidate=Cast<UAnimGraphNode_BlendSpacePlayer>(node)) move=candidate;
        }
        if(!move) continue;
        auto* output=move->FindPin(TEXT("Pose"));
        if(!output || output->LinkedTo.IsEmpty()) return false;
        auto targets=output->LinkedTo;
        blueprint->Modify();
        auto* upper=NewObject<UAnimGraphNode_SequencePlayer>(graph);
        graph->AddNode(upper);upper->CreateNewGuid();upper->PostPlacedNewNode();upper->Node.SetSequence(pose);upper->Node.SetLoopAnimation(true);upper->AllocateDefaultPins();
        upper->NodePosX=move->NodePosX;upper->NodePosY=move->NodePosY+220;
        auto* layer=NewObject<UAnimGraphNode_LayeredBoneBlend>(graph);
        graph->AddNode(layer);layer->CreateNewGuid();layer->PostPlacedNewNode();
        FBranchFilter filter;filter.BoneName=bone;filter.BlendDepth=1;
        layer->Node.LayerSetup[0].BranchFilters.Add(filter);layer->Node.BlendWeights[0]=1.f;layer->Node.bMeshSpaceRotationBlend=true;
        layer->AllocateDefaultPins();layer->NodePosX=move->NodePosX+300;layer->NodePosY=move->NodePosY;
        UEdGraphPin* blendPin=nullptr;
        for(auto* pin:layer->Pins) if(pin->Direction==EGPD_Input && pin->PinName.ToString().StartsWith(TEXT("BlendPoses"))) {blendPin=pin;break;}
        auto* schema=graph->GetSchema();
        if(!blendPin || !schema->TryCreateConnection(upper->FindPin(TEXT("Pose")),blendPin)) return false;
        output->BreakAllPinLinks();
        if(!schema->TryCreateConnection(output,layer->FindPin(TEXT("BasePose")))) return false;
        for(auto* target:targets) if(!schema->TryCreateConnection(layer->FindPin(TEXT("Pose")),target)) return false;
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);FKismetEditorUtilities::CompileBlueprint(blueprint);
        return blueprint->Status!=BS_Error;
    }
    return false;
}
//
bool UEnemyAnimBlueprintTools::ReplaceLocomotion(UAnimBlueprint* blueprint,UBlendSpace* blend)
{
#if WITH_EDITOR
    if(!blueprint || !blend || blueprint->TargetSkeleton!=blend->GetSkeleton()) return false;
    TArray<UEdGraph*> graphs; blueprint->GetAllGraphs(graphs); bool changed=false;
    for(auto* graph:graphs) for(UEdGraphNode* node:graph->Nodes)
        if(auto* player=Cast<UAnimGraphNode_BlendSpacePlayer>(node))
        {
            blueprint->Modify(); player->Modify(); player->Node.SetBlendSpace(blend);
            player->PostPlacedNewNode();
            player->Node.SetLoop(true); player->Node.SetPlayRate(1.f); changed=true;
            for(auto& property:player->ShowPinForProperties)
                if(property.PropertyName==TEXT("X") || property.PropertyName==TEXT("Y")) property.bShowPin=true;
            player->ReconstructNode();
            for(int32 axis=0;axis<2;++axis)
            {
                auto* input=player->FindPin(axis==0?TEXT("X"):TEXT("Y"));
                if(!input) continue;
                const FName name=blend->GetBlendParameter(axis).DisplayName.Contains(TEXT("Direction"))?TEXT("m_direction"):TEXT("m_speed");
                input->BreakAllPinLinks();
                auto* getter=NewObject<UK2Node_VariableGet>(graph);
                graph->AddNode(getter); getter->CreateNewGuid(); getter->PostPlacedNewNode();
                getter->VariableReference.SetSelfMember(name); getter->AllocateDefaultPins();
                getter->NodePosX=player->NodePosX-250; getter->NodePosY=player->NodePosY+axis*120;
                if(!graph->GetSchema()->TryCreateConnection(getter->GetValuePin(),input)) return false;
            }
        }
    if(changed)
    {
        TArray<UEdGraphNode*> unused;
        for(auto* graph:graphs) for(UEdGraphNode* node:graph->Nodes)
            if(auto* getter=Cast<UK2Node_VariableGet>(node))
                if(auto* pin=getter->GetValuePin(); pin && pin->LinkedTo.IsEmpty() && (getter->VariableReference.GetMemberName()==TEXT("m_speed") || getter->VariableReference.GetMemberName()==TEXT("m_direction"))) unused.Add(node);
        for(auto* node:unused) FBlueprintEditorUtils::RemoveNode(blueprint,node,false);
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint); FKismetEditorUtilities::CompileBlueprint(blueprint);
    }
    return changed && blueprint->Status!=BS_Error;
#else
    return false;
#endif
}
//
FString UEnemyAnimBlueprintTools::InspectLocomotion(UAnimBlueprint* blueprint)
{
    FString result;
#if WITH_EDITOR
    if(!blueprint) return result;
    TArray<UEdGraph*> graphs; blueprint->GetAllGraphs(graphs);
    for(auto* graph:graphs) for(UEdGraphNode* node:graph->Nodes)
    {
        if(auto* player=Cast<UAnimGraphNode_BlendSpacePlayer>(node))
        {
            result+=graph->GetName()+TEXT(" ")+GetNameSafe(player->Node.GetBlendSpace())+TEXT("\n");
            for(auto* pin:node->Pins)
            {
                result+=pin->PinName.ToString()+TEXT("=")+pin->DefaultValue+TEXT(" ");
                for(auto* link:pin->LinkedTo) result+=link->GetOwningNode()->GetName()+TEXT(".")+link->PinName.ToString()+TEXT(" ");
                result+=TEXT("\n");
            }
        }
    }
#endif
    return result;
}
//
bool UEnemyAnimBlueprintTools::WrapLocomotion(UAnimBlueprint* blueprint)
{
#if WITH_EDITOR
    if (!blueprint) return false;
    for (UEdGraph* graph : blueprint->FunctionGraphs)
    {
        UAnimGraphNode_BlendSpacePlayer* player = nullptr;
        UAnimGraphNode_Slot* slot = nullptr;
        for (UEdGraphNode* node : graph->Nodes)
        {
            if (Cast<UAnimGraphNode_StateMachine>(node)) return true;
            if (auto* found = Cast<UAnimGraphNode_BlendSpacePlayer>(node)) player = found;
            if (auto* found = Cast<UAnimGraphNode_Slot>(node)) slot = found;
        }
        if (!player || !slot) continue;
        blueprint->Modify(); graph->Modify();
        auto* machine = NewObject<UAnimGraphNode_StateMachine>(graph);
        graph->AddNode(machine); machine->CreateNewGuid();
        machine->PostPlacedNewNode(); machine->AllocateDefaultPins();
        machine->NodePosX = -450;
        auto* states = machine->EditorStateMachineGraph.Get();
        FBlueprintEditorUtils::RenameGraph(states, TEXT("Locomotion"));
        auto* ground = NewObject<UAnimStateNode>(states);
        states->AddNode(ground); ground->CreateNewGuid();
        ground->PostPlacedNewNode(); ground->AllocateDefaultPins(); ground->NodePosX = 250;
        auto* inner = ground->GetBoundGraph();
        FBlueprintEditorUtils::RenameGraph(inner, TEXT("Ground"));
        bool ok = false;
        for (UEdGraphNode* node : states->Nodes)
            if (auto* entry = Cast<UAnimStateEntryNode>(node))
                ok = states->GetSchema()->TryCreateConnection(entry->Pins[0],ground->GetInputPin());
        player->FindPin(TEXT("Pose"))->BreakAllPinLinks();
        TArray<UEdGraphNode*> moving;
        moving.Add(player);
        for (UEdGraphNode* node : graph->Nodes)
            if (Cast<UK2Node_VariableGet>(node)) moving.Add(node);
        for (auto* node : moving)
        {
            graph->Nodes.Remove(node);
            node->Rename(nullptr,inner,REN_DontCreateRedirectors);
            inner->Nodes.Add(node);
        }
        ok &= inner->GetSchema()->TryCreateConnection(player->FindPin(TEXT("Pose")),ground->GetPoseSinkPinInsideState());
        ok &= graph->GetSchema()->TryCreateConnection(machine->FindPin(TEXT("Pose")),slot->FindPin(TEXT("Source")));
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);
        FKismetEditorUtilities::CompileBlueprint(blueprint);
        return ok && blueprint->Status != BS_Error;
    }
#endif
    return false;
}
//
bool UEnemyAnimBlueprintTools::SetupLocomotion(UAnimBlueprint* blueprint, UBlendSpace* blend)
{
#if WITH_EDITOR
    if (!blueprint || !blend || blueprint->TargetSkeleton != blend->GetSkeleton()) return false;
    TArray<UEdGraph*> graphs;
    blueprint->GetAllGraphs(graphs);
    for (auto* graph : graphs)
    {
        UAnimGraphNode_Root* root = nullptr;
        for (UEdGraphNode* node : graph->Nodes) if (auto* found = Cast<UAnimGraphNode_Root>(node)) root = found;
        if (!root) continue;
        if (graph->Nodes.Num() != 1) return false;
        blueprint->Modify(); graph->Modify();
        auto* player = NewObject<UAnimGraphNode_BlendSpacePlayer>(graph);
        graph->AddNode(player); player->CreateNewGuid();
        player->PostPlacedNewNode();
        player->Node.SetBlendSpace(blend);
        for (auto& property : player->ShowPinForProperties)
            if (property.PropertyName == TEXT("X") || property.PropertyName == TEXT("Y")) property.bShowPin = true;
        player->AllocateDefaultPins(); player->NodePosX = -450;
        auto* slot = NewObject<UAnimGraphNode_Slot>(graph);
        graph->AddNode(slot); slot->CreateNewGuid(); slot->Node.SlotName = TEXT("DefaultSlot");
        slot->PostPlacedNewNode();
        slot->AllocateDefaultPins(); slot->NodePosX = -200;
        const auto* schema = graph->GetSchema();
        bool ok = schema->TryCreateConnection(player->FindPin(TEXT("Pose")),slot->FindPin(TEXT("Source")));
        ok &= schema->TryCreateConnection(slot->FindPin(TEXT("Pose")),root->FindPin(TEXT("Result")));
        for (int32 axis=0; axis<2; ++axis)
        {
            const FString label = blend->GetBlendParameter(axis).DisplayName.ToLower();
            const FName member = label.Contains(TEXT("direction")) ? TEXT("m_direction") : TEXT("m_speed");
            auto* getter = NewObject<UK2Node_VariableGet>(graph);
            graph->AddNode(getter); getter->CreateNewGuid();
            getter->VariableReference.SetExternalMember(member,UBossAnimInstance::StaticClass());
            getter->AllocateDefaultPins(); getter->NodePosX = -700; getter->NodePosY = axis*120;
            if (auto* pin = player->FindPin(axis == 0 ? TEXT("X") : TEXT("Y")))
                ok &= schema->TryCreateConnection(getter->GetValuePin(),pin);
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);
        FKismetEditorUtilities::CompileBlueprint(blueprint);
        return ok && blueprint->Status != BS_Error;
    }
#endif
    return false;
}
//

//
int32 UEnemyAnimBlueprintTools::SetupMinionTree(UBehaviorTree* tree)
{
#if WITH_EDITOR
    if (!tree) return 0;
    TArray<UObject*> objects;
    GetObjectsWithOuter(tree,objects,true);
    int32 count = 0;
    for (auto* object : objects)
    {
        if (auto* task = Cast<UBossPeformAction>(object))
        {
            task->Modify();
            task->m_attackDist = 100.f;
            task->m_rushAction = NAME_None;
            task->m_retreatAction = NAME_None;
            ++count;
        }
    }
    tree->MarkPackageDirty();
    return count;
#else
    return 0;
#endif
}
//

//
#include "Enemy/AI/BossUpdateCombat.h"
#include "Enemy/AI/MinionUpdateCombat.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "AIGraphNode.h"
int32 UEnemyAnimBlueprintTools::SplitMinionService(UBehaviorTree* tree)
{
    if (!tree) return 0;
    TArray<UObject*> objects;
    GetObjectsWithOuter(tree,objects,true);
    TMap<UObject*,UObject*> replacements;
    for (auto* object : objects)
    {
        if (auto* old = Cast<UBossUpdateCombat>(object))
        {
            auto* service = NewObject<UMinionUpdateCombat>(old->GetOuter(),NAME_None,RF_Transactional);
            service->InitializeNode(old->GetParentNode(),old->GetExecutionIndex(),0,old->GetTreeDepth());
            replacements.Add(old,service);
        }
    }
    objects.Add(tree);
    for (auto* object : objects)
    {
        object->Modify();
        FArchiveReplaceObjectRef<UObject> archive(object,replacements,EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::IgnoreArchetypeRef);
        if (auto* graphNode = Cast<UAIGraphNode>(object); graphNode && Cast<UMinionUpdateCombat>(graphNode->NodeInstance))
            graphNode->ClassData = FGraphNodeClassData(UMinionUpdateCombat::StaticClass(),FString());
    }
    tree->MarkPackageDirty();
    return replacements.Num();
}
//
//
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTService.h"
FString UEnemyAnimBlueprintTools::InspectServices(UBehaviorTree* tree)
{
    FString result;
    TFunction<void(UBTCompositeNode*)> visit=[&](UBTCompositeNode* node)
    {
        if (!node) return;
        for (const auto& service : node->Services) if (service) result+=service->GetClass()->GetName()+TEXT(";");
        for (const auto& child : node->Children)
        {
            visit(child.ChildComposite);
            if (child.ChildTask) for (const auto& service : child.ChildTask->Services) if(service) result+=service->GetClass()->GetName()+TEXT(";");
        }
    };
    if (tree) visit(tree->RootNode);
    return result;
}
//
