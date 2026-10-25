#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossShadowSummon.generated.h"

class ABossShadow;

//連携や分身との戦闘に使う影を一体だけ呼ぶ。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UBossShadowSummon : public UActorComponent
{
    GENERATED_BODY()

public:
    //一定間隔で分身を確認する。
    UBossShadowSummon();
    //戦闘をやり直す時に影と履歴を片付ける。
    void Reset();

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

protected:
    //分身とボスの状態に合わせて影を管理する。
    virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;
    //マップを閉じた時にも影を残さない。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
    //同じ攻撃で何度も連携を抽選しないための番号。
    int32 m_lastJoin = INDEX_NONE;

    //一定間隔で数えた、範囲内の分身。
    int32 m_cloneCount = 0;

    //次に影を呼べる時刻。
    double m_nextTime = 0.;
};
