#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat.generated.h"

class UMeshComponent;
class UAnimMontage;
class UCameraShakeBase;

//武器が当たった相手へダメージを渡す。
UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class ECHO_API UCombat : public UActorComponent
{
    GENERATED_BODY()

public:

    //武器が当たった相手へダメージを渡す。
    UCombat();

    //BPで指定した剣の判定を接続する。
    virtual void BeginPlay() override;

    //打ち上げ後の重力を元に戻す。
    void EndLift();

    //終了時にタイマーや登録を解除する。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;

    //剣の攻撃判定を有効にする。
    void BeginHitWindow(float _damageScale = 1.0f, FName _bone = NAME_None, FVector _tip = FVector::ZeroVector, bool _leftBlade = false);

    //剣の攻撃判定を終了する。
    void EndHitWindow();

    //終了通知を出した手の判定だけを閉じる。
    void EndHand(bool left);

    //爆発が回避開始位置まで届いたか調べる。
    bool DodgedBlast(AActor* target, FVector point, float radius, float window) const;

    //抜刀・納刀の通知で左の剣を出し入れする。
    void UpdateLeftBlade(bool _show);

    //左手に刀が出ているかを返す。
    UFUNCTION(BlueprintPure, Category = "Enemy|Sword")
    bool HasTwoSwords() const;

    //BPで左の刀に付けるタグ。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Sword")
    FName m_leftWeaponTag;

    //次の技に必要な持ち替えを返す。
    UAnimMontage* GetBladeChange(bool _dual) const;

    //攻撃判定と演出で同じ剣の位置を読む。
    bool GetBladePoints(bool _left, FVector& _base, FVector& _tip) const;

    //当たり判定の位置を返す。
    bool ReadPoints(bool left, FVector& base, FVector& tip) const;

    //指定した手が今ダメージを与えられるか。
    UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
    bool IsHandOpen(bool left) const { return m_hands[left].m_bHitWindow; }

    //攻撃判定が有効か返す。
    bool IsHitWindowOpen() const { return m_hands[0].m_bHitWindow || m_hands[1].m_bHitWindow; }

    //攻撃と防御の判定を初期化する。
    void ResetWindows();

    //経過時間に合わせて敵の状態を更新する。
    virtual void TickComponent( float _deltaTime, ELevelTick _type, FActorComponentTickFunction* _function) override;

public:

    //攻撃判定に使う武器を設定する。
    UFUNCTION(BlueprintCallable, Category = "Enemy | Combat")
    bool SetWeaponMesh(UMeshComponent* _mesh, FName _base = TEXT("BladeBase"), FName _tip = TEXT("BladeTip"));

public:

    //大技がプレイヤーに当たった時の揺れ。敵BPで選ぶ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Impact")
    TSubclassOf<UCameraShakeBase> m_hitShake;

    //一撃の揺れの強さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Impact", meta = (ClampMin = "0", ClampMax = "2"))
    float m_shakeScale = .7f;

    //狙っている相手以外も、剣が直接当たれば判定する。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
    bool m_hitNearby = false;

    //BPの剣部品に付けるタグ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
    FName m_weaponTag;

    //攻撃判定の根元ソケット。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "WeaponBaseSocket"))
    FName m_baseSocket = TEXT("hand_r");

    //攻撃判定の先端ソケット。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "WeaponTipSocket"))
    FName m_tipSocket = TEXT("SwordTip");

    //判定に使う半径。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Radius", ClampMin = "1"))
    float m_radius = 12.f;

    //確認用の判定を表示するか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Debug"))
    bool m_bDebug = false;

    //回避の無敵時間中か。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Invulnerable"))
    bool m_bInvulnerable = false;

    //ガード判定が有効か。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Guarding"))
    bool m_bGuarding = false;

private:
    //刃が回避前の体に届いたか調べ、成功なら相手に知らせる。
    void CheckPerfectDodge(class AEnemyCharacterBase& _enemy, const FVector& _start, const FVector& _end);

    //剣のタグから判定部品をつなぎ直す。
    void BindWeapon();

    //BPに置いた左の刀。表示と命中判定で同じ部品を使う。
    UPROPERTY(Transient)
    TObjectPtr<UMeshComponent> m_leftWeapon;

    //同時に振った刀も、一振りずつ命中を記録する。
    struct FHitWindow
    {
        //今の通知で指定された武器のボーン。
        FName m_hitBone;

        //今の通知で指定された剣先の位置。
        FVector m_tipOffset = FVector::ZeroVector;

        //現在の攻撃判定のダメージ倍率。
        float m_hitScale = 1.0f;

        //攻撃判定の区間内か。
        bool m_bHitWindow = false;

        //前回の剣の位置があるか。
        bool m_hasPreviousHit = false;

        //前回の剣先の位置。
        FVector m_previousTip = FVector::ZeroVector;

        //前回の剣の根元の位置。
        FVector m_previousBase = FVector::ZeroVector;

        //今回の攻撃で既に当たった相手。
        TSet<TWeakObjectPtr<AActor>> m_hitActors;
    };
    //右手と左手の命中記録。
    FHitWindow m_hands[2];

    //この手が通った場所を調べる。
    void TraceHand(bool left, FHitWindow& hand);

    //命中した相手を打ち上げる。
    void BeginLift(class ACharacter* _target, const struct FEnemyActionData& _action);

private:
    //多段技では最初にダメージを与えた一撃だけ揺らす。
    int32 m_shakeAction = INDEX_NONE;

    //打ち上げ中の相手。
    TWeakObjectPtr<class ACharacter> m_liftTarget;

    //打ち上げ前の相手の重力。
    float m_targetGravity = 1.f;

    //打ち上げ前の自分の重力。
    float m_selfGravity = 1.f;

    //空中コンボ中の重力倍率。
    float m_liftGravity = 1.f;

    //空中コンボ用の重力を終える時刻。
    double m_liftEnd = 0.0;

    //武器のソケットで判定するか。
    bool m_useWeapon = false;

    //回避を確認している相手。
    TWeakObjectPtr<AActor> m_dodgeTarget;

    //前回確認した時に回避していたか。
    bool m_wasDodging = false;

    //相手が回避を始めた時刻。
    double m_dodgeStart = -100.;

    //相手が回避を始めた位置。
    FVector m_dodgePos = FVector::ZeroVector;

    //前回の相手の位置。
    FVector m_lastTargetPos = FVector::ZeroVector;

    //相手の当たり判定の半径。
    float m_dodgeRadius = 0.f;

    //相手の当たり判定の半分の高さ。
    float m_dodgeHalf = 0.f;

private:
    //攻撃判定に使う武器のモデル。
    UPROPERTY(Transient)
    TObjectPtr<UMeshComponent> m_weapon;
};
