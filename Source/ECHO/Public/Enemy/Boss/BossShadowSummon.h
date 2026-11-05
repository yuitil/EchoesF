#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossShadowSummon.generated.h"

class ABossShadow;
class ABossEnemyCharacter;
class UAnimMontage;

//分身の数に合わせて、連携する影を最大二体呼ぶ。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UBossShadowSummon : public UActorComponent
{
    GENERATED_BODY()

public:
    //一定間隔で分身を確認する。
    UBossShadowSummon();
    //戦闘をやり直す時に影と履歴を片付ける。
    void Reset();

    //持ち替えや入れ替えの動作が終わるまで別の技を待つ。
    bool IsChanging() const;
    //本体のコンボの切れ目を記録する。
    void QueueRelay();
    //交代できる位置にいる影だけ、次の一撃を少し待つ。
    bool CanCover(const ABossShadow& _partner) const;

    //入れ替えに使う短い足運び。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Swap")
    TObjectPtr<UAnimMontage> m_swapMove;
    //同じ戦闘で連続して入れ替わらないための秒数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Swap",meta=(ClampMin="1"))
    float m_swapDelay = 12.f;
    //どのくらい追跡が止まったら脱出するか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Swap",meta=(ClampMin="1"))
    float m_stuckTime = 2.f;
    //このHPの割合以下で、相手の攻撃から離れる。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Swap",meta=(ClampMin="0",ClampMax="1"))
    float m_panicHP = .35f;
    //追跡が詰まったかを調べ始める距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Stance",meta=(ClampMin="0"))
    float m_drawRange = 500.f;
    //交代後に影が狙う地上攻撃。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Swap")
    FName m_relayAction;

    //近くにいるプレイヤーの分身の数。
    int32 GetCloneCount() const { return m_cloneCount; }

public:
    //フェーズ1でも連携を使うか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Join")
    bool m_phaseOneJoin = true;

    //対象の攻撃一回につき、連携を使う確率。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Join", meta=(ClampMin="0", ClampMax="1"))
    float m_joinChance = .25f;

    //フェーズ1で影を呼ぶ、近くの分身の数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="1"))
    int32 m_cloneLimit = 2;

    //影を二体に増やす、近くの分身の数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="2"))
    int32 m_twoCloneCount = 5;

    //分身に囲まれた時に選びやすくする攻撃。BPで選ぶ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow")
    TArray<FName> m_crowdMoves;

    //囲まれた時の攻撃の選びやすさ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="1"))
    float m_crowdWeight = 1.5f;

    //召喚する影のBP。影自身では未設定にする。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow")
    TSubclassOf<ABossShadow> m_shadowClass;
    //分身を見つける範囲。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="0"))
    float m_range = 3000.f;
    //影を出す本体からの距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="100"))
    float m_spawnRange = 250.f;
    //影の活動時間の最小値。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="1"))
    float m_lifeMin = 30.f;
    //影の活動時間の最大値。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="1"))
    float m_lifeMax = 60.f;
    //別の分身へ影を呼ぶまでの最短間隔。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="0"))
    float m_cooldown = 10.f;
    //分身がいない時にも連携を使うか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Join")
    bool m_enableJoin = true;
    //連携のきっかけにする本体の攻撃。BPで選ぶ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Join")
    TArray<FName> m_joinMoves;
    //影が最初に使う攻撃名。影の攻撃データにある名前を入れる。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Join")
    FName m_joinAction;
    //影が動き出すまでの間。出現より短くはならない。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Join", meta=(ClampMin="0"))
    float m_joinDelay = 1.f;
    //連携を始めてよい距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Join", meta=(ClampMin="0"))
    float m_joinRange = 600.f;
    //次の連携までの待ち時間。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Join", meta=(ClampMin="1"))
    float m_joinCooldown = 25.f;
    //現在呼び出している影。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shadow")
    TWeakObjectPtr<ABossShadow> m_shadow;

    //分身が多い時に呼ぶ二体目の影。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shadow")
    TWeakObjectPtr<ABossShadow> m_secondShadow;

protected:
    //分身とボスの状態に合わせて影を管理する。
    virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;
    //マップを閉じた時にも影を残さない。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
    //同じ攻撃で何度も連携を抽選しないための番号。
    int32 m_lastJoin = INDEX_NONE;
    //壁、相手の攻撃、コンボの順に交代できるか調べる。
    bool TryStep(ABossEnemyCharacter& _boss, double _now);
    //影が姿を現すまで、危機から離れる目的を覚えておく。
    bool m_rescuePanic = false;
    //生まれた影を待つ期限。
    double m_rescueEnd = 0.;
    //再び交代できる時刻。
    double m_nextSwap = 0.;
    //追跡中に足が止まった時刻。
    double m_blockedSince = 0.;
    //味方へ次の一撃を渡せる期限。
    double m_relayEnd = 0.;
    //追跡で最後に進んだ場所。
    FVector m_lastPlace = FVector::ZeroVector;
    //いま自分で始めた持ち替えかステップ。
    TWeakObjectPtr<UAnimMontage> m_changeMove;

    //一定間隔で数えた、範囲内の分身。
    int32 m_cloneCount = 0;

    //次に影を呼べる時刻。
    double m_nextTime = 0.;
};




