// 担当：佐々木奏太
// イベント情報を管理する構造体

#pragma once

#include "CoreMinimal.h"
#include "EventData.generated.h"

class AEventPoint;
class AGate;


USTRUCT(BlueprintType)
struct ECHO_API FEventData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<AEventPoint>> m_EventPoints;
	
	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<AGate>> m_Gates;
};