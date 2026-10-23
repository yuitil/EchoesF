#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "EnemyThrowBomb.generated.h"
class AEnemyBomb;
//
UCLASS(meta=(DisplayName="EnemyThrowBomb"))
class ECHO_API UEnemyThrowBomb : public UAnimNotify
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") TSubclassOf<AEnemyBomb> m_bomb;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") FName m_socket = TEXT("head");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") FVector m_offset = FVector(100,0,0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb",meta=(ClampMin="1",ClampMax="3")) int32 m_count=1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") float m_spread=130.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb",meta=(ClampMin="0.5")) float m_flight=1.3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") float m_lead=.2f;
    virtual void Notify(USkeletalMeshComponent* mesh,UAnimSequenceBase* anim,const FAnimNotifyEventReference& reference) override;
};
//
