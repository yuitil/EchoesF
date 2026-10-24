#include "Enemy/UI/EnemyHealthWidget.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Boss/BossEnemyCharacter.h"
#include "Enemy/Combat/Defense.h"
#include "Enemy/Boss/BossPosture.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

//表示する敵を切り替え、残り時間をリセットする。
void UEnemyHealthWidget::SetEnemy(AEnemyCharacterBase* _enemy)
{
    m_enemy = _enemy;
    m_phaseGlow = Cast<UImage>(GetWidgetFromName(m_phaseGlowName));
    if(m_phaseGlow) { m_phaseGlow->SetVisibility(ESlateVisibility::Hidden); }
    m_wasRising = false;
    m_trail = 1.f;
    m_shown = 1.f;
    m_wasPhaseTwo = false;
    m_lastHealth = _enemy ? _enemy->GetHealth() : 0.f;
    m_showLeft = 0.f;
    m_wasBroken = false;
    SetRenderOpacity(m_hitOnly ? 0.f : 1.f);
}

//敵の体力を読み取り、BPで作った表示を動かす。
void UEnemyHealthWidget::NativeTick(const FGeometry& _shape, float _delta)
{
    //敵の体力を読み取り、BPで作った表示を動かす。
    Super::NativeTick(_shape, _delta);
    //現在表示している敵。
    auto* enemy = m_enemy.Get();
    if(!enemy) { SetRenderOpacity(0.f); return; }

    //今の体力とガード状態を確認する。
    const auto* posture = m_postureMode ? enemy->FindComponentByClass<UBossPosture>() : nullptr;
    const float health = posture ? posture->m_value : enemy->GetHealth();
    //体力の上限。
    const float maxHealth = posture ? posture->m_max : enemy->GetMaxHealth();
    //バーに使う0から1の体力割合。
    const float ratio = maxHealth > 0.f ? FMath::Clamp(health / maxHealth, 0.f, 1.f) : 0.f;
    //ガードが崩れているか。
    const bool broken = enemy->m_defense && enemy->m_defense->m_bGuardBroken;
    //体力が減った時だけダメージ演出を始める。
    if(health < m_lastHealth)
    {
        m_showLeft = m_showTime;
        m_trailLeft = m_trailDelay;
        if(m_hitAnim) { StopAnimation(m_hitAnim); PlayAnimation(m_hitAnim); }
    }
    //ガード崩れの開始と終了を表示へ伝える。
    if(broken && !m_wasBroken && m_breakAnim) { PlayAnimation(m_breakAnim, 0.f, 0); }
    if(!broken && m_wasBroken && m_breakAnim) { StopAnimation(m_breakAnim); }
    m_wasBroken = broken;
    m_lastHealth = health;
    //表示時間が過ぎたら、少しずつ隠す。
    m_showLeft = FMath::Max(0.f, m_showLeft - _delta);
    m_trailLeft = FMath::Max(0.f, m_trailLeft - _delta);
    //今回のフレームで表示を続けるか。
    const bool show = enemy->m_state != EEnemyState::Dead && (!m_hitOnly || m_showLeft > 0.f || broken);
    SetRenderOpacity(FMath::FInterpConstantTo(GetRenderOpacity(), show ? 1.f : 0.f, _delta, m_fadeSpeed));
    //減った体力の残り色を遅れて追いつかせる。
    if(ratio > m_trail) { m_trail = ratio; }
    else if(m_trailLeft <= 0.f) { m_trail = FMath::FInterpConstantTo(m_trail, ratio, _delta, m_trailSpeed); }
    //立ち上がりの開始では空の表示から回復を見せる。
    const auto* boss = Cast<ABossEnemyCharacter>(enemy);
    const bool rising = boss && boss->m_bossPhase == EBossPhase::Transition;
    if(rising && !m_wasRising && !m_postureMode) { m_shown = 0.f; }
    m_wasRising = rising;
    //回復は少しずつ満たし、被弾はすぐ反映する。
    m_shown = ratio < m_shown ? ratio : FMath::FInterpConstantTo(m_shown, ratio, _delta, m_fillSpeed);
    if(m_healthBar) { m_healthBar->SetPercent(m_shown); }
    if(m_trailBar) { m_trailBar->SetPercent(FMath::Min(m_trail, ratio > m_shown ? m_shown : m_trail)); }
    //フェーズ2のオーラはWidget BPのアニメーションで指定する。
    const bool phaseTwo = boss && boss->m_bossPhase == EBossPhase::PhaseTwo;
    if(m_phaseAura && phaseTwo != m_wasPhaseTwo)
    {
        if(phaseTwo) { PlayAnimation(m_phaseAura, 0.f, 0); }
        else { StopAnimation(m_phaseAura); }
    }
    if(m_phaseGlow) { m_phaseGlow->SetVisibility(phaseTwo ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden); }
    m_wasPhaseTwo = phaseTwo;
    //ボスのフェーズ名を切り替える。
    if(m_phaseText)
    {
        if(boss)
        {
            //現在のフェーズに合わせる表示名。
            FText phase = m_phaseOne;
            if(boss->m_bossPhase == EBossPhase::PhaseTwo) { phase = m_phaseTwo; }
            else if(boss->m_bossPhase == EBossPhase::Transition) { phase = m_riseText; }
            m_phaseText->SetText(phase);
        }
    }
    //ガードが崩れた間だけ文字を出す。
    if(m_breakText)
    {
        m_breakText->SetVisibility(broken ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
}
