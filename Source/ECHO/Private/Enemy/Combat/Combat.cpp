#include "Enemy/Combat/Combat.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Enemy/Animation/EnemySwing.h"
#include "Enemy/Combat/EnemyHitEffect.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraShakeBase.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Player/CombatComponent.h"
#include "Enemy/AI/EnemyTargeting.h"

//判定に使う幅や待ち時間。
namespace CombatTuning
{
	//打ち上げ状態を続ける時間の上限。
	constexpr auto maxLiftTime = 3.f;
	//剣の軌道を調べる分割数の上限。
	constexpr auto maxSweepSteps = 24;
	//確認用の表示サイズ。
	constexpr auto debugSize = 5.0f;
	//打ち上げ判定に使う最小値。
	constexpr auto minLiftValue = .1f;
}

//初期設定と必要な部品を用意する。
UCombat::UCombat()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

//BPで指定した剣の判定を接続する。
void UCombat::BeginPlay()
{
	Super::BeginPlay();
	BindWeapon();

	//BPで左の刀に付けたタグから、判定に使う部品を見つける。
	if (!m_leftWeaponTag.IsNone())
	{
		m_leftWeapon = GetOwner()->FindComponentByTag<UMeshComponent>(m_leftWeaponTag);
	}
}

//攻撃前にも剣の接続を確かめる。
void UCombat::BindWeapon()
{
	if (!GetOwner() || m_weaponTag.IsNone()) { return; }

	for (auto* part : GetOwner()->GetComponentsByTag(UMeshComponent::StaticClass(), m_weaponTag))
	{
		if (SetWeaponMesh(Cast<UMeshComponent>(part), m_baseSocket, m_tipSocket)) { return; }
	}
}

//攻撃判定に使う武器を設定する。
bool UCombat::SetWeaponMesh(UMeshComponent* _mesh, FName _base, FName _tip)
{
	if (_mesh != m_weapon || _base != m_baseSocket || _tip != m_tipSocket) { EndHitWindow(); }
	m_weapon = nullptr;
	m_useWeapon = true;
	if (!IsValid(_mesh) || _mesh->GetOwner() != GetOwner() || !_mesh->DoesSocketExist(_base) || !_mesh->DoesSocketExist(_tip))
	{
		return false;
	}

	//手のアニメーションが更新された後に、刃の位置を読む。
	if (auto* parent = _mesh->GetAttachParent()) { AddTickPrerequisiteComponent(parent); }
	m_weapon = _mesh;
	m_baseSocket = _base;
	m_tipSocket = _tip;
	return true;
}

//命中した相手を打ち上げる。
void UCombat::BeginLift(ACharacter* _target, const FEnemyActionData& _action)
{
	//処理の対象となる敵。
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->CanLiftTarget(_target)) { return; }
	EndLift();
	m_liftTarget = _target;

	//元の重力を記録してから、空中コンボ用に変更する。
	m_targetGravity = _target->GetCharacterMovement()->GravityScale;
	m_selfGravity = enemy->GetCharacterMovement()->GravityScale;
	m_liftGravity = FMath::Max(CombatTuning::minLiftValue, _action.m_liftGravity);
	m_liftEnd = GetWorld()->GetTimeSeconds() + FMath::Clamp(_action.m_liftTime, CombatTuning::minLiftValue, CombatTuning::maxLiftTime);
	_target->GetCharacterMovement()->GravityScale = m_liftGravity;
	enemy->GetCharacterMovement()->GravityScale = m_liftGravity;

	//相手と自分を打ち上げて空中で攻撃をつなぐ。
	_target->LaunchCharacter(FVector(0, 0, _action.m_liftSpeed), true, true);
	enemy->LaunchCharacter(FVector(0, 0, _action.m_liftSpeed), true, true);
}

//打ち上げ後の重力を元に戻す。
void UCombat::EndLift()
{
	if (m_liftEnd <= 0.) { return; }
	if (auto* target = m_liftTarget.Get())
	{
		if (FMath::IsNearlyEqual(target->GetCharacterMovement()->GravityScale, m_liftGravity))
		{
			target->GetCharacterMovement()->GravityScale = m_targetGravity;
		}
	}
	if (auto* enemy = Cast<ACharacter>(GetOwner()))
	{
		if (FMath::IsNearlyEqual(enemy->GetCharacterMovement()->GravityScale, m_liftGravity))
		{
			enemy->GetCharacterMovement()->GravityScale = m_selfGravity;
		}
	}
	m_liftTarget.Reset();
	m_liftEnd = 0.;
}

//終了時にタイマーや登録を解除する。
void UCombat::EndPlay(const EEndPlayReason::Type _reason)
{
	EndLift();
	Super::EndPlay(_reason);
}

//剣の攻撃判定を有効にする。
void UCombat::BeginHitWindow(float _damageScale, FName _bone, FVector _tip, bool _leftBlade)
{
	BindWeapon();

	//左右の武器を、一振りごとの通知で切り替える。
	auto& hand = m_hands[_leftBlade];
	if (_leftBlade) { UpdateLeftBlade(true); }
	hand.m_hitBone = _bone;
	hand.m_tipOffset = _tip;
	hand.m_hitScale = FMath::Max(0.0f, _damageScale);

	//前の命中記録を消して、次の判定に備える。
	hand.m_hitActors.Empty();
	hand.m_hasPreviousHit = false;
	hand.m_bHitWindow = true;
	if (auto* swing = GetOwner()->FindComponentByClass<UEnemySwing>())
	{
		swing->StartHand(_leftBlade);
	}
}

//剣の攻撃判定を終了する。
void UCombat::EndHitWindow()
{
	EndHand(false);
	EndHand(true);
}

//同時に振っている反対側の刀の判定を残す。
void UCombat::EndHand(bool left)
{
	auto& hand = m_hands[left];
	hand.m_bHitWindow = false;
	if (GetOwner())
	{
		if (auto* swing = GetOwner()->FindComponentByClass<UEnemySwing>())
		{
			swing->EndHand(left);
		}
	}
	hand.m_hasPreviousHit = false;
}

//左手に刀が出ているかを返す。
bool UCombat::HasTwoSwords() const
{
	return IsValid(m_leftWeapon) && m_leftWeapon->IsVisible();
}

//今の持ち方と次の技が違う時だけ持ち替える。
UAnimMontage* UCombat::GetBladeChange(bool _dual) const
{
	const auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	const bool drawn = HasTwoSwords();
	if (!enemy || !enemy->m_definition || drawn == _dual) { return nullptr; }
	return _dual ? enemy->m_definition->m_drawLeft.Get() : enemy->m_definition->m_sheatheLeft.Get();
}

//抜刀・納刀のタイミングで、BPに置いた左の刀を表示する。
void UCombat::UpdateLeftBlade(bool _show)
{
	//抜刀する時に、BPで作られた現在の左刀を取り直す。
	if (_show && !m_leftWeaponTag.IsNone())
	{
		m_leftWeapon = GetOwner()->FindComponentByTag<UMeshComponent>(m_leftWeaponTag);
	}

	if (IsValid(m_leftWeapon)) { m_leftWeapon->SetVisibility(_show); }
}

//攻撃と防御の判定を初期化する。
void UCombat::ResetWindows()
{
	EndHitWindow();

	//前の命中記録を消して、次の判定に備える。
	m_bInvulnerable = false;
	m_bGuarding = false;
}

//攻撃判定と演出で同じ剣の位置を読む。
bool UCombat::GetBladePoints(bool _left, FVector& _base, FVector& _tip) const
{
	const UMeshComponent* blade = _left ? m_leftWeapon.Get() : m_weapon.Get();
	if (!IsValid(blade) || !blade->IsVisible() || !blade->DoesSocketExist(m_baseSocket) || !blade->DoesSocketExist(m_tipSocket))
	{
		return false;
	}
	_base = blade->GetSocketLocation(m_baseSocket);
	_tip = blade->GetSocketLocation(m_tipSocket);
	return true;
}

//この手の通知で指定された剣やボーンを読む。
bool UCombat::ReadPoints(bool left, FVector& _base, FVector& _tip) const
{
	const auto& hand = m_hands[left];
	const auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	const auto* action = enemy ? enemy->GetActiveAction() : nullptr;
	if (!enemy || !action) { return false; }

	//剣を使う時の初期値。素手や左手の攻撃なら、対応する部品に切り替える。
	FName base = m_baseSocket;
	FName tip = m_tipSocket;
	UMeshComponent* mesh = enemy->GetMesh();
	const bool singleBone = !hand.m_hitBone.IsNone();
	const bool bonePair = !action->m_hitBase.IsNone() && !action->m_hitTip.IsNone();
	if (singleBone)
	{
		base = tip = hand.m_hitBone;
	}
	else if (bonePair)
	{
		base = action->m_hitBase;
		tip = action->m_hitTip;
	}
	else if (m_useWeapon) { mesh = m_weapon.Get(); }
	if (left) { mesh = m_leftWeapon.Get(); }
	if (!IsValid(mesh) || mesh->GetOwner() != enemy) { return false; }
	if (!mesh->DoesSocketExist(base) || !mesh->DoesSocketExist(tip)) { return false; }

	//剣はソケットをそのまま使い、手足の攻撃はBPのずらし幅を足す。
	_base = mesh->GetSocketLocation(base);
	_tip = mesh->GetSocketLocation(tip);
	if (singleBone)
	{
		_tip = mesh->GetSocketTransform(tip).TransformPosition(hand.m_tipOffset);
	}
	else if (bonePair)
	{
		_base = mesh->GetSocketTransform(base).TransformPosition(action->m_hitBaseOffset);
		_tip = mesh->GetSocketTransform(tip).TransformPosition(action->m_hitTipOffset);
	}
	return true;
}

//爆発の直前に回避し、回避前の体が爆風に入っていたか調べる。
bool UCombat::DodgedBlast(AActor* target, FVector point, float radius, float window) const
{
	if (window <= 0.f || !m_wasDodging || m_dodgeTarget.Get() != target) { return false; }
	if (GetWorld()->GetTimeSeconds() - m_dodgeStart > window) { return false; }
	const FVector height(0.f, 0.f, FMath::Max(0.f, m_dodgeHalf - m_dodgeRadius));
	const FVector body = FMath::ClosestPointOnSegment(point, m_dodgePos - height, m_dodgePos + height);
	if (FVector::DistSquared(point, body) > FMath::Square(radius + m_dodgeRadius)) { return false; }
	FCollisionQueryParams query(SCENE_QUERY_STAT(BombDodge), false, GetOwner());
	query.AddIgnoredActor(target);
	return !GetWorld()->LineTraceTestByChannel(point, body, ECC_Visibility, query);
}

//回避が始まった場所に、本当に刃が届いたか調べる。
void UCombat::CheckPerfectDodge(AEnemyCharacterBase& _enemy, const FVector& _start, const FVector& _end)
{
	//受付時間外と、同じ攻撃で成功済みの場合は対象外。
	const float elapsed = GetWorld()->GetTimeSeconds() - m_dodgeStart;
	if (m_dodgeRadius <= 0.f || _enemy.WasPerfectDodged()) { return; }
	if (!_enemy.IsDefenseInputValid(EDefenseResult::PerfectDodge, elapsed)) { return; }

	//回避前の体の中心線を、足側から頭側まで求める。
	const FVector height(0, 0, FMath::Max(0.f, m_dodgeHalf - m_dodgeRadius));
	const FVector bottom = m_dodgePos - height;
	const FVector top = m_dodgePos + height;

	//刃と体が一番近づいた二点を求める。
	FVector bladePoint, bodyPoint;
	FMath::SegmentDistToSegmentSafe(_start, _end, bottom, top, bladePoint, bodyPoint);

	//刃と体の太さを合わせても届かなければ、普通の回避になる。
	const float reach = m_radius + m_dodgeRadius;
	if (FVector::DistSquared(bladePoint, bodyPoint) > FMath::Square(reach)) { return; }

	//刃と相手の間に壁があるときは成功にしない。
	AActor* target = _enemy.m_combatTarget.Get();
	FCollisionQueryParams params(SCENE_QUERY_STAT(BossSword), false, &_enemy);
	FHitResult wall;
	const bool hit = GetWorld()->LineTraceSingleByChannel(wall, _start, bodyPoint, ECC_Visibility, params);
	if (hit && wall.GetActor() != target) { return; }

	//成功を知らせる。演出は受け取った相手が決める。
	_enemy.ApplyPerfectDodge(target);
}

//剣が動いた範囲を毎フレーム調べる。
void UCombat::TickComponent(float _deltaTime, ELevelTick _type, FActorComponentTickFunction* _function)
{
	Super::TickComponent(_deltaTime, _type, _function);

	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = GetOwner() ? Cast<AEnemyCharacterBase>(GetOwner()) : nullptr;
	if (m_liftEnd > 0. && (!enemy || !enemy->IsCombatActive() || !m_liftTarget.IsValid() || GetWorld()->GetTimeSeconds() >= m_liftEnd))
	{
		EndLift();
	}

	//使用する行動の設定。
	const FEnemyActionData* action = enemy ? enemy->GetActiveAction() : nullptr;

	//戦闘が止まったら、次の抜刀に備えて片付ける。
	if (!enemy || !enemy->IsCombatActive()) { UpdateLeftBlade(false); }

	//狙っている相手。
	AActor* target = enemy ? enemy->m_combatTarget.Get() : nullptr;

	//相手の攻撃や回避の状態を確認する部品。
	const auto* playerCombat = IsValid(target) ? target->FindComponentByClass<UCombatComponent>() : nullptr;

	//相手が回避中か。
	const bool dodging = playerCombat && playerCombat->IsDodging();

	//狙う相手が変わったら、回避の記録を切り替える。
	if (m_dodgeTarget.Get() != target)
	{
		m_dodgeTarget = target;
		m_wasDodging = false;
		m_dodgeStart = -100.;
		m_lastTargetPos = IsValid(target) ? target->GetActorLocation() : FVector::ZeroVector;
	}
	if (dodging && !m_wasDodging)
	{
		m_dodgeStart = GetWorld()->GetTimeSeconds();
		m_dodgePos = m_lastTargetPos;

		//キャラクターの当たり判定。
		const auto* capsule = target->FindComponentByClass<UCapsuleComponent>();
		m_dodgeRadius = capsule ? capsule->GetScaledCapsuleRadius() : 0.f;
		m_dodgeHalf = capsule ? capsule->GetScaledCapsuleHalfHeight() : 0.f;
	}
	m_wasDodging = dodging;
	if (IsValid(target)) { m_lastTargetPos = target->GetActorLocation(); }

	if (!enemy || !enemy->HasAuthority() || !IsHitWindowOpen() || !action || !enemy->IsCombatActive())
	{
		return;
	}

	//両手の通知が重なった時も、それぞれの刃を調べる。
	for (bool left : { false, true })
	{
		if (m_hands[left].m_bHitWindow) { TraceHand(left, m_hands[left]); }
	}
}

//この刃が通った場所だけにダメージを与える。
void UCombat::TraceHand(bool left, FHitWindow& hand)
{
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	const auto* action = enemy ? enemy->GetActiveAction() : nullptr;
	if (!action || !enemy->IsActing()) { return; }
	auto* target = enemy->m_combatTarget.Get();

	//演出と同じ位置で判定する。
	FVector start, end;
	if (!ReadPoints(left, start, end))
	{
		hand.m_hasPreviousHit = false;
		return;
	}

	//判定の対象とする種類。
	FCollisionObjectQueryParams objects;

	//分身はPawnと別の種類なので、巻き込み時は動く物も調べる。
	if (m_hitNearby)
	{
		objects = FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects);
	}
	objects.AddObjectTypesToQuery(ECC_Pawn);

	//ターゲットのオブジェクトタイプも対象にする（Player プリセット対応）
	if (const UPrimitiveComponent* targetRoot = IsValid(target) ? Cast<UPrimitiveComponent>(target->GetRootComponent()) : nullptr)
	{
		objects.AddObjectTypesToQuery(targetRoot->GetCollisionObjectType());
	}

	//生成や当たり判定に使う設定。
	FCollisionQueryParams params(SCENE_QUERY_STAT(BossSword), false, enemy);

	//ダメージを与える前に、回避の成功を判定する。
	const bool alreadyDodged = enemy->WasPerfectDodged();
	CheckPerfectDodge(*enemy, start, end);

	//命中した場所の一覧。
	TArray<FHitResult> hits;

	//剣の太さを表す球を、根元から剣先まで動かす。
	const auto bladeShape = FCollisionShape::MakeSphere(m_radius);
	GetWorld()->SweepMultiByObjectType(hits, start, end, FQuat::Identity, objects, bladeShape, params);
	if (hand.m_hasPreviousHit)
	{

		//軌道を調べる分割数。
		const float spacing = FMath::Max(2.f, m_radius * 2.f);
		const int32 steps = FMath::CeilToInt(FVector::Dist(start, end) / spacing);
		const int32 samples = FMath::Clamp(steps, 1, CombatTuning::maxSweepSteps);
		for (int32 i = 0; i <= samples; ++i)
		{

			//二つの値の間の割合。
			const float alpha = float(i) / samples;

			//移動した剣の軌道の命中結果。
			TArray<FHitResult> motionHits;

			//刃の同じ場所を、前フレームの位置から今の位置まで調べる。
			const FVector from = FMath::Lerp(hand.m_previousBase, hand.m_previousTip, alpha);
			const FVector to = FMath::Lerp(start, end, alpha);
			GetWorld()->SweepMultiByObjectType(motionHits, from, to, FQuat::Identity, objects, bladeShape, params);
			CheckPerfectDodge(*enemy, from, to);
			hits.Append(motionHits);
		}
	}

	//回避した相手は今回の一振りだけ命中済みにする。
	if (!alreadyDodged && enemy->WasPerfectDodged() && target) { hand.m_hitActors.Add(target); }

	hand.m_previousTip = end;
	hand.m_previousBase = start;
	hand.m_hasPreviousHit = true;

	//今回与えるダメージ。
	const float hitDamage = action->m_damage * hand.m_hitScale;

	//強攻撃として扱うか。
	const bool heavy = action->m_kind == EEnemyAction::Heavy;

	//ダメージの種類。
	TSubclassOf<UDamageType> damageType = heavy ? UEBHeavyDamageType::StaticClass() : UEBCombatDamageType::StaticClass();
	if (action->m_unblockable)
	{
		damageType = heavy ? UEBHeavyUnblockableDamageType::StaticClass() : UEBUnblockableDamageType::StaticClass();
	}

	for (const FHitResult& hit : hits)
	{
		//攻撃が当たった相手。
		AActor* hitActor = hit.GetActor();
		if (!IsValid(hitActor) || hand.m_hitActors.Contains(hitActor)) { continue; }

		//敵同士は傷つけず、狙い以外への命中はBPの設定で許可する。
		if (hitActor != target && (!m_hitNearby || !enemy->m_targeting->IsCandidate(hitActor)))
		{
			continue;
		}

		//障害物との接触結果。
		FHitResult wallHit;
		if (GetWorld()->LineTraceSingleByChannel(wallHit, start, hit.ImpactPoint, ECC_Visibility,
			params) &&
			wallHit.GetActor() != hitActor)
		{
			continue;
		}

		hand.m_hitActors.Add(hitActor);

		//別の相手に当たった場合は、その相手自身の回避を確認する。
		const auto* hitCombat = hitActor->FindComponentByClass<UCombatComponent>();
		if (hitCombat && hitCombat->IsDodging()) { continue; }

		//防御の結果。
		EDefenseResult response = enemy->ResolveTargetDefense(hitActor);

		//BPから届く結果にも、この攻撃の回避ルールを適用する。
		if (response == EDefenseResult::PerfectDodge && action->m_unblockable &&
			enemy->m_definition && enemy->m_definition->m_dodgeOnly)
		{
			response = EDefenseResult::None;
		}

		if (action->m_special && action->m_unblockable && response == EDefenseResult::Guard)
		{
			enemy->TargetGuardBroken(hitActor);
		}

		//剣で防げたか、ジャスト回避で避けたかを分けて確認する。
		const bool usedSword = response == EDefenseResult::Guard || response == EDefenseResult::Parry || response == EDefenseResult::Deflect;
		const bool blockedBySword = usedSword && !action->m_unblockable;
		if (response == EDefenseResult::PerfectDodge || blockedBySword)
		{

			//回避には火花を出さず、剣で防がれた時だけ出す。
			if (response != EDefenseResult::PerfectDodge)
			{
				enemy->m_hitFx->PlayClash(hitActor);
			}
			enemy->AttackDefended_Implementation(hitActor, response);

			//攻撃自体が止まった時だけ終了し、他の命中相手も調べる。
			if (!enemy->IsActing() || !hand.m_bHitWindow) { break; }
			continue;
		}

		//実際に与えたダメージ。
		const float actualDamage = UGameplayStatics::ApplyDamage(hitActor, hitDamage, enemy->GetController(), enemy, damageType);

		//確認中だけ、命中した技と相手までの距離を表示する。
		if (m_bDebug)
		{
			const float distance = FVector::Dist2D(enemy->GetActorLocation(), hitActor->GetActorLocation());
		}

		if (actualDamage > 0.0f)
		{
			//大技を実際に受けたプレイヤーの画面だけに短い衝撃を出す。
			if (m_hitShake && m_shakeScale > 0.f && (heavy || action->m_unblockable || action->m_special) && m_shakeAction != enemy->m_actionSerial)
			{
				APawn* pawn = Cast<APawn>(hitActor);
				auto* player = pawn ? Cast<APlayerController>(pawn->GetController()) : nullptr;
				if (player)
				{
					m_shakeAction = enemy->m_actionSerial;
					player->ClientStartCameraShake(m_hitShake, m_shakeScale);
				}
			}

			//狙った相手に当たった時だけ、次の技と命中記録につなぐ。
			if (hitActor == target) { enemy->RecordAttackHit(); }
			if (enemy->m_hitFx)
			{
				enemy->m_hitFx->ShowStrike(hit.ImpactPoint, hit.ImpactNormal.GetSafeNormal());
			}
			if (hitActor == target && action->m_launch && enemy->IsActing() &&
				!enemy->WasAttackBlocked())
			{
				BeginLift(Cast<ACharacter>(hitActor), *action);
			}

			//空中で当たった時は、相手と上下の速さを合わせる。
			if (action->m_airFollow && action->m_airOnly && m_liftEnd > 0. && hitActor == m_liftTarget.Get())
			{
				enemy->GetCharacterMovement()->Velocity.Z = hitActor->GetVelocity().Z;
			}
		}

		if (!enemy->IsActing() || !hand.m_bHitWindow) { break; }
	}
}
