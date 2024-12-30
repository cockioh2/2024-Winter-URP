# Chapter 11 Summary

## Table of Contents

- [Chapter 11 Summary](#chapter-11-summary)
  - [Table of Contents](#table-of-contents)
  - [Basic DRAM Commands](#basic-dram-commands)
    - [1. Generic DRAM Command Format](#1-generic-dram-command-format)
    - [2. Timing Parameter Summary](#2-timing-parameter-summary)
    - [3. Row Access Command](#3-row-access-command)
    - [4. Column Read Command](#4-column-read-command)
    - [5. Column Write Command](#5-column-write-command)
    - [6. Precharge Command](#6-precharge-command)
    - [7. Refresh Command](#7-refresh-command)
    - [8. Read Cycle](#8-read-cycle)
    - [9. Write Cycle](#9-write-cycle)
    - [10. Compound Commands](#10-compound-commands)
  - [DRAM Command Interactions](#dram-command-interactions)
    - [1. Consecutive Reads and Writes to Same Rank](#1-consecutive-reads-and-writes-to-same-rank)
    - [2. Read to Precharge Timing](#2-read-to-precharge-timing)
    - [3. Consecutive Reads to Different Rows of Same Rank](#3-consecutive-reads-to-different-rows-of-same-rank)
      - [Best Scenario](#best-scenario)
      - [Worst Scenario](#worst-scenario)
    - [4. Consecutive Reads to Different Banks: Bank Conflict](#4-consecutive-reads-to-different-banks-bank-conflict)
      - [Without Command Reordering](#without-command-reordering)
      - [With Command Reordering](#with-command-reordering)
    - [5. Consecutive Read Requests to Different Ranks](#5-consecutive-read-requests-to-different-ranks)
    - [6. Consecutive Write Requests: Open Banks](#6-consecutive-write-requests-open-banks)
    - [7. Consecutive Write Requests: Bank Conflicts](#7-consecutive-write-requests-bank-conflicts)
    - [8. Write Request Following Read Request: Open Banks](#8-write-request-following-read-request-open-banks)
    - [9. Write Request Following Read Request to Different Banks, Bank Conflict, Best Case, No Reordering](#9-write-request-following-read-request-to-different-banks-bank-conflict-best-case-no-reordering)
    - [10. Read Following Write to Same Rank, Open Banks](#10-read-following-write-to-same-rank-open-banks)
    - [11. Write to Precharge Timing](#11-write-to-precharge-timing)
    - [12. Read Following Write to Different Ranks, Open Banks](#12-read-following-write-to-different-ranks-open-banks)
    - [13. Read Following Write to Same Bank, Bank Conflict](#13-read-following-write-to-same-bank-bank-conflict)
    - [14. Read Following Write to Different Banks of Same Rank, Bank Conflict, Best Case, No Reordering](#14-read-following-write-to-different-banks-of-same-rank-bank-conflict-best-case-no-reordering)
    - [15. Column-Read-and-Precharge Command Timing](#15-column-read-and-precharge-command-timing)
    - [16. Column-Write-and-Precharge Timing](#16-column-write-and-precharge-timing)
  - [Additional Constraints](#additional-constraints)
    - [1. Device Power Limit](#1-device-power-limit)
    - [2. tRRD: Row-to-Row (Activation) Delay](#2-trrd-row-to-row-activation-delay)
    - [3. tFAW: Four-Bank Activation Window](#3-tfaw-four-bank-activation-window)
    - [4. 2T Command Timing in Unbuffered Memory System](#4-2t-command-timing-in-unbuffered-memory-system)

## [Basic DRAM Commands](#table-of-contents)

![image](https://github.com/user-attachments/assets/d68ce251-29c8-4614-a799-893e8088f95d)


### [1. Generic DRAM Command Format](#table-of-contents)

![Generic DRAM Command Format](images/DRAM_Generic_Format.png)

*t*<sub>CMD</sub>: DRAM Controller에서 보낸 CMD가 DRAM Device로 도착하는데 걸리는 시간이다.

*t*<sub>parameter1</sub>: 해당 CMD로 인해 특정 뱅크가 점유되는 시간. 각 뱅크마다 독립적으로 적용된다.

*t*<sub>parameter2</sub>: 해당 CMD로 인해 공유 자원이 점유되는 시간. 모든 뱅크가 공유하는 자원이기 때문에 각 뱅크가 공유하는 시간이다.

### [2. Timing Parameter Summary](#table-of-contents)

![Table_1_1](images/Timing_Param_Table_1.png)
![Table_1_2](images/Timing_Param_Table_2.png)

*t*<sub>AL</sub>: Column Access 시에 추가되는 Latency. Posted CAS 커맨드를 지원하는 DRAM Device에 대해 설정된 값에 기반하여 CAS 커맨드의의 동작을 지연시킨다. <br> --예시: 명령어 CAS#가 활성화되고 나서 2 클럭 주기 후에 데이터가 반환되도록 추가 딜레이(tAL)가 발생.
DDRx 메모리에서 데이터를 읽으려고 할 때 컨트롤러가 일부 데이터 버퍼를 준비할 시간을 확보한다.

*t*<sub>BURST</sub>: Data Burst 시간. 데이터를 데이터 버스를 통해 전달하는 시간이다. 통상적으로 4 또는 8 beats를 전달하며, 4 beats는 clock cycle 2개를 차지한다 (rise 2번, fall 2번번). <br> --예시:
DDR4 SDRAM에서 데이터를 연속적으로 읽을 때, 8개의 비트(1바이트)가 데이터 버스를 통해 전송된다. 이 과정은 4 클럭 사이클이 걸린다.
만약 CPU가 메모리에서 "128비트"를 읽고자 한다면, 16개의 비트 단위 전송(8비트씩 2회)이 이루어지며 총 2개의 tBURST 사이클이 필요.


*t*<sub>CAS</sub>: Column Access Strobe Latency. Columnn Read 커맨드 입력 이후에 DRAM device로부터 요청한 데이터를 데이터 버스에 배치하기까지 걸리는 시간이다. <br> --예시:
tCAS가 12 클럭 주기라면, 열(column)을 지정한 후 정확히 12 클럭 뒤에 데이터가 메모리 버스에 출력된다.
 = CPU가 특정 열의 데이터를 요청했을 때, 12 클럭 동안 기다린 후 데이터가 준비된다.

*t*<sub>CCD</sub>: Column-to-Column Delay. 최소 버스트 기간 혹은 최소 column-to-column 커맨드 타이밍을 의미한다. DRAM device의 Prefetch 길이에 의해 결정된다. 만약 DDR SDRAM device의 Prefetch 길이가 2 beats라면 *t*<sub>CCD</sub>는 1 full-cycle이다. <br> --예시:
DRAM에서 첫 번째 열을 읽은 후 두 번째 열을 읽으려면 2 클럭(tCCD) 간 대기한다.
연속적으로 읽기를 요청하면 메모리 컨트롤러는 이 제한 시간을 계산해 대기.

*t*<sub>CMD</sub>: Command Transport duration. 명령 버스(cmd & addr bus)가 점유되는 시간이다.<br> --예시:
메모리 컨트롤러가 활성화 명령(Activate)을 DRAM으로 보냈을 때, 이 명령이 도달하고 실행될 때까지 tCMD 주기가 소요됨.

*t*<sub>CWD</sub>: Column Write Delay. Column Write 커맨드가 버스로 입력된 시점부터 Write Data가 Data Bus에 위치하는데까지 걸리는 시간이다 (cmd -> data burst).<br> --예시:
쓰기 명령이 실행되고 나서 데이터 버스에 데이터가 도달하기까지 8 클럭(tCWD)이 걸림. (데이터 버스의 충돌 방지 및 데이터 안정성을 보장하기 위한 시간)

*t*<sub>FAW</sub>: Four bank Activation Window. 동일한 DRAM 디바이스의 최대 네 개의 로우 활성화가 동시에 진행되는 경우의 구체적인 롤링 시간 구성의 정의. Row Activation은 적어도 RRD time만큼 떨어져 있어야 하며, 다섯 번째 Row Activation이 첫 번째 Row Activation이 시작된 시점으로부터 적어도 FAW time만큼 떨어져 있어야 한다. 이는 더 큰 DDR2 SDRAM 디바이스의 전류 소모량의 증가를 제한하기 위해서이다.<br>--예시:
특정 DRAM 칩이 4개의 뱅크를 동시에 활성화하려고 할 때, 40ns(tFAW)의 제한이 있음. 이 제한을 초과하려면 각 활성화가 순차적으로 이루어져야 함.

*t*<sub>OST</sub>: ODT Switching Time. 랭크간 ODT(On-Die Termination) 제어 전환에 필요한 시간이다. 다른 랭크로의 연속적인 column write command에서 BURST time 사이에 추가되는 시간이다.<br>--예시:
랭크 0에서 데이터 쓰기가 끝난 후, 랭크 1로 전환하려면 10ns의 tOST 시간이 필요함.

*t*<sub>RAS</sub>: Row Access Strobe Latency. DRAM cell의 Row Data의 방출과 Refresh에 걸리는 시간을 의미한다.<br>--예시:
특정 행이 활성화된 후 데이터를 읽거나 쓰기 전에 반드시 30ns(tRAS)가 지나야 함. 그렇지 않으면 데이터 무결성이 깨질 수 있음.

*t*<sub>RC</sub>: Row Cycle. RAS time + Row Precharge(RP) time. <br>--예시:
행 A가 활성화된 후 행 B를 활성화하기 전에 tRC(예: 55ns)가 필요함. 이는 tRAS와 tRP를 더한 값.

*t*<sub>RCD</sub>: Row to Column command delay. Row Access 커맨드 이후로, DRAM cell array의 데이터를 S/A로 옮기는 데 걸리는 시간이다.<br>--예시:
행(row)을 활성화하고, 열(column)을 읽으려면 15ns(tRCD)의 대기 시간이 필요.

*t*<sub>RFC</sub>: Refresh Cycle Time. 모든 뱅크로 하나의 Refresh 커맨드를 전송하고 완료되기까지 걸리는 시간이다.<br>--예시:
DRAM이 리프레시 명령(refresh)을 완료하고 새로운 읽기 명령을 받기 전에 300ns(tRFC)의 대기 시간이 필요.

*t*<sub>RP</sub>: Row Precharge. Precharge 커맨드 입력 이후에 선택된 뱅크의 Bit Line과 S/A가 Precharge되는데 걸리는 시간이다.<br>--예시:
행(row) A에서 데이터를 읽은 후, 행 B를 활성화하려면 15ns(tRP)가 필요.


*t*<sub>RRD</sub>: Row Activation to Row Activation Delay. 같은 device의 두 Row를 연속적으로 활성화할 때 기다려야하는 최소 시간이다. 이는 FAW time과 같이 전류 소모를 제한하기 위해 설정되었다.<br>--예시:
DRAM의 뱅크 A를 활성화한 후 뱅크 B를 활성화하려면 최소 5ns(tRRD)가 필요함.

*t*<sub>RTP</sub>: Read to Precharge. 본질적으로 RTP time은은, 자체적으로 Column Read 커맨드와 Precharge 커맨드 사이에 필요한 최소시간을 의미한다. 그러나 일부 DRAM device에서는 약간 다르게 표현되기도 한다. 자세한 내용은 [11.2.2. Read to Precharge Timing](#2-read-to-precharge-timing) 참고 <br> --예시:
특정 데이터를 읽은 후 DRAM을 준비 상태로 되돌리려면 7.5ns(tRTP)가 필요함.


*t*<sub>RTRS</sub>: Rank-to-rank switching time. DDR과 DDR2 SDRAM에서 사용되며, SDRAM이나 Direct RDRAM에서는 사용되지 않는다. 다른 랭크에 대한 연속적인 Read 커맨드에 대한 스위칭 시간이다.<br>--예시:
랭크 0에서 읽기 명령이 완료된 후, 랭크 1에서 쓰기 명령을 실행하려면 10ns(tRTRS)가 필요함.

*t*<sub>WR</sub>: Write Recovery time. Write 데이터가 DRAM Array로 전파되는 데 걸리는 시간으로, Write 커맨드 직후 Precharge 커맨드를 전송하기 위해 대기해야하는 최소시간이다.<br>--예시:
데이터를 DRAM에 쓰기 완료한 후, 데이터 안정성을 보장하기 위해 15ns(tWR)가 필요함.

*t*<sub>WTR</sub>: Write-to-Read Turnaround delay. I/O 게이팅 자원들이 Write 커맨드에 의하여 방출되는 데 걸리는 시간으로, Write 커맨드 직후 Read 커맨드를 전송하기 위해 대기해야 하는 최소시간이다.<br> --예시:
특정 열(column)에 데이터를 쓴 후 다른 데이터를 읽으려면 10ns(tWTR)가 필요함.


### [3. Row Access Command](#table-of-contents)

관련된 time 요소: *t*<sub>RCD</sub>, *t*<sub>RAS</sub>

![Row Access](images/Row_Access.png)

Row Access Command, 또는 Row Activation Command를 통해 DRAM Array에 있는 데이터를 S/A로 이동시키고, DRAM Array에 데이터를 Restore한다.
Command 입력 후 *t*<sub>RCD</sub> 이후에 활성화된 데이터의 모든 Row가 S/A와 연결된다. 그 후 Column Read나 Write를 통해 S/A와 메모리 컨트롤러간에 데이터를 주고 받는다. *t*<sub>RAS</sub>는 *t*<sub>RCD</sub> + 데이터 Restore 시간이다. *t*<sub>RAS</sub> 이후, S/A가 동일한 뱅크 내에 있는 다른 Row Access를 수행하기 위한 Precharge가 완료된다.

### [4. Column Read Command](#table-of-contents)

관련된 time 요소: *t*<sub>CAS</sub>, *t*<sub>BURST</sub>, *t*<sub>CCD</sub>

![Column_Read](images/Column_read.png)


- 전체 동작 과정<br>

1단계: Command & Address 전달
메모리 컨트롤러에서 Column Read 명령과 Column 주소를 DRAM으로 전달한다.
DRAM 내부의 디코더(Decoder)가 이 정보를 해석해 데이터를 읽을 준비를 시작한다.

2단계: 첫 번째 데이터 읽기 (tCAS의 시작)
DRAM은 활성화된 Row의 특정 Column 데이터를 센스 앰프에서 읽어 준비한다.
이 준비 과정이 끝난 후 데이터는 I/O 게이팅을 통해 데이터 버스로 전송된다.
tCAS (Column Access Strobe Latency)는 Read 명령이 입력된 후, 첫 번째 데이터가 데이터 버스에 나타날 때까지 걸리는 시간이다.

3단계: 연속 데이터 읽기 (tBURST)
DRAM은 한 번의 명령으로 데이터를 버스트(Burst) 단위로 전송한다.
첫 번째 데이터 이후, 두 번째 데이터가 같은 Row의 다음 Column에서 연속적으로 전송된다.
tBURST (Burst Duration)는 데이터를 연속적으로 데이터 버스로 전송하는 시간이다.

4단계: 데이터 전송 완료
준비된 데이터는 데이터 버스를 통해 메모리 컨트롤러로 전송된다.
이 과정은 tBURST 내에서 모든 데이터를 연속적으로 처리하며 종료된다.

- 주요 타이밍 파라미터<br>

1) tCAS (Column Access Strobe Latency)
tCAS는 칼럼 Read 명령 입력 후, 첫 번째 데이터가 데이터 버스에 나타날 때까지 걸리는 시간이다.
DRAM 내부에서 데이터를 준비하고 출력하는 데 필요한 초기 대기 시간이다.
예를 들어, tCAS가 10 클럭이라면, 명령 입력 후 10 클럭 동안 대기한 후 데이터가 버스에 출력된다.

2) tBURST (Burst Duration)
tBURST는 한 번의 Column Read 명령에서 데이터를 연속적으로 전송하는 시간이다.
DRAM은 데이터를 버스트 단위로 묶어서 전송한다.
예를 들어, tBURST가 4 클럭이라면, DRAM은 첫 번째 데이터 이후 4 클럭 동안 데이터를 연속적으로 전송한다.

3) tCCD (Column-to-Column Delay)
tCCD는 두 칼럼 명령 간에 필요한 최소 대기 시간이다.
DRAM 내부의 데이터 충돌을 방지하고, 칼럼 간 명령 간격을 유지한다.
예를 들어, tCCD가 2 클럭이라면, 첫 번째 칼럼 명령과 두 번째 칼럼 명령 간에는 최소 2 클럭의 대기 시간이 필요하다.



### [5. Column Write Command](#table-of-contents)

관련된 time 요소: *t*<sub>CWD</sub>, *t*<sub>BURST</sub>, *t*<sub>WTR</sub>, *t*<sub>WR</sub>


![image](https://github.com/user-attachments/assets/568264f7-238b-4594-a981-8155078c643a)

- 전체 동작 과정<br>

1단계: Command & Address 전달
메모리 컨트롤러에서 Column Write 명령과 Column 주소를 DRAM으로 전달한다.
DRAM은 이 명령을 해석하고 데이터를 받을 준비를 한다.<br>

2단계: tCWD 대기 후 데이터 전송 시작
Write 명령이 전달된 후, 데이터가 데이터 버스에 배치되기까지 tCWD(Column Write Delay)만큼 대기한다.
이후 메모리 컨트롤러는 데이터를 데이터 버스를 통해 DRAM으로 전송한다.<br>

3단계: 데이터 쓰기 (tBURST 구간)
DRAM은 데이터를 버스트 단위로 센스 앰프에 저장하고, DRAM 셀에 기록한다.
이 데이터 전송이 tBURST (Burst Duration) 동안 진행된다.<br>

4단계: 안정화 (tWR 대기)
데이터가 DRAM 셀에 안정적으로 기록되기 위해 tWR (Write Recovery) 동안 대기해야 한다.
tWR 시간이 끝나기 전에는 Precharge 명령을 실행할 수 없다. 
<br>

- 주요 타이밍 파라미터<br>

1) tCWD (Column Write Delay)
tCWD는 Write 명령이 DRAM으로 전달된 후, 메모리 컨트롤러가 데이터를 데이터 버스에 배치하기까지 걸리는 시간이다.
SDRAM: tCWD = 0 (명령과 동시에 데이터 전송 시작).
DDR: tCWD = 1 클럭.
DDR2: tCWD = tCAS - tCMD.<br>

2) tBURST (Burst Duration)
tBURST는 데이터를 버스트 단위로 연속적으로 데이터 버스를 통해 전송하는 시간이다.
이 시간 동안 DRAM은 데이터 전송을 안정적으로 처리한다.<br>

3) tWR (Write Recovery)
tWR은 DRAM 셀에 데이터를 안정적으로 쓰고 나서, Precharge 명령을 실행하기 전까지의 최소 대기 시간이다.
예를 들어, tWR이 10ns라면, Write 작업 후 최소 10ns 동안 대기한 후 Precharge 명령을 실행할 수 있다.<br>

4) tWTR (Write-to-Read Turnaround)
tWTR은 Write 작업 후 I/O 게이팅 자원을 초기화하고, Read 명령을 실행하기 위해 필요한 최소 대기 시간이다.
예를 들어, tWTR이 5 클럭이라면, Write 작업 후 최소 5 클럭 동안 대기한 후 Read 명령을 실행할 수 있다.<br>


### [6. Precharge Command](#table-of-contents)

관련된 time 요소: *t*<sub>RAS</sub>, *t*<sub>RP</sub>, *t*<sub>RC</sub>

### [7. Refresh Command](#table-of-contents)

관련된 time 요소: *t*<sub>RAS</sub>, *t*<sub>RP</sub>, *t*<sub>RC</sub>, *t*<sub>RFC</sub>

### [8. Read Cycle](#table-of-contents)

관련된 time 요소: *t*<sub>RAS</sub>, *t*<sub>RP</sub>, *t*<sub>RC</sub>, *t*<sub>RCD</sub>, *t*<sub>CAS</sub>, *t*<sub>BURST</sub>

### [9. Write Cycle](#table-of-contents)

관련된 time 요소: *t*<sub>RAS</sub>, *t*<sub>RP</sub>, *t*<sub>RC</sub>, *t*<sub>RCD</sub>, *t*<sub>CWD</sub>, *t*<sub>BURST</sub>, *t*<sub>WR</sub>

### [10. Compound Commands](#table-of-contents)

관련된 time 요소: *t*<sub>RAS</sub>, *t*<sub>RP</sub>, *t*<sub>RC</sub>, *t*<sub>RCD</sub>, *t*<sub>CAS</sub>, *t*<sub>BURST</sub>, *t*<sub>AL</sub>

---------

## [DRAM Command Interactions](#table-of-contents)

### [1. Consecutive Reads and Writes to Same Rank](#table-of-contents)

관련된 time 요소: *t*<sub>BURST</sub>, *t*<sub>CCD</sub>, *t*<sub>CAS</sub>

### [2. Read to Precharge Timing](#table-of-contents)

관련된 time 요소: *t*<sub>BURST</sub>, *t*<sub>CCD</sub>, *t*<sub>RTP</sub>

### [3. Consecutive Reads to Different Rows of Same Rank](#table-of-contents)

관련된 time 요소: *t*<sub>BURST</sub>, *t*<sub>CCD</sub>, *t*<sub>RTP</sub>, *t*<sub>RP</sub>, *t*<sub>RCD</sub>, *t*<sub>CAS</sub>

#### [Best Scenario](#table-of-contents)

#### [Worst Scenario](#table-of-contents)

### [4. Consecutive Reads to Different Banks: Bank Conflict](#table-of-contents)

관련된 time 요소: *t*<sub>RP</sub>, *t*<sub>RCD</sub>, *t*<sub>CMD</sub>

#### [Without Command Reordering](#table-of-contents)

#### [With Command Reordering](#table-of-contents)

### [5. Consecutive Read Requests to Different Ranks](#table-of-contents)

관련된 time 요소: *t*<sub>BURST</sub>, *t*<sub>RTRS</sub>, *t*<sub>CAS</sub>

### [6. Consecutive Write Requests: Open Banks](#table-of-contents)

관련된 time 요소: *t*<sub>BURST</sub>, *t*<sub>OST</sub>, *t*<sub>CWD</sub>

### [7. Consecutive Write Requests: Bank Conflicts](#table-of-contents)

관련된 time 요소: *t*<sub>BURST</sub>, *t*<sub>WR</sub>, *t*<sub>CWD</sub>, *t*<sub>RP</sub>, *t*<sub>RCD</sub>, *t*<sub>CMD</sub>

### [8. Write Request Following Read Request: Open Banks](#table-of-contents)

관련된 time 요소: *t*<sub>BURST</sub>, *t*<sub>CAS</sub>, *t*<sub>RTRS</sub>, *t*<sub>CWD</sub>

### [9. Write Request Following Read Request to Different Banks, Bank Conflict, Best Case, No Reordering](#table-of-contents)

### [10. Read Following Write to Same Rank, Open Banks](#table-of-contents)

### [11. Write to Precharge Timing](#table-of-contents)

### [12. Read Following Write to Different Ranks, Open Banks](#table-of-contents)

### [13. Read Following Write to Same Bank, Bank Conflict](#table-of-contents)

### [14. Read Following Write to Different Banks of Same Rank, Bank Conflict, Best Case, No Reordering](#table-of-contents)

### [15. Column-Read-and-Precharge Command Timing](#table-of-contents)

### [16. Column-Write-and-Precharge Timing](#table-of-contents)

-------

## [Additional Constraints](#table-of-contents)

### [1. Device Power Limit](#table-of-contents)

### [2. t<sub>RRD</sub>: Row-to-Row (Activation) Delay](#table-of-contents)

### [3. t<sub>FAW</sub>: Four-Bank Activation Window](#table-of-contents)

### [4. 2T Command Timing in Unbuffered Memory System](#table-of-contents)
