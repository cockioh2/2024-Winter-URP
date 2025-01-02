# BlockHammer: Preventing RowHammer at Low Cost by Blacklisting Rapidly-Accessed DRAM Rows

## I. Table of Contents
- [BlockHammer: Preventing RowHammer at Low Cost by Blacklisting Rapidly-Accessed DRAM Rows](#blockhammer-preventing-rowhammer-at-low-cost-by-blacklisting-rapidly-accessed-dram-rows)
  - [I. Table of Contents](#i-table-of-contents)
  - [II. Abstract](#ii-abstract)
  - [III. Introduction](#iii-introduction)
  - [IV. BlockHammer](#iv-blockhammer)
    - [IV-1. RowBlocker](#iv-1-rowblocker)
    - [IV-2. AttackThrottler](#iv-2-attackthrottler)
  - [V. Many-Sided RowHammer Attacks](#v-many-sided-rowhammer-attacks)
  - [VI. Security Analysis](#vi-security-analysis)
  - [VII. Hardware Complexity Analysis](#vii-hardware-complexity-analysis)
  - [VIII.Experimental Methodology](#viiiexperimental-methodology)
  - [IX. Performance and Energy Evaluation](#ix-performance-and-energy-evaluation)
  - [X. Comparison of Mitigation Mechanisms](#x-comparison-of-mitigation-mechanisms)
  - [XI. Conclusion](#xi-conclusion)

## II. Abstract

현대 DRAM 기술의 density가 높아지면서 RowHammer가 문제가 되고 있다. 이 현상은 Attacker가 악용하면 시스템 권한을 상승시키거나 민감 데이터를 유출하는 등의 공격등으로 이어질 수 있다. <br> 또한, 최신 DRAM 기술(DDR4/LPDDR4)은 이전 DRAM 기술보다 훨씬 취약하며, 이를 미루어보았을 때 미래의 DRAM Chip에서도 문제가 될 가능성이 높다. <br>

이를 완화하기 위한 RowHammer mitigation 방법들이 존재하나, 기존 RowHammer mitigation은 다음과 같은 문제가 존재한다.

1. 취약한 DRAM일수록 더 많은 점점 더 높은 성능 그리고(또는) 면적 오버헤드가 발생함.

2. 기존 메커니즘은 DRAM 설계 정보(기밀 데이터)에 의존하거나, DRAM 칩의 물리적 설계를 변경해야 함.

본 논문의 BlockHammer는 low-cost, effective, 그리고 easy-to-adopt한 Row mitigation 메커니즘이며 기존의 RowHammer mitigation이 가지는 문제를 모두 극복하였다.

BlockHammer의 주요 아이디어는 아래와 같다.

- RowBlocker: Area-efficient [Bloom Filter](https://en.wikipedia.org/wiki/Bloom_filter) 기반 Row Activation Rate 추적

- AttackThrottler: 추적한 데이터를 이용하여 RowHammer bit-flip이 일어나지 않도록 row activation 제한

280개 workload 테스트 결과, RowHammer Attack이 없을 때는 기존 기술에 견줄만한 성능과 에너지를 보였으며, 공격이 있으면 상당히 더 좋은 성능과 에너지 효율을 보였다(성능 45% 개선, 에너지 효율 28.9% 향상).

## III. Introduction

DRAM은 공정 기술의 발전으로 인해 density가 증가하면서 chip의 비트당 비용을 줄이는 데에는 도움이 되었지만, 데이터의 신뢰성에는 부정적인 영향을 미쳤다. 현재 DRAM chip은 이전보다 RowHammer에 훨씬 취약하다. RowHammer란, DRAM 행(Agressor row, 공격 행)을 빠른 속도로 열고 닫는(Activating and Precharging) 작업이 물리적으로 인접한 다른 행(victim row, 피해 행)에서 bit-flip을 유발하는 현상이다.

RowHammer는 권한 상승이나 민감한 데이터 유출과 같은 보안 위협이 발생할 수 있음을 여러 연구에서 입증했다. 게다가 최근 연구에서, RowHammer 문제가 이전보다 더 심각해졌으며 향후 DRAM 칩에서도 더 악화될 것으로 예상된다고 보고하고 있다.

현재 DRAM 벤더들은 DRAM 내부에 자체적으로 target row refresh와 같은 RowHammer mitigation mechanism을 구현하고 있다. 그러나 최근 연구들은 상용 DDR3, DDR4, LPDDR4 Chip들이 여전히 RowHammer에 취약하다고 보고하고 있다. 특히 내부에 존재하는 TRR mechanism을 우회하여 RowHammer를 유발할 수 있다는 것을 TRRespass를 통해 보여주었다. 선행 연구에서는 2014년부터 2020년 사이 DRAM 칩의 bit-flip을 유발하기 위해 필요한 활성화 횟수가 139,200회에서 9,600회로 감소했다고 보고하였다.

이를 해결하기 위해 다양한 완화 방법이 제안되었으며, 이 제안들은 네가지 주요 접근 방식으로 분류할 수 있다.

1. Increased refresh rate
    - 모든 행을 더 자주 refresh하여 bit-flip 확률 감소
2. Physical isolation
    - 민감한 데이터를 잠재적인 attacker의 메모리 공간과 물리적으로 분리
3. Reactive refresh
    - row activation을 관찰하여 rapid row activation에 반응해 victim row를 refresh
4. proactive throttling
    - row activation rate 자체를 RowHammer에 안전한 rate로 제한

먼저, DRAM chip이 RowHammer에 더욱 취약해짐에 따라, 완화 메커니즘은 더 공격적으로 작동해야 한다. scalable한 메커니즘은 수용 가능한 성능, 에너지 효율, 칩 면적 오버헤드를 보여주어야 한다. 그러나 RowHammer에 취약해질수록, 위의 4가지 접근 방법은 아래와 같은 문제를 겪는다. 

1. 고정된 설계 지점(fixed design point)에 기반하기 때문에 쉽게 적응하지 못한다.
2. 성능, 에너지, 면적 오버헤드가 점점 더 커지는 문제를 겪는다.

두 번째로, physical isolation이나 reactive refresh의 경우 다음 중 하나를 필요로 한다.

1. potential victim row를 모두 식별할 수 있는 능력
2. DRAM chip 자체를 수정하여 potential victim row를 내부적으로 격리하거나, RowHammer mitigation 메커니즘이 모든 potential victim row를 정확하게 refresh할 수 있도록 만드는 방법

potential victim row를 식별하려면, virtual address를 physical address로 변환하는 내부 매핑 방식을 알아야 한다. 그러나 DRAM 벤더는 이러한 내부 매핑 방식을 기밀 정보로 취급하기 때문에 구현에 어려움이 있다.

또한 DRAM chip을 수정하는 것은 상용 DRAM chip의 경우 제조 비용의 증가 또는 기존 시스템과의 호환성 문제를 초래할 수도 있다.

BlockHammer는 위의 문제점을 해결하였다. 즉, RowHammer에 더 취약하더라도 효율적으로 확장 가능한(scalable) 메커니즘이며, 상용 DRAM chip과 완벽히 호환 가능하고, DRAM chip 내부 정보나 수정이 필요 없는 메커니즘이다.

## IV. BlockHammer

BlockHammer는 2가지 목표를 달성하기 위해 설계되었다.

1. RowHammer 취약성이 증가하더라도 효율적으로 확장 가능한 시스템
2. 상용 DRAM chip과의 완벽한 호환성 유지

이를 해결하기 위해 두 가지 주요 구성 요소로 BlockHammer를 설계하였다.

1. RowBlocker: DRAM 행이 RowHammer bit-flip을 유발할 정도로 높은 활성화 속도를 겪는 것을 방지
2. AttackThrottler: RowHammer 공격이 정상 애플리케이션 성능에 미치는 영향을 완화

RowBlocker는 DRAM 행의 활성화 속도를 낮게 유지한다. 이는 다음과 같은 방법으로 달성된다.

- Bloom Filter를 사용하여 행 활성화 비율을 효율적으로 추적
- 높은 활성화 속도를 보이는 행의 활성화를 차단 또는 제한하여 RowHammer 안전성 보장

RowBlocker는 완전히 메모리 컨트롤러 내에 구현되기 때문에, DRAM chip의 내부 정보나 수정이 필요하지 않다. 따라서 모든 상용 DRAM chip과 호환성을 가진다.

AttackThrottler는 RowHammer 공격 자체가 시스템 성능에 미치는 영향을 줄이는 역할을 한다.

- RowHammer 공격을 수행하는 것으로 식별되는 Thread의 메모리 대역폭 사용량을 제한
- 정상 애플리케이션이 더 높은 메모리 대역폭을 사용할 수 있도록 메모리 리소스 재분배

AttackThrottler는 공격 Thread를 제한하여 정상 애플리케이션의 성능을 보호해준다.

### IV-1. RowBlocker

- 성능 오버헤드

  - 배경: 많은 정상 애플리케이션은 최근 활성화된 DRAM 행을 반복적으로 활성화하는 경향이 있습니다【44, 45, 57, 76】. 이는 제한 메커니즘이 잘못된 판단으로 정상 애플리케이션을 제한할 위험을 증가시켜 시스템 성능을 저하시킬 수 있습니다.
해결 방법: RowBlocker는 특정 임계값을 초과하는 활성화 속도를 보이는 행만 제한하여 RowHammer 비트 플립을 방지합니다.
구성 요소:
RowBlocker-BL: 활성화 속도가 임계값(NBL)을 초과하는 행을 블랙리스트에 추가.
RowBlocker-HB: 최근 활성화된 행을 기록하고, 블랙리스트에 추가된 행의 활성화를 일정 시간(tDelay) 동안 지연.
RowBlocker는 블랙리스트에 등록된 행만 제한하므로, 정상 애플리케이션의 활성화 요청에 미치는 영향을 최소화합니다.


다음은 논문 3.1 RowBlocker 부분의 번역입니다:

3.1 RowBlocker
RowBlocker의 목표는 RowHammer 공격이 발생할 가능성을 완전히 제거하기 위해 DRAM 행의 활성화 속도를 효율적으로 제한하는 것입니다. RowBlocker는 성능 및 면적 오버헤드와 관련된 두 가지 주요 문제를 해결합니다.

첫 번째 문제: 성능 오버헤드
배경: 많은 정상 애플리케이션은 최근 활성화된 DRAM 행을 반복적으로 활성화하는 경향이 있습니다【44, 45, 57, 76】. 이는 제한 메커니즘이 잘못된 판단으로 정상 애플리케이션을 제한할 위험을 증가시켜 시스템 성능을 저하시킬 수 있습니다.
해결 방법: RowBlocker는 특정 임계값을 초과하는 활성화 속도를 보이는 행만 제한하여 RowHammer 비트 플립을 방지합니다.
구성 요소:
RowBlocker-BL: 활성화 속도가 임계값(NBL)을 초과하는 행을 블랙리스트에 추가.
RowBlocker-HB: 최근 활성화된 행을 기록하고, 블랙리스트에 추가된 행의 활성화를 일정 시간(tDelay) 동안 지연.
RowBlocker는 블랙리스트에 등록된 행만 제한하므로, 정상 애플리케이션의 활성화 요청에 미치는 영향을 최소화합니다.
두 번째 문제: 면적 오버헤드
배경: 활성화 제한을 위해서는 전체 갱신 창(refresh window) 동안 모든 행의 활성화 횟수를 추적해야 하며, 이는 면적 오버헤드를 증가시킬 수 있습니다.
해결 방법: RowBlocker는 **Counting Bloom Filter (CBF)**를 사용하여 면적 효율적으로 활성화 속도를 추적합니다.
두 개의 CBF를 시간 교차(time-interleaved) 방식으로 사용하여 정보 손실 없이 대규모 시간 창을 관리.
자세한 CBF 작동 방식은 3.1.1 섹션에서 설명합니다.

RowBlocker의 작동 원리
RowBlocker는 메모리 요청 스케줄러를 수정하여, RowHammer 비트 플립 위험이 있는 활성화 요청을 일시적으로 차단(또는 지연)합니다. 구체적인 작동 과정은 다음과 같습니다:

활성화 요청 확인:
메모리 요청 스케줄러가 특정 행을 활성화하려고 할 때, RowBlocker는 요청된 행이 RowHammer 안전한지 확인합니다.
이를 위해 다음 두 가지를 동시에 실행합니다:
RowBlocker-BL에서 해당 행이 블랙리스트에 있는지 확인.
RowBlocker-HB에서 해당 행이 최근 활성화되었는지 확인.
활성화 차단:
행이 블랙리스트에 포함되어 있고 최근 활성화된 경우, RowBlocker는 해당 활성화를 차단.
차단 이유: 블랙리스트에 포함된 행을 계속 활성화하면 RowHammer 비트 플립 발생 가능성이 증가하기 때문입니다.
차단된 요청은 일정 시간이 지난 후(tDelay) 안전한 활성화로 간주되어 다시 허용됩니다.
데이터 업데이트:
활성화가 허용되면, RowBlocker는 활성화된 행에 대한 정보를 RowBlocker-BL 및 RowBlocker-HB에 업데이트합니다.

RowBlocker-BL과 RowBlocker-HB 간의 역할 분담
RowBlocker-BL:
특정 행의 활성화 속도가 임계값(NBL)을 초과했는지 감지.
Counting Bloom Filter(CBF)를 사용하여 효율적으로 속도를 추적.
RowBlocker-HB:
특정 행이 최근 활성화되었는지 기록.
Content-Addressable Memory(CAM) 구조를 사용하여 빠르게 검색 가능.

3.1.1 RowBlocker-BL 메커니즘
RowBlocker-BL은 두 개의 **Counting Bloom Filter (CBF)**를 시간 교차(time-interleaved) 방식으로 사용하여, 특정 행이 블랙리스트에 포함되어야 하는지를 결정합니다. 각 CBF는 번갈아 가며 블랙리스트 여부를 판단하는 역할을 합니다. 특정 행이 **블랙리스트 임계값(NBL)**을 초과하는 활성화 속도를 보이면, 해당 행은 블랙리스트에 추가되며, 이후 활성화가 제한됩니다.

Counting Bloom Filter (CBF)의 작동 방식
CBF의 구조:

Counting Bloom Filter는 일반 Bloom Filter의 비트 배열을 카운터 배열로 대체하여, 데이터가 삽입된 횟수를 추적할 수 있습니다.
데이터가 삽입되면, 해시 함수로 계산된 인덱스에 대응하는 카운터를 증가시킵니다.
특정 데이터가 블랙리스트 임계값(NBL)을 초과하여 삽입된 경우, 해당 데이터는 블랙리스트에 포함됩니다.
False Negative 없음: CBF는 특정 데이터가 삽입되었을 때 이를 놓치지 않으므로 안전성을 보장합니다. 그러나, False Positive(잘못된 블랙리스트 포함)는 가능할 수 있습니다.
CBF의 작동 단계:

삽입: 특정 행이 활성화될 때, 해당 행의 주소가 해싱되어 CBF에 삽입됩니다.
테스트: 해당 행의 활성화 횟수가 블랙리스트 임계값(NBL)을 초과했는지 확인합니다.
초기화와 갱신:
CBF는 일정 주기마다 초기화(clear)되어 새로운 데이터 추적을 시작합니다.
초기화로 인해 발생할 수 있는 데이터 손실을 방지하기 위해 두 개의 CBF를 번갈아 사용합니다.
이중 CBF(Dual Counting Bloom Filters) 사용
RowBlocker-BL은 **Dual Counting Bloom Filter (D-CBF)**를 사용하여 블랙리스트를 효율적으로 유지합니다. D-CBF의 작동 방식은 다음과 같습니다:

활성(active) CBF와 비활성(passive) CBF:
두 개의 CBF가 교대로 활성/비활성 상태를 유지합니다.
한 CBF가 활성 상태에서 테스트 요청을 처리하는 동안, 다른 CBF는 데이터를 삽입하거나 초기화 작업을 수행합니다.
블랙리스트 결정:
활성 CBF에서 특정 행의 카운터 값이 블랙리스트 임계값(NBL)을 초과하면, 해당 행은 블랙리스트에 포함됩니다.
블랙리스트에 포함된 행은 이후 활성화가 제한됩니다.
D-CBF의 장점
정보 손실 방지: 초기화 과정에서도 데이터를 보존하여 중요한 활성화 정보를 유지.
효율적인 공간 사용: Bloom Filter의 공간 효율성을 유지하면서도 활성화 횟수를 정확히 추적.
D-CBF의 작동 과정
CBF 교체:
일정 시간이 지나면 활성 CBF와 비활성 CBF의 역할을 교환합니다.
비활성 CBF는 초기화되며, 새로운 데이터를 수집할 준비를 합니다.
블랙리스트 유지:
두 CBF를 교차(interleave)하여 블랙리스트의 신선함을 유지합니다.
특정 행의 활성화 속도가 블랙리스트 임계값(NBL)을 초과하면, 이 값은 새롭게 활성화된 CBF에서도 반영됩니다.
CBF의 False Positive 방지
RowBlocker-BL은 해시 함수의 **랜덤 시드(seed)**를 주기적으로 변경하여 False Positive를 줄입니다.
특정 행이 블랙리스트에 잘못 포함되지 않도록, CBF 초기화 시 새로운 해시 시드를 생성하여 해시 함수 간 충돌 가능성을 최소화합니다

3.1.2 RowBlocker-HB 메커니즘
RowBlocker-HB의 목표는 블랙리스트에 포함된 행이 RowHammer 비트 플립을 유발할 정도로 자주 활성화되지 않도록 활성화 지연(delay)을 적용하는 것입니다. 이를 위해 RowBlocker-HB는 특정 시간 동안(tDelay) 최근 활성화된 행의 기록을 유지하고, 해당 시간 내의 반복 활성화를 제한합니다.

RowBlocker-HB의 작동 원리
히스토리 버퍼(History Buffer) 유지:

RowBlocker-HB는 선입선출(FIFO) 방식의 히스토리 버퍼를 사용하여, 최근 활성화된 행의 기록을 저장합니다.
각 버퍼 항목에는 다음 정보가 포함됩니다:
행 ID: DRAM 랭크 내에서 고유한 ID.
타임스탬프: 해당 행이 활성화된 시간.
유효 플래그(valid bit): 활성화 기록이 여전히 유효한지 여부.
활성화 요청 처리:

RowBlocker-HB는 활성화 요청이 들어오면 해당 행 ID를 히스토리 버퍼에서 검색하여, 해당 행이 최근에 활성화된 적이 있는지를 확인합니다.
만약 행이 tDelay 시간 내에 활성화된 기록이 있으면, 요청을 차단하거나 지연합니다.
그렇지 않으면, 요청을 허용하고 히스토리 버퍼를 업데이트합니다.
히스토리 버퍼 갱신:

히스토리 버퍼는 주기적으로 가장 오래된 기록을 제거합니다.
tDelay 시간을 초과한 기록은 유효하지 않은 것으로 간주하고 삭제합니다.
히스토리 버퍼 구현
구조:

히스토리 버퍼는 순환 큐(circular queue) 형태로 구현됩니다.
두 개의 포인터(head와 tail)가 가장 오래된 항목과 가장 최신 항목을 나타냅니다.
동작:

새로운 활성화가 발생할 때, 행 ID와 타임스탬프가 버퍼의 tail에 추가됩니다.
가장 오래된 항목(버퍼의 head)이 tDelay 시간을 초과하면, 해당 항목의 유효 플래그(valid bit)를 0으로 설정하여 제거됩니다.
검색 효율성:

RowBlocker-HB는 **Content-Addressable Memory (CAM)**를 사용하여 행 ID를 병렬로 검색합니다.
버퍼 내의 항목 중 행 ID가 일치하는 항목이 있으면, 그 행이 최근 활성화된 것으로 판단합니다.
tDelay의 설정
RowHammer 임계값과의 관계:

RowHammer 비트 플립이 발생하지 않으려면, 특정 행의 활성화 횟수가 RowHammer 임계값(NRH)을 초과하지 않아야 합니다.
tDelay는 DRAM의 갱신 창(refresh window, tREFW) 동안 허용 가능한 활성화 속도를 기준으로 설정됩니다.
tDelay 계산:

tDelay는 다음과 같은 방식으로 정의됩니다:

$asd$
 
여기서:

tCBF: Counting Bloom Filter의 수명.
NBL: 블랙리스트 임계값.
tRC: 동일 은행에서의 연속 활성화 간 최소 시간.
tREFW: DRAM 갱신 창.
의미:

tDelay는 특정 행이 tDelay 시간 내에 반복적으로 활성화되지 않도록 지연시키는 최소 시간입니다.
이는 RowHammer 비트 플립이 발생할 가능성을 완전히 제거합니다.

3.1.3 Configuration (구성)
RowBlocker는 조정 가능한 세 가지 주요 구성 파라미터를 가지고 있습니다. 이 파라미터들은 RowBlocker의 **오탐률(false positive rate)**과 면적 특성을 정의합니다:

CBF 크기: Counting Bloom Filter(CBF)에 포함된 카운터의 수.
tCBF: CBF의 수명.
NBL: 블랙리스트 임계값.
이 파라미터들의 설정은 CBF의 저장소 요구사항과 Bloom Filter에서 행 주소들이 동일 카운터에 매핑될 가능성에 직접적인 영향을 미칩니다. 또한, NBL과 tCBF의 설정은 RowHammer-safe 동작을 위해 필요한 활성화 간 지연(tDelay)을 결정하며, 각 에포크(epoch) 동안 RowBlocker가 추적해야 하는 행의 최대 수를 좌우합니다.

파라미터 설정 방법
RowBlocker의 각 파라미터에 대해 적절한 값을 설정하기 위해 세 단계 방법론을 사용합니다:

CBF 크기 선택:

CBF의 크기는 실험에서 관찰된 오탐률을 기반으로 경험적으로 선택합니다.
CBF 크기를 1,024 카운터로 설정했습니다. 이는 크기를 줄였을 때 Bloom Filter의 충돌(aliasing)로 인해 오탐률이 급격히 증가하는 것을 방지합니다.
NBL 설정:

NBL은 다음 세 가지 목표를 충족하도록 설정됩니다:
**RowHammer 임계값(NRH)**보다 작아야 하여 RowHammer 비트 플립을 방지.
정상 애플리케이션의 활성화 횟수보다 충분히 커야 하여 오탐으로 인해 정상 애플리케이션이 블랙리스트에 포함되지 않도록 보장.
tDelay(블랙리스트 행의 활성화 지연 시간)를 최소화하기 위해 가능한 한 낮게 설정.
125개의 멀티프로그램 워크로드를 분석한 결과, NBL을 8,192(8K)로 설정했습니다. 이는 RowHammer 임계값(32K)의 1/4에 해당하며, 정상 애플리케이션의 활성화 패턴을 기반으로 낮은 오탐률(0.01% 미만)을 보장합니다.
tCBF 설정:

tCBF는 잘못된 블랙리스트로 인해 발생하는 지연(tDelay)을 최소화하도록 설정됩니다.
tCBF를 DRAM의 갱신 창(tREFW)과 동일하게 설정하여, RowBlocker가 RowHammer-safe 동작을 유지하도록 최적화합니다.

Tuning for Different DRAM Standards (다양한 DRAM 표준에 대한 조정)
BlockHammer의 파라미터 값은 메모리 표준에서 정의된 세 가지 주요 시간 제약 조건에 따라 설정됩니다:

tRC: 동일한 뱅크에서의 최소 활성화 간 시간.
tREFW: DRAM 갱신 창.
tFAW: 4번의 활성화가 가능한 시간 창.
BlockHammer에서 tDelay는 다음과 같은 방식으로 영향을 받습니다:

tREFW와 선형적으로 비례합니다.
tRC에 의해 약간 영향을 받습니다(Equation 1에 따라).
세부 시간 제약 조건에 따른 조정
DDR 표준에서의 tREFW:

DDR에서 DDR4까지 tREFW는 64ms로 일정하게 유지됩니다【51-55】.
tRC는 55ns에서 46.25ns로 소폭 감소했습니다【53-55, 95, 96】.
따라서, 여러 DDR 세대에 걸쳐 tDelay는 약간만 증가합니다.
LPDDR4에서의 tREFW:

LPDDR4에서는 tREFW가 절반으로 감소합니다.
이는 tDelay를 줄이고, 블랙리스트 행에 대한 지연 시간을 감소시킵니다.
tFAW의 영향:

tFAW는 히스토리 버퍼 크기에만 영향을 미칩니다.
tFAW 값은 현대 DRAM 표준에서 30~45ns 사이에서 변동합니다【53-55, 95, 96】.
결론
BlockHammer는 다양한 DRAM 표준에 맞추어 조정될 수 있습니다. DDRx 표준에서 일정한 갱신 창(tREFW)과 LPDDR4에서의 절반 갱신 창은 각각 블랙리스트 행의 지연 시간을 최적화하는 데 사용됩니다. 이러한 조정은 BlockHammer가 여러 세대의 DRAM 기술에서 RowHammer를 방지하는 데 효과적으로 작동할 수 있도록 합니다.

### IV-2. AttackThrottler
AttackThrottler의 주요 목표는 RowHammer 공격이 발생하는 동안 정상 애플리케이션의 성능 저하를 최소화하는 것입니다. 이를 위해 AttackThrottler는 다음 두 가지 작업을 수행합니다:

RowHammer 공격으로 의심되는 스레드를 식별합니다.
의심 스레드의 메모리 대역폭 사용을 제한하여 정상 애플리케이션의 성능을 보호합니다.
RowHammer 공격 스레드 식별
기본 개념:

RowHammer 공격은 특정 DRAM 행에 매우 높은 활성화 빈도를 요구합니다.
RowBlocker-BL은 높은 활성화 속도를 가진 행을 식별하므로, AttackThrottler는 이러한 데이터를 활용하여 공격 스레드를 효율적으로 감지할 수 있습니다.
활성화 패턴 분석:

RowBlocker-BL에 의해 블랙리스트에 포함된 DRAM 행을 활성화한 스레드를 추적합니다.
일정 시간 창 내에서 블랙리스트 행을 지속적으로 활성화한 스레드를 공격 스레드로 식별합니다.
공격 스레드에 대한 대역폭 제한
목표:

공격 스레드가 DRAM 메모리 대역폭을 과도하게 사용하지 못하도록 제한.
정상 애플리케이션이 DRAM 대역폭을 충분히 사용할 수 있도록 보장.
작동 방식:

할당 대역폭 추적:
모든 스레드에 대해 메모리 대역폭 사용량을 추적합니다.
공격 스레드로 식별된 스레드에 대한 대역폭 사용 제한을 설정합니다.
정상 스레드 보호:
정상 스레드에 할당된 대역폭은 유지되며, 공격 스레드로 인해 방해받지 않도록 보장합니다.
성능 보호 메커니즘
정상 애플리케이션 성능 최적화:

AttackThrottler는 정상 애플리케이션의 DRAM 대역폭을 최적화하여 RowHammer 공격의 영향을 최소화합니다.
이를 통해 공격 중에도 정상 워크로드의 성능이 유지됩니다.
동적 대역폭 관리:

공격 스레드의 메모리 대역폭은 DRAM의 사용 상태에 따라 동적으로 조정됩니다.
대역폭 제한은 실시간으로 업데이트되며, 공격 강도에 따라 유연하게 적용됩니다.
AttackThrottler의 장점
효율성:
RowBlocker-BL의 데이터를 활용하여 공격 스레드를 빠르고 정확하게 식별.
성능 오버헤드가 낮음.
확장성:
여러 워크로드와 DRAM 구성에서 효과적으로 작동.
정상 스레드 보호:
정상 애플리케이션 성능이 RowHammer 공격의 영향을 받지 않도록 보장.

3.2.1 진행 중인 RowHammer 공격 식별
AttackThrottler는 특정 스레드가 RowHammer 공격을 수행하고 있는지를 식별하기 위해 새로운 지표인 **RowHammer Likelihood Index (RHLI)**를 사용합니다. RHLI는 주어진 스레드의 메모리 접근 패턴과 실제 RowHammer 공격 간의 유사성을 정량화합니다.

RHLI 계산
RHLI는 각 <스레드, DRAM 뱅크> 쌍에 대해 계산됩니다.

RHLI는 스레드가 블랙리스트에 포함된 행을 활성화한 횟수를, BlockHammer 시스템에서 허용 가능한 블랙리스트 행 활성화 최대 횟수로 정규화한 값으로 정의됩니다.

RowBlocker는 특정 CBF 수명 동안 행 활성화 횟수를 RowHammer 임계값(NRH)로 제한하며, 이는 다음과 같이 정의됩니다:


NBL: 블랙리스트 임계값.
RHLI 해석
RHLI 값이 0일 경우, 스레드는 특정 뱅크에서 RowHammer 공격을 수행하지 않았음을 의미합니다.
RHLI 값이 1에 도달하면, 해당 스레드는 RowHammer 비트 플립을 유발할 가능성이 매우 높은 것으로 간주됩니다.
BlockHammer 시스템에서는 RHLI가 1을 초과하지 않습니다. 이는 RHLI가 1에 도달하면 AttackThrottler가 해당 스레드의 메모리 접근을 차단하기 때문입니다.
실험 결과
실험에서 RHLI를 통해 RowHammer 공격 스레드와 정상 스레드를 구분했습니다.
관찰 모드(Observe-Only Mode):
RowBlocker의 블랙리스트 로직과 AttackThrottler의 카운터를 활성화하되, 메모리 요청을 차단하지 않는 상태에서 RHLI를 계산했습니다.
이 모드에서 정상 애플리케이션은 RHLI 값이 0을 유지했으며, RowHammer 공격 스레드는 평균 RHLI 값이 10.9로 나타났습니다.
완전 작동 모드(Full-Functional Mode):
BlockHammer가 정상적으로 작동하며, RowHammer 공격 스레드의 요청을 차단했습니다.
이 모드에서는 RowHammer 공격 스레드의 RHLI가 평균 54배 감소하여 1 이하로 낮아졌습니다.
RHLI 계산 및 저장
AttackThrottler는 각 <스레드, 뱅크> 쌍에 대해 두 개의 카운터를 사용합니다. 카운터는 RowBlocker의 Dual Counting Bloom Filter(D-CBF)와 유사하게 시간 교차 방식으로 작동합니다:
하나는 활성 카운터, 다른 하나는 비활성 카운터로 설정됩니다.
활성 카운터는 현재 RHLI 계산에 사용되며, RowBlocker가 활성 필터를 초기화하면 카운터를 교체합니다.
RHLI 카운터는 포화 카운터로 구현되며, 카운터 값이 RowHammer 임계값을 초과하지 않도록 설계되었습니다.
저장 요구사항
AttackThrottler의 카운터는 각 <스레드, 뱅크> 쌍에 대해 4바이트의 저장소를 필요로 합니다.
8개의 스레드와 16개의 DRAM 뱅크를 가진 시스템에서는 총 512바이트의 추가 저장소가 필요합니다.

3.2.2 Throttling RowHammer Attack Threads (RowHammer 공격 스레드 제한)
AttackThrottler는 RowHammer 공격 스레드로 식별된 스레드의 메모리 접근 속도를 동적으로 제한하여 RowHammer 공격을 완화합니다. 이 제한은 RowHammer Likelihood Index (RHLI)에 따라 조정됩니다.

1. 제한 메커니즘
AttackThrottler는 RowHammer 공격 스레드의 메모리 요청을 줄이기 위해 다음의 정책을 적용합니다:

최대 요청 비율 설정:
특정 스레드의 메모리 요청이 전체 대역폭에서 차지하는 최대 비율을 설정합니다.
요청 스케줄링 지연:
공격 스레드의 요청을 지연시키거나, 블랙리스트 행과 관련된 요청의 우선순위를 낮춥니다.
스레드가 사용하는 RHLI 값에 따라 제한 강도를 조정합니다:

RHLI ≥ 1: 스레드의 메모리 요청이 RowHammer 비트 플립을 유발할 가능성이 높은 상태이며, 강력한 제한이 필요합니다.
RHLI < 1: 스레드가 정상 애플리케이션에 가까운 동작을 보이므로 제한이 완화됩니다.
2. 제한 정책 설계
효율적인 성능 유지:

AttackThrottler는 정상 애플리케이션 성능을 유지하기 위해, 제한이 RowHammer 공격 스레드에만 집중되도록 설계되었습니다.
이를 위해 메모리 요청의 스레드 ID와 RHLI를 활용하여 정확하게 제한 대상을 식별합니다.
공격 완화 및 성능 균형:

제한은 공격 스레드의 요청 비율이 RowHammer 안전 임계값을 초과하지 않도록 보장합니다.
동시에 정상 애플리케이션의 메모리 요청은 우선 처리됩니다.
3. 저장 및 처리 요구사항
AttackThrottler는 제한 정책을 적용하기 위해 메모리 요청 스케줄러와 통합됩니다.
각 스레드에 대해 현재 RHLI와 제한 상태를 저장하며, 이 저장소는 각 <스레드, 뱅크> 쌍당 약 4바이트의 추가 메모리를 요구합니다.
3.2.3 Exposing RHLI to the System Software (RHLI를 시스템 소프트웨어에 노출)
AttackThrottler는 RowHammer Likelihood Index (RHLI)를 시스템 소프트웨어에 노출하여, 운영 체제와 같은 상위 소프트웨어 계층에서 추가적인 보안 정책을 적용할 수 있도록 합니다.

1. RHLI의 역할
RHLI는 특정 스레드가 RowHammer 공격을 수행할 가능성을 나타내는 지표로, 소프트웨어 계층에 중요한 정보를 제공합니다.
운영 체제(OS)는 RHLI 데이터를 기반으로 공격을 완화하거나 스레드의 실행을 제한하는 조치를 취할 수 있습니다.
2. 운영 체제와의 통합
스레드 관리:

운영 체제는 RHLI 값이 높은 스레드의 우선순위를 낮추고, 정상 애플리케이션이 시스템 리소스를 더 잘 사용할 수 있도록 보장합니다.
RHLI가 높은 스레드가 계속해서 공격 패턴을 보이면, 운영 체제는 해당 스레드를 더 강력하게 제한하거나 실행을 중단할 수 있습니다.
정책 적용:

운영 체제는 RHLI를 활용하여 동적 스레드 스케줄링을 수행하고, 메모리 대역폭을 효율적으로 분배합니다.
예를 들어, 공격 스레드가 자주 활성화하는 행과 같은 데이터는 메모리 접근을 제한하거나, 공격에 취약한 데이터와 분리할 수 있습니다.

3. 시스템 로그 및 보안 관리
공격 감지 기록:

운영 체제는 RHLI 값을 기반으로 RowHammer 공격의 발생 여부를 기록합니다.
이 기록은 보안 관리자와 시스템 설계자가 시스템 내 RowHammer 취약성을 분석하고 대응책을 수립하는 데 사용됩니다.
사후 분석 지원:

RHLI 데이터는 RowHammer 공격이 발생한 시점, 영향을 받은 스레드, 관련된 메모리 뱅크 및 행 정보를 포함합니다.
이를 통해 시스템 관리자와 보안 팀은 RowHammer 공격을 분석하고 대응 전략을 개선할 수 있습니다.

## V. Many-Sided RowHammer Attacks
4. Many-Sided RowHammer Attacks (다면적 RowHammer 공격)
RowHammer 공격은 특정 DRAM 행(공격 행, aggressor row)을 반복적으로 활성화하여 물리적으로 가까운 행(희생 행, victim row)에 비트 플립을 유발합니다. 공격 행이 반드시 희생 행에 바로 인접하지 않더라도, 물리적으로 가까운 거리에 있는 행들을 간섭하여 비트 플립을 유발할 수 있습니다【73, 35】. 이는 다수의 DRAM 행을 동시에 공격하여 누적 간섭 효과로 RowHammer 비트 플립을 발생시키는 다면적 RowHammer 공격이 가능함을 의미합니다【72, 73】.

물리적 거리와 간섭 영향
연구에 따르면, 공격 행이 희생 행에 미치는 영향은 두 행 간의 물리적 거리가 멀어질수록 감소합니다.
예를 들어, 두 행 간의 거리가 행 단위로 증가할 때마다 간섭 효과는 대략 한 자릿수씩 감소합니다.
물리적 거리가 특정 임계치를 초과하면 간섭 효과는 사라집니다.
최신 연구에서는 이 거리 임계치가 약 6개의 행임을 보고하고 있습니다【72, 73】.
BlockHammer의 다면적 RowHammer 공격 방지 전략
BlockHammer는 다면적 RowHammer 공격을 방지하기 위해 다음과 같은 보수적인 접근 방식을 채택합니다:

누적 간섭 효과 계산:

공격 행 각각의 영향을 합산하여 RowHammer 임계값(NRH)을 재설정합니다.
이렇게 함으로써, 여러 행이 동시에 NRH*만큼 활성화되더라도, 인접한 단일 행이 NRH만큼 활성화된 경우와 동일한 간섭 수준을 가지도록 보장합니다.
NRH 계산*: NRH*는 다음 세 가지 주요 매개변수를 사용하여 계산됩니다:

NRH: 단일 행에 대한 RowHammer 임계값.
blast radius (rblast): 공격 행의 간섭 효과가 관찰될 수 있는 최대 물리적 거리(행 단위).
blast impact factor (ck): 인접 행과 k번째 떨어진 행에서 발생한 비트 플립 간의 활성화 횟수 비율.
간섭 효과는 아래와 같이 계산됩니다:


최악의 경우 시나리오
최신 DRAM 칩의 실험 결과를 바탕으로, 최악의 경우 시나리오는 다음과 같이 정의됩니다:

rblast = 6: 공격 행의 영향이 미치는 최대 거리.
ck = 0.5^{k-1}: 간섭 효과가 거리에 따라 지수적으로 감소.
이 값을 사용하여 계산한 결과:

NRH*는 NRH의 약 **25.39%**로 설정됩니다.
이는 다면적 RowHammer 공격을 방지하기 위한 최소 임계값을 나타냅니다.


## VI. Security Analysis

BlockHammer는 **모순 증명 방식(proof by contradiction)**을 사용하여 어떤 RowHammer 공격도 이를 우회할 수 없음을 증명합니다. 즉, BlockHammer를 우회하여 특정 DRAM 행을 갱신 주기(refresh window) 내에 NRH(RowHammer Threshold) 이상 활성화하는 접근 패턴이 존재할 수 없음을 수학적으로 입증합니다.

증명 과정 개요
가정:
BlockHammer를 우회하여 특정 DRAM 행을 NRH 이상 활성화할 수 있는 접근 패턴이 존재한다고 가정합니다.
수학적 표현:
가능한 모든 행 활성화 분포를 수학적으로 모델링하고, 주어진 시간 창에서 특정 행이 NRH 이상 활성화되기 위한 제약 조건을 정의합니다.
모순 발견:
정의된 제약 조건이 충족될 수 없음을 증명하며, 따라서 이러한 접근 패턴이 존재하지 않음을 보입니다.
본 논문의 공간 제한으로 인해 증명 과정의 모든 단계를 간략히 요약하였으며, 전체 증명은 확장 버전에 제공됩니다## 위협 모델 BlockHammer는 포괄적인 위협 모델을 가정하며, 이 모델에서 공격자는 다음과 같은 능력을 가집니다:

메모리 대역폭의 완전한 활용:
DRAM의 전체 메모리 대역폭을 효율적으로 사용.
정밀한 메모리 요청 타이밍:
각 메모리 요청의 타이밍을 정밀하게 제어.
메모리 컨트롤러와 BlockHammer의 동작 완전 이해:
메모리 컨트롤러, BlockHammer, DRAM 구현 세부사항에 대한 완전한 지식 보유.
이 모델에서는 메모리 컨트롤러와 DRAM 칩, 두 하드웨어 요소와 이들을 연결하는 물리적 인터페이스를 신뢰 가능한 컴포넌트로 간주합니다. 그 외의 하드웨어 및 소프트웨어 요소는 신뢰할 수 없다고 가정합니다.

공격 설계 모델
RowHammer 공격의 메모리 접근 패턴은 공격 행(aggressor row)의 관점에서 모델링됩니다:

공격은 RowBlocker의 **D-CBF(Dual Counting Bloom Filter)**의 갱신 명령(clear command)에 의해 구분되는 에포크(epoch) 단위로 표현됩니다.
각 에포크에서 공격 행의 총 활성화 횟수는 D-CBF의 블랙리스트 기준에 따라 제한됩니다.
에포크별 활성화 제한
BlockHammer는 모든 가능한 RowHammer 공격 패턴에 대해 DRAM 행의 활성화 횟수를 에포크 단위로 제한합니다:

활성화 분류:
각 에포크는 이전 에포크(
𝑁
𝑒
𝑝
−
1
Nep 
−1
​
 )와 현재 에포크(
𝑁
𝑒
𝑝
Nep)의 활성화 횟수를 기반으로 분류됩니다.
제한 타입 정의:
에포크는 다섯 가지 타입(T0~T4)으로 분류되며, 각 타입의 활성화 가능 범위가 정의됩니다.

## VII. Hardware Complexity Analysis

개요
BlockHammer의 하드웨어 복잡도는 다음 주요 항목으로 분석됩니다:

칩 면적, 정적 전력 및 접근 에너지 소모 (CACTI를 사용해 측정)** (Synopsys DC를 사용하여 분석) .
BlockHam최신 RowHammer 완화 메커니즘과 경쟁 가능한 수준입니다.

6.1 칩 면적, 정적 전력 및 접근 에너지
BlockHammer는 RowBlocker와 AttackThrottler의 조합으로 구성됩니다:
RowBlocker:
RowBlocker-BL: DRAM 뱅크당 듀얼 카운팅 Bloom Filter를 구현.
RowBlocker-HB: DRAM 랭크당 행 활성화 기록 버퍼를 구현.
AttackThrottler:
DRAM 뱅크당 스레드별 RHLI를 추적하는 카운터 두 개를 사용.
주요 구성 요소 및 칩 면적
Bloom Filter:

각 DRAM 뱅크당 1,024개의 13비트 카운터로 구성된 SRAM 배열로 저장.
4개의 H3 해시 함수로 색인 처리.
면적 오버헤드: DRAM 랭크당 0.14 mm².
히스토리 버퍼:

각 DRAM 랭크에 887개의 항목을 저장.
각 항목: 32비트 (행 ID, 타임스탬프, 유효 비트).
총 면적 오버헤드:

DDR4 메모리에서 DRAM 랭크당 0.14 mm².
RowHammer 임계값(NRH)에 따른 면적 변화
NRH = 32K:
총 면적 오버헤드: 0.55 mm² (CPU 다이 면적의 0.06%).
NRH = 1K:
Bloom Filter 크기 증가 및 히스토리 버퍼 항목 확장으로 총 면적 오버헤드 1.57 mm²로 증가 (CPU 다이 면적의 0.64%).
경쟁 기술과의 비교
BlockHammer의 칩 면적과 에너지 소모는 다른 최신 기술 대비 효율적입니다:

Graphene, TWiCe, CBT 등의 메타데이터 저장 요구사항에 비해 낮은 면적 오버헤드.
PARA, PRoHIT, MRLoc과 같은 확률적 메커니즘 대비 효율적.
6.2 회로 지연
BlockHammer의 회로 지연 분석:

Verilog HDL로 설계하고 Synopsys DC로 65nm 공정에서 합성.
"이 행이 RowHammer-safe인가?"에 대한 응답 시간: 0.97 ns.
이 지연은 DRAM 행 접근 지연(4550 ns)의 12 차수 작아 실질적인 성능 영향이 없음.

## VIII.Experimental Methodology

7. Experimental Methodology (실험 방법론)
BlockHammer의 성능, 에너지 소비, 하드웨어 오버헤드를 평가하기 위해 다양한 실험 환경과 도구를 사용했습니다. 실험은 다양한 RowHammer 완화 메커니즘과 비교하여 수행되었으며, 성능과 효율성을 종합적으로 검증했습니다.

7.1 평가 도구 및 시뮬레이션 환경
성능 평가:
Ramulator: DRAM 하위 시스템의 성능 시뮬레이션을 위해 사용된 오픈소스 메모리 시뮬레이터.
RowHammer 공격 패턴을 생성하고 메모리 시스템의 성능을 분석하기 위해 수정되었습니다.
에너지 소비 분석:
DRAMPower: DRAM의 전력 및 에너지 소비를 추정하기 위해 사용.
하드웨어 설계 평가:
CACTI 7.0: RowBlocker 및 AttackThrottler 구성 요소의 면적, 전력, 접근 에너지 소모를 평가하기 위해 사용.
7.2 시스템 구성
다음은 실험에 사용된 시스템의 하드웨어 및 메모리 구성입니다:

구성 요소	사양
프로세서	3.2GHz, 1코어 및 8코어, 4-와이드 명령 발행(issue), 128-엔트리 명령 창
최종 레벨 캐시	16MB, 64B 캐시 라인, 8-way 세트 연관
메모리 컨트롤러	읽기 및 쓰기 큐 각각 64엔트리, 스케줄링 정책: FR-FCFS, 주소 매핑 방식: MOP
주 메모리	DDR4, 1채널, 1랭크, 4뱅크 그룹(group), 각 그룹당 4뱅크, 뱅크당 64K 행
7.3 공격 모델
BlockHammer는 다음과 같은 공격 모델에서 평가되었습니다:

RowHammer 공격 모델:
Double-sided RowHammer 공격을 사용하여 BlockHammer의 성능을 검증.
RowHammer 임계값(NRH):
NRH는 동일한 갱신 창(tREFW) 내에 특정 DRAM 행이 활성화될 수 있는 최대 횟수로 설정.
실험에서는 NRH 값을 16,384로 설정했으며, 이는 기존 연구에서 사용된 값과 일치합니다.
7.4 비교 대상
BlockHammer의 성능은 다음의 최신 RowHammer 완화 메커니즘과 비교되었습니다:

확률적 메커니즘:
PARA: 메모리 접근 후 인접 행을 무작위로 갱신하여 RowHammer를 완화.
PRoHIT: 공격 행의 활성화 확률을 낮추기 위해 확률 기반 기법 적용.
MRLoc: RowHammer 감지 후 DRAM의 특정 지역(locality)만을 보호.
결정적 메커니즘:
TWiCe: 행 활성화 기록을 유지하고 반복 활성화를 제한.
Graphene: RowHammer를 감지하고 갱신 우선순위를 동적으로 조정.
CBT: 카운터 기반 메커니즘으로 특정 DRAM 행의 활성화 횟수를 추적.
7.5 RowHammer 완화 설정
PARA:

각 행의 활성화 후 인접 행을 활성화하는 확률을 설정하여 비트 플립 위험을 완화.
PARA의 확률 임계값은 데이터 무결성을 유지할 수 있는 최소값으로 조정되었습니다.
BlockHammer 설정:

RowBlocker와 AttackThrottler를 통합하여 모든 RowHammer 비트 플립 가능성을 차단.
NRH 및 관련 파라미터(tCBF, tDelay 등)는 DRAM 물리적 특성에 맞게 최적화.

7.6 워크로드
BlockHammer의 성능 및 에너지 효율성을 평가하기 위해 다음과 같은 워크로드를 사용했습니다:

멀티프로그래밍 워크로드:

SPEC CPU2006, SPEC CPU2017, TPC, YCSB 벤치마크 세트를 기반으로 생성된 멀티프로그래밍 워크로드.
각 워크로드는 8개의 애플리케이션으로 구성되며, 성능 및 메모리 접근 패턴 다양성을 반영.
RowHammer 공격 워크로드:

메모리 대역폭의 90%를 활용하는 Double-sided RowHammer 공격.
RowHammer 공격은 정상 워크로드와 함께 실행되어 BlockHammer의 공격 완화 효과와 정상 애플리케이션 성능 유지 능력을 평가.
7.7 평가 기준
BlockHammer는 다음 세 가지 기준에 따라 평가되었습니다:

성능 오버헤드:

멀티프로그래밍 워크로드에서 정상 애플리케이션 성능에 미치는 영향을 측정.
성능 오버헤드는 RowHammer 공격 유무에 따라 비교.
에너지 소비:

DRAMPower를 사용해 DRAM 전력 소비를 측정.
BlockHammer가 RowHammer 공격을 방지하는 동안 에너지 효율성을 유지하는지 분석.
보안 효과:

RowHammer 공격이 BlockHammer를 우회하여 DRAM 비트 플립을 유발할 가능성을 평가.

## IX. Performance and Energy Evaluation

BlockHammer와 여섯 가지 최신 RowHammer 완화 메커니즘의 성능 및 에너지 오버헤드를 비교하여 분석하였습니다. 분석은 네 가지 주요 측면으로 구성됩니다.

8.1 단일 코어 애플리케이션
단일 코어에서 실행되는 정상 애플리케이션의 실행 시간과 DRAM 에너지 소비를 비교하였습니다.
BlockHammer는 RowHammer 완화 메커니즘이 없는 기준 시스템과 비교하여 성능 및 에너지 오버헤드가 전혀 없는 것으로 나타났습니다.
정상 애플리케이션의 행 활성화 속도가 BlockHammer의 블랙리스트 임계값(NBL)을 초과하지 않기 때문입니다.
반면, PARA와 MRLoc은 높은 RBCPKI 애플리케이션에서 각각 평균 0.7% 및 0.8%의 성능 오버헤드, 4.9%의 에너지 오버헤드를 발생시켰습니다.
CBT, TWiCe, Graphene은 정상 애플리케이션의 활성화 속도가 희생 행 갱신을 트리거할 만큼 높지 않아, 성능 오버헤드가 없었습니다.
8.2 멀티프로그램 워크로드
8코어 시스템에서 멀티프로그램 워크로드를 사용하여 두 가지 시나리오를 비교하였습니다:
RowHammer 공격 없음: 모든 8개의 애플리케이션이 정상적.
RowHammer 공격 존재: 하나의 공격 스레드와 7개의 정상 애플리케이션이 함께 실행.
결과
RowHammer 공격 없음:

BlockHammer는 RowHammer 공격이 없는 경우에도 멀티프로그램 워크로드에서 매우 낮은 성능 오버헤드를 나타냅니다:
가중 속도 증가(weighted speedup): 0.5% 미만.
조화 속도 증가(harmonic speedup): 0.6% 미만.
최대 지연(maximum slowdown): 1.2% 미만.
PARA 및 MRLoc은 각각 1.2% 및 2.0%의 성능 오버헤드를 나타냈습니다.
RowHammer 공격 존재:

BlockHammer는 공격 스레드의 요청을 제한하여, 정상 애플리케이션의 성능 저하를 효과적으로 완화했습니다:
가중 속도 증가: 평균 45.0%, 최대 61.9% 개선.
조화 속도 증가: 평균 56.2%, 최대 73.4% 개선.
최대 지연 감소: 평균 22.7%, 최대 45.4% 감소.
BlockHammer는 DRAM 에너지 소비도 평균 28.9% 감소시켰습니다.
8.3 악화되는 RowHammer 취약성에 대한 효과
DRAM 칩이 점점 더 RowHammer에 취약해지는 경우(즉, RowHammer 임계값 NRH가 감소할 때) BlockHammer의 성능과 DRAM 에너지 소비가 어떻게 변화하는지 분석하였습니다.
PARA, TWiCe, Graphene과 비교하였으며, NRH가 1024까지 감소하는 경우를 포함하였습니다.
결과
BlockHammer는 NRH가 낮아질수록 에너지 소비와 성능 저하 측면에서 더 높은 효율성을 유지하였습니다:
TWiCe 및 CBT는 NRH 감소에 따라 면적 및 정적 전력 오버헤드가 크게 증가하였으나, BlockHammer는 이러한 오버헤드 증가를 제한하였습니다.

8.4 BlockHammer 내부 메커니즘 분석
BlockHammer의 성능과 DRAM 에너지 소비에 대한 영향은 다음 두 가지 요소에 의해 결정됩니다:

블랙리스트 메커니즘의 오탐률

BlockHammer의 Bloom 필터가 잘못된 행을 블랙리스트에 추가하여, 활성화를 부당하게 지연시키는 비율입니다.
이는 블랙리스트에 추가되지 않아야 할 행이 Bloom 필터의 aliasing(충돌)으로 인해 잘못 지연되는 경우 발생합니다.
오탐 페널티

잘못 지연된 행 활성화로 인해 추가적으로 발생하는 시간 지연입니다.
1. 오탐률 계산
RowHammer 임계값(NRH) 32K로 설정된 경우, BlockHammer의 오탐률은 **0.010%**로 나타났습니다.
NRH를 1K로 감소시킨 경우에도, 오탐률은 **0.012%**로 약간 증가하는 데 그쳤습니다.
따라서, BlockHammer는 전체 활성화 요청의 99.98% 이상이 지연 없이 처리되도록 보장합니다.
2. 오탐 페널티 분석
블랙리스트 행 활성화 요청에 대해 설정된 지연 시간(
𝑡
𝐷
𝑒
𝑙
𝑎
𝑦
t 
Delay
​
 )은 7.7 µs입니다.
잘못 지연된 활성화 요청의 지연 시간 분포는 다음과 같습니다:
50번째 백분위수(중위값): 1.7 µs
90번째 백분위수: 3.9 µs
100번째 백분위수(최악의 경우): 7.6 µs
3. 품질 보장
관찰된 최악의 지연 시간(7.6 µs)은 일반적인 서비스 품질(QoS) 목표(밀리초 수준)보다 최소 2차수 이상 작습니다.
이러한 낮은 오탐률과 µs 수준의 지연 시간은 BlockHammer가 서비스 품질(QoS) 위반 가능성을 거의 발생시키지 않음을 나타냅니다.
결론
BlockHammer는 Bloom 필터 기반 메커니즘으로 인해 오탐률이 극히 낮으며, 잘못된 지연이 발생하더라도 매우 짧은 시간 내에 처리됩니다. 이는 BlockHammer가 정상적인 DRAM 성능과 에너지 소비에 미치는 영향을 최소화하면서도 높은 보안성을 제공함을 입증합니다.

## X. Comparison of Mitigation Mechanisms

BlockHammer와 기존의 RowHammer 완화 메커니즘을 비교하기 위해, 이를 네 가지 주요 접근 방식으로 분류했습니다:

갱신 속도 증가 (Increased Refresh Rate):
모든 행을 더 자주 갱신하여 비트 플립 발생 확률을 줄이는 방법.
물리적 격리 (Physical Isolation):
민감한 데이터를 잠재적인 공격자 메모리 공간에서 물리적으로 격리하는 방법.
반응형 갱신 (Reactive Refresh):
행 활성화를 모니터링하고, 잠재적인 희생 행을 갱신하여 대응.
사전적 제한 (Proactive Throttling):
행 활성화 속도를 RowHammer-safe 수준으로 제한.
이 메커니즘들은 네 가지 평가 기준에 따라 비교되었습니다:

포괄적인 보호 (Comprehensive Protection): 공격자가 사용하는 방법에 상관없이 모든 RowHammer 비트 플립을 방지.
상용 DRAM 호환성 (Compatibility with Commodity DRAM Chips): 기존 상용 DRAM과의 호환성.
RowHammer 취약성 확장성 (Scaling with RowHammer Vulnerability): DRAM이 더 취약해지는 경우 확장 가능성.
결정적 보호 (Deterministic Protection): 항상 일관된 보호 제공.

비교 결과는 아래와 같다.

## XI. Conclusion

BlockHammer는 RowHammer 문제를 해결하기 위해 설계된 효율적이고 확장 가능한 메커니즘입니다. 이 논문에서는 BlockHammer가 다음과 같은 장점을 제공함을 입증했습니다:

낮은 오버헤드:

RowHammer 공격이 없는 환경에서, BlockHammer의 성능 및 에너지 오버헤드는 무시할 수 있을 정도로 낮습니다. 이는 RowHammer 임계값(NRH)이 매우 낮은 1K일 때에도 유효합니다.
높은 확장성:

BlockHammer는 RowHammer 공격이 있는 경우, 기존의 최첨단 메커니즘보다 훨씬 높은 성능과 낮은 에너지 소비를 제공합니다.
이는 다양한 DRAM 환경과 공격 시나리오에서 효과적임을 나타냅니다.
결정론적 보호:

BlockHammer는 모든 RowHammer 비트 플립 가능성을 완벽히 방지하는 포괄적인 보호를 제공합니다.
이를 통해, DRAM 보안이 요구되는 현대 시스템에서 신뢰할 수 있는 방어 메커니즘으로 자리잡을 수 있습니다.
상용 DRAM 호환성:

BlockHammer는 추가 하드웨어나 특별한 DRAM 설계 변경 없이 기존 DRAM 시스템에 쉽게 통합될 수 있습니다.
