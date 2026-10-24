#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyAfterimageTrail.generated.h"

class AEnemyAfterimage;

//通知の区間で、移動した敵の姿を一定間隔で残す。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UEnemyAfterimageTrail : public UActorComponent
{
    GENERATED_BODY()

public:
    //モデル更新後に残像を作るようにする。
    UEnemyAfterimageTrail();

    //通知から残像を開始する。
    void Start(bool _allowStill, float _duration);

    //通知から残像を終了する。
    void Stop();

protected:
    //表示した残像を終了時に片付ける。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;

    //移動距離と間隔を調べて残像を出す。
    virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;

public:
    //使用する残像BP。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage")
    TSubclassOf<AEnemyAfterimage> m_imageClass;

    //見た目に使用するメッシュ部品のタグ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage")
    FName m_meshTag;

    //次の残像までの秒数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage", meta=(ClampMin="0.03"))
    float m_interval = .08f;

    //残像を出すのに必要な移動距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage", meta=(ClampMin="1"))
    float m_distance = 12.f;

    //同時に残す数の上限。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage", meta=(ClampMin="1", ClampMax="12"))
    int32 m_limit = 5;

private:
    //通知が終わるまでの残り時間。
    float m_left = 0.f;

    //その場の攻撃でも姿を残すか。
    bool m_allowStill = false;

    //残像を出す区間内か。
    bool m_active = false;

    //前回の表示からの秒数。
    float m_wait = 0.f;

    //前回の残像の位置。
    FVector m_lastPos = FVector::ZeroVector;

    //今表示している残像。
    TArray<TWeakObjectPtr<AEnemyAfterimage>> m_images;
};
