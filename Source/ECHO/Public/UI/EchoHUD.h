//EchoHUD.h
//
//ロックオンターゲットのマーカーを描画と、ステータスUIの表示を行うHUD

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EchoHUD.generated.h"

class UEchoStatusWidget;

UCLASS()
class ECHO_API AEchoHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	virtual void BeginPlay() override;

	//表示するステータスUI（BPで WBP_EchoStatus を設定）
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UEchoStatusWidget> StatusWidgetClass;

private:
	UPROPERTY()
	UEchoStatusWidget* StatusWidget;

	//マーカーの基本半径
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float m_MarkerBaseRadius = 15.f;

	//距離のスケールの基準距離
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float m_MarkerReferenceDistance = 600.f;

	//マーカーの最少・最大半径
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float m_MarkerMinRadius = 5.f;

	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float m_MarkerMaxRadius = 15.f;

	//線の太さ
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float m_MarkerThickness = 2.f;

	//マーカの色
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	FLinearColor MarkerColor = FLinearColor::White;

	//円を表示するセグメント数
	int32 m_iMarkerSegments = 16;

	//円をDrawLine()で分割描画する
	void DrawCircle(float CenterX, float CenterY, float Radius);
};
