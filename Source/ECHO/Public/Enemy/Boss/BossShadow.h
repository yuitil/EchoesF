#pragma once

#include "CoreMinimal.h"
#include "Enemy/Boss/BossEnemyCharacter.h"
#include "BossShadow.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UNiagaraComponent;

//分身との戦闘、または本体との一度の連携を行う影。
UCLASS()
class ECHO_API ABossShadow : public ABossEnemyCharacter
{
    GENERATED_BODY()

public:
    //影の周りに煙を出す部品を用意する。
    ABossShadow();

    //生成直後に相手と活動時間を決める。
    void Setup(AActor* _target, float _life);
    //本体の攻撃に続く、一度だけの連携を始める。
    void StartJoin(FName _action, float _delay, float _range);
    //攻撃を止めて影を消し始める。
    void FadeOut();
    //出現中と消滅中は移動や攻撃を待たせる。
    virtual bool IsRepositioning() const override;
    //出現と消滅の見た目を更新する。
    virtual void Tick(float _delta) override;

public:
    //体から漏れる煙。使用するNiagaraはBPで選ぶ。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shadow|FX")
    TObjectPtr<UNiagaraComponent> m_aura;

    //出現と消滅で共通に使う、地面の煙。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|FX")
    TObjectPtr<UNiagaraSystem> m_smoke;

    //地面に開く煙の輪。素材はBPで選ぶ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|FX")
    TObjectPtr<UNiagaraSystem> m_gate;

    //地面へ広がる煙の大きさ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|FX")
    FVector m_smokeScale = FVector::OneVector;

    //空中から消える時も地面を探す距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|FX", meta=(ClampMin="1"))
    float m_floorDepth = 2000.f;

    //煙が床に埋まらないよう浮かせる高さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|FX", meta=(ClampMin="0"))
    float m_floorOffset = 6.f;

    //影の色と模様。マテリアルはBPで選ぶ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Look")
    TObjectPtr<UMaterialInterface> m_shadowMat;
    //マテリアルの出現率を変える名前。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Look")
    FName m_fadeParam = TEXT("ShadowFade");
    //地面から姿を現す時間。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Look", meta=(ClampMin="0.1"))
    float m_riseTime = 1.f;
    //消えていく時間。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Look", meta=(ClampMin="0.1"))
    float m_fadeTime = .6f;
    //近づけない時などに連携を打ち切る時間。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Combat", meta=(ClampMin="1"))
    float m_joinLimit = 12.f;

    //コンボ終了後に追加する待ち時間の最小値。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Combat", meta=(ClampMin="0"))
    float m_waitMin = .2f;
    //コンボ終了後に追加する待ち時間の最大値。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow|Combat", meta=(ClampMin="0"))
    float m_waitMax = .8f;

protected:
    //本体用のHP表示と召喚を開始前に無効にする。
    virtual void PostInitializeComponents() override;
    //BPの見た目へ影のマテリアルを設定する。
    virtual void BeginPlay() override;
    //影には第二のHPを与えず消滅させる。
    virtual void HandleHealthDepleted() override;
    //本体のフェーズ2と同じ候補から選ぶ。
    virtual bool IsActionAllowed(const FEnemyActionData& _action) const override;
    //影の攻撃は観察補正なしで抽選する。
    virtual float GetActionWeightMultiplier(const FEnemyActionData& _action) const override;
    //コンボの後に短いランダムな間を入れる。
    virtual void OnAttackFinished() override;
    //中断された場合も連携を終えて片付ける。
    virtual void OnActionEnded() override;

private:
    //活動中のプレイヤーの分身を探す。
    AActor* FindClone() const;

    //足元の地面へ煙を一度だけ出す。
    void PlaySmoke();

private:
    //この影だけに使うマテリアル。
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> m_mats;

private:
    //連携で最初に使う攻撃。未設定なら既存の候補から選ぶ。
    FName m_joinAction;
    //連携中だけ指定した攻撃で開始する。
    bool m_join = false;
    //連携のコンボを終えたか。
    bool m_joinDone = false;
    //攻撃を始めてよい時刻。
    float m_joinDelay = 0.f;
    //分身を探す距離。
    float m_cloneRange = 0.f;

    //生成されてからの時間。
    float m_age = 0.f;
    //消えるまでの活動時間。
    float m_life = 45.f;
    //消え始めてからの時間。
    float m_fadeAge = 0.f;
    //消滅を開始したか。
    bool m_fading = false;
};
