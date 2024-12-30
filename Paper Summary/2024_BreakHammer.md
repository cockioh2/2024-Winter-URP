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


## IV. BlockHammer

### IV-1. RowBlocker

### IV-2. AttackThrottler

## V. Many-Sided RowHammer Attacks

## VI. Security Analysis

## VII. Hardware Complexity Analysis

## VIII.Experimental Methodology

## IX. Performance and Energy Evaluation

## X. Comparison of Mitigation Mechanisms

## XI. Conclusion