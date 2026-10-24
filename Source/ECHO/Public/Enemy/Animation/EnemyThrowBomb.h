#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "EnemyThrowBomb.generated.h"
class AEnemyBomb;

//口元から相手の足元へ弾を投げる。
UCLASS(meta=(DisplayName="EnemyThrowBomb"))
class ECHO_API UEnemyThrowBomb : public UAnimNotify
{
	GENERATED_BODY()

public:
	//アニメーションの指定位置で処理を呼ぶ。
	virtual void Notify(USkeletalMeshComponent* _mesh,UAnimSequenceBase* _anim,const FAnimNotifyEventReference& _reference) override;

public:
	//生成した呪石弾。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	TSubclassOf<AEnemyBomb> m_bomb;

	//弾を出すソケット名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	FName m_socket = TEXT("head");

	//弾の出現位置のずらし量。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	FVector m_offset = FVector(100,0,0);

	//生成する数。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb",meta=(ClampMin="1",ClampMax="3"))
	int32 m_count=1;

	//弾の着地点を散らす幅。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	float m_spread=130.f;

	//弾が届くまでの目安の時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb",meta=(ClampMin="0.5"))
	float m_flight=1.3f;

	//相手の移動を先読みする時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	float m_lead=.2f;

	//移動や再生に使う速さ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb|Flight", meta=(ClampMin="1"))
	float m_speed = 950.f;

	//弾の飛行時間の下限。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb|Flight", meta=(ClampMin="0.1"))
	float m_minFlight = 0.55f;

	//着地点の地面を下へ探す距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb|Flight", meta=(ClampMin="1"))
	float m_floorDepth = 1500.f;

	//地面を探し始める上方向の距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb|Flight", meta=(ClampMin="0"))
	float m_floorRise = 150.f;

	//先読みに使う相手の速度の上限。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb|Flight", meta=(ClampMin="0"))
	float m_maxLeadSpeed = 500.f;

	//先読みする時間の上限。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb|Flight", meta=(ClampMin="0"))
	float m_maxLead = 0.5f;
};
