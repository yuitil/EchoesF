#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyAfterimage.generated.h"

class UPoseableMeshComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

//その瞬間の敵の姿を残し、短い時間で消す。
UCLASS(Blueprintable)
class ECHO_API AEnemyAfterimage : public AActor
{
    GENERATED_BODY()

public:
    //当たり判定のない表示用モデルを用意する。
    AEnemyAfterimage();

    //元の敵の姿勢と位置を写す。
    UFUNCTION(BlueprintCallable, Category="Enemy|Afterimage")
    void Capture(USkeletalMeshComponent* _source);

    //時間に合わせて薄くする。
    virtual void Tick(float _delta) override;

public:
    //写した姿勢を表示するモデル。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Afterimage")
    TObjectPtr<UPoseableMeshComponent> m_pose;

    //BPから選ぶ紫黒の残像マテリアル。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage")
    TObjectPtr<UMaterialInterface> m_material;

    //残像を残す秒数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage", meta=(ClampMin="0.05", ClampMax="1"))
    float m_life = .3f;

    //薄くなり始めるまで姿を保つ秒数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage", meta=(ClampMin="0", ClampMax="0.5"))
    float m_hold = .1f;

    //最初に表示する濃さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage", meta=(ClampMin="0", ClampMax="1"))
    float m_opacity = .4f;

    //薄さを変えるマテリアルの項目名。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Afterimage")
    FName m_fadeName = TEXT("GhostOpacity");

private:
    //残像を出してからの秒数。
    float m_age = 0.f;

    //この残像だけの薄さを変える材質。
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> m_fade;
};
