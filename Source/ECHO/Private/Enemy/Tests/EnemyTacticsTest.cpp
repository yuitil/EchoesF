#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Enemy/AI/EnemyPlayerHistory.h"
#include "Enemy/Boss/BossEnemyCharacter.h"
#include "Enemy/Boss/BossObservation.h"
#include "Enemy/Boss/BossData.h"
#include "Enemy/Boss/BossPosture.h"
#include "Enemy/Combat/Defense.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/Minion/MinionEnemyCharacter.h"
#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Minion/MinionCombatDirector.h"

//分身への人数制限と、記録がない時の判断を確認する。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyTacticsTest, "ECHO.Enemy.Tactics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyTacticsTest::RunTest(const FString& _parameters)
{
    UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, TEXT("EnemyTacticsWorld"));
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
    auto* director = world->GetSubsystem<UMinionCombatDirector>();
    auto* history = world->GetSubsystem<UEnemyPlayerHistory>();
    auto* player = world->SpawnActor<AActor>();
    auto* clone = world->SpawnActor<AActor>();
    TArray<AMinionEnemyCharacter*> enemies;
    FActorSpawnParameters spawn;
    spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (int32 i = 0; i < 4; ++i)
    {
        auto* enemy = world->SpawnActor<AMinionEnemyCharacter>(FVector(i * 100.f, 0.f, 100.f), FRotator::ZeroRotator, spawn);
        enemies.Add(enemy);
        director->Register(enemy->m_formation);
        enemy->m_combatTarget = player;
    }
    TestTrue(TEXT("Four enemies can assign one clone defender"), director->CanAssignClone(enemies[0], player, clone, 2000.f, .5f, 1));
    enemies[1]->m_combatTarget = clone;
    TestFalse(TEXT("Do not crowd the same clone"), director->CanAssignClone(enemies[0], player, clone, 2000.f, .5f, 1));
    auto* second = world->SpawnActor<AActor>();
    TestTrue(TEXT("A second clone can have another defender"), director->CanAssignClone(enemies[0], player, second, 2000.f, .5f, 1));
    enemies[2]->m_combatTarget = second;
    TestFalse(TEXT("Keep half the group for the player"), director->CanAssignClone(enemies[0], player, world->SpawnActor<AActor>(), 2000.f, .5f, 1));
    enemies[1]->m_state = EEnemyState::Dead;
    enemies[2]->m_state = EEnemyState::Dead;
    enemies[3]->m_state = EEnemyState::Dead;
    TestFalse(TEXT("The last enemy stays with the player"), director->CanAssignClone(enemies[0], player, clone, 2000.f, .5f, 1));

    auto* boss = world->SpawnActor<ABossEnemyCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, spawn);
    auto* observer = boss->FindComponentByClass<UBossObservation>();
    TestEqual(TEXT("No history keeps normal attacks"), observer->AdaptWeight(uint8(EEnemyAction::Counter)), 1.f);
    history->m_attacks = 6;
    history->m_repeatRate = .8f;
    TestTrue(TEXT("Phase one uses earlier repeated attacks"), observer->AdaptWeight(uint8(EEnemyAction::Counter)) > 1.f);
    observer->Freeze();
    const float before = observer->AdaptWeight(uint8(EEnemyAction::Counter));
    history->m_repeatRate = .1f;
    TestTrue(TEXT("Phase two still follows changed habits"), observer->AdaptWeight(uint8(EEnemyAction::Counter)) < before);
    observer->m_useHistory = false;
    TestEqual(TEXT("BP can disable shared history"), observer->AdaptWeight(uint8(EEnemyAction::Counter)), 1.f);

    //履歴がない初対面でも、見えている移動から追跡技を選べる。
    history->m_attacks = 0;
    observer->Reset();
    observer->m_useHistory = true;
    auto* seenPlayer = world->SpawnActor<ACharacter>(FVector(300.f, 0.f, 100.f), FRotator::ZeroRotator, spawn);
    seenPlayer->GetCharacterMovement()->Velocity = FVector(300.f, 0.f, 0.f);
    observer->ReadMotion(seenPlayer);
    FEnemyActionData chase;
    chase.m_kind = EEnemyAction::Chase;
    chase.m_maxRange = 1000.f;
    FEnemyActionData slash;
    slash.m_kind = EEnemyAction::Slash;
    slash.m_maxRange = 350.f;
    const float newChase = observer->GetActionWeight(chase);
    TestTrue(TEXT("Unobserved retreat favors chase over a short slash"), newChase > observer->GetActionWeight(slash));
    TestTrue(TEXT("Prediction raises chase before any attack history"), newChase > 1.f);
    history->m_attacks = observer->m_knownAttacks;
    TestEqual(TEXT("Enough history reaches full confidence"), observer->GetConfidence(), 1.f);
    TestTrue(TEXT("Learned mode keeps prediction but lowers its dominance"), observer->GetActionWeight(chase) < newChase && observer->GetActionWeight(chase) > 1.f);
    seenPlayer->SetActorRotation(FRotator(0.f, 180.f, 0.f));
    seenPlayer->GetCharacterMovement()->Velocity = FVector(-300.f, 0.f, 0.f);
    observer->ReadMotion(seenPlayer);
    FEnemyActionData counter = slash;
    counter.m_kind = EEnemyAction::Counter;
    FEnemyActionData heavy = slash;
    heavy.m_kind = EEnemyAction::Heavy;
    TestTrue(TEXT("Visible approach favors counter over slow heavy attack"), observer->PredictWeight(counter) > observer->PredictWeight(heavy));
    observer->m_usePrediction = false;
    TestEqual(TEXT("BP can turn prediction off"), observer->PredictWeight(chase), 0.f);
    //通常被弾は少しずつ減り、崩れている間は追加で減らさない。
    auto* posture = boss->FindComponentByClass<UBossPosture>();
    TestTrue(TEXT("Test boss starts alive"), boss->IsCombatActive());
    posture->m_hitCost = 5.f;
    posture->m_damageScale = 0.f;
    posture->Reset();
    posture->Hit(500.f);
    TestEqual(TEXT("Large HP damage still costs only five posture"), posture->m_value, 95.f);
    posture->Hit(0.f);
    TestEqual(TEXT("Zero damage does not reduce posture"), posture->m_value, 95.f);
    boss->m_defense->m_bGuardBroken = true;
    posture->Hit(50.f);
    TestEqual(TEXT("Broken posture does not take additional hit cost"), posture->m_value, 95.f);
    boss->m_defense->m_bGuardBroken = false;
    FFileHelper::SaveStringToFile(HasAnyErrors() ? TEXT("FAILED") : TEXT("PASSED: clone budget, one defender per clone, lone enemy, no-history fallback, phase-one adaptation, phase-two updates, BP disable"), *(FPaths::ProjectSavedDir() / TEXT("enemy_tactics_assertions.txt")));
    world->DestroyWorld(false);
    GEngine->DestroyWorldContext(world);
    return true;
}
#endif
