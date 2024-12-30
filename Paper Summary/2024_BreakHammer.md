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