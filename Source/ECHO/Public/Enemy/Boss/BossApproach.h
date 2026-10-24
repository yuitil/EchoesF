#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossApproach.generated.h"

class UAnimMontage;

//近づいた相手への抜刀と、追いかける速さを管理する。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UBossApproach : public UActorComponent
{
    GENERATED_BODY()

public:
    //近づいた相手への抜刀と、追いかける速さを管理する。
    UBossApproach();
    //次の戦闘に向けて状態を初期化する。
    void Reset();
    //抜刀中か返す。
    bool IsDrawing() const { return m_drawing; }
    //抜刀を終えているか返す。
    bool HasDrawn() const { return m_drawn && !m_drawing; }

public:
    //抜刀を通知して刀を表示する。
    UFUNCTION(BlueprintCallable, Category="Approach|Draw")
    void ShowSword();

protected:
    //ゲーム開始時に必要な状態を準備する。
    virtual void BeginPlay() override;
    //他の端末にも伝える値を登録する。
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& _props) const override;
    //経過時間に合わせて敵の状態を更新する。
    virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;

public:
    //歩きながら刀を抜くモンタージュ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw")
    TObjectPtr<UAnimMontage> m_walkDraw;

    //走りながら刀を抜くモンタージュ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw")
    TObjectPtr<UAnimMontage> m_runDraw;

    //表示を切り替える刀の目印。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw")
    FName m_swordTag = TEXT("BossSword");

    //歩き抜刀中の移動速度。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw", meta=(ClampMin="0"))
    float m_walkDrawSpeed = 150.f;

    //走り抜刀中の移動速度。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw", meta=(ClampMin="0"))
    float m_runDrawSpeed = 350.f;

    //相手が待機中と判断する速度。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw", meta=(ClampMin="0"))
    float m_waitSpeed = 30.f;

    //抜刀中に経路を更新する間隔。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw", meta=(ClampMin="0.1"))
    float m_pathDelay = .3f;

    //その場で刀を抜くモンタージュ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw")
    TObjectPtr<UAnimMontage> m_draw;

    //刀を抜き始める距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw", meta=(ClampMin="0"))
    float m_drawRange = 900.f;

    //抜刀の再生速度。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Draw", meta=(ClampMin="0.1"))
    float m_drawRate = 1.f;

    //通常の走りの速度。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Run", meta=(ClampMin="1"))
    float m_runSpeed = 350.f;

    //全力で走る時の速度。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Run", meta=(ClampMin="1"))
    float m_sprintSpeed = 600.f;

    //全力で走り始める距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Run", meta=(ClampMin="0"))
    float m_sprintRange = 700.f;

    //走りから全力走りへ切り替える時間。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Run", meta=(ClampMin="0"))
    float m_sprintDelay = 0.6f;

    //移動速度を変える速さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Approach|Run", meta=(ClampMin="1"))
    float m_speedChange = 400.f;

private:
    //抜刀の状態に合わせて刀の表示を切り替える。
    UFUNCTION()
    void UpdateSword();

private:
    //刀を表示しているか。
    UPROPERTY(ReplicatedUsing=UpdateSword)
    bool m_swordOut = false;

    //再生中の抜刀モンタージュ。
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> m_drawClip;

private:
    //抜刀を終えたか。
    bool m_drawn = false;
    //抜刀を再生中か。
    bool m_drawing = false;
    //追跡して走り続けた時間。
    float m_runTime = 0.f;
    //次に抜刀中の経路を更新する時刻。
    float m_pathTime = 0.f;
    //選んだ抜刀に合わせた移動速度。
    float m_drawSpeed = 0.f;
};
