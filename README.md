# HEAT
1. 기본 구현 (배점 50%)
    - 지정된 명령어를 실행하여 EXIT CODE를 통해 성공/실패 여부 처리
    - 사용 예시 (1)
        - 30초 주기로 CURL 명령어 실행하기 (SIGALARM 참고)
            ```shell
            $ heat -i 30 curl -sf localhost/health_check
            ```
        - CURL 명령어에서 -f 옵션은 Resposne Status Code 가 200이 아니면, 0 이 아닌 exit code를 반환한다.
    - 사용 예시 (2)
        - 쉘 스크립트 지정하여 사용하기
            ```shell
            $ heat -i 30 -s ./check
            ```
        - 쉘 스크립트는 executable 해야 한다. 그렇지 않으면 실패 처리해야 한다.
        - 검사 스크립트와 검사 명령어가 모두 지정되거나 그렇지 않은 경우 에러 처리해야 한다.
2. 옵션 1 (배점 10%)
    - 실패를 감지할 경우 지정된 프로세스에 시그널 보내기
        ```shell
        $ heat -i 30 -s ./check --pid 1234 --signal USR1
        ```
    - --pid 에서 지정한 프로세스에 시그널 보내기
        - 실행시 해당 프로세스가 없으면 에러 처리 후 종료
    - --signal 을 통해 전달할 시그널 정하기, 생략하는 경우 SIGHUP 전달
3. 옵션 2 (배점 10%)
    - 실패를 감지할 경우 지정된 스크립트 실행하기
        ```shell
        $ heat -i 30 -s ./check --fail ./failure.sh
        ```
    - --fail 에서 지정한 스크립트 실행하기
        - 해당 스크립트가 실행 중에 다음 인터벌이 도달하는 경우
        - (1) 다음 인터벌을 수행하지 않음
        - (2) 다음 인터벌을 수행, 현재 수행 중인 failure.sh 가 실행이 종료되면 이어서 바로 실행하기
            - 단 여럿의 인터벌에 의해 다수 누적되더라도 한 번만 실행하기 (기존 누적횟수 초기화)
    - 실패 스크립트를 실행할 때,
        ```shell
        $ heat -i 30 -s ./check --fail ./failure.sh
        ```
    - 환경 변수 HEAT_FAIL_CODE에 exit code를 전달
    - 환경 변수 HEAT_FAIL_TIME에 발생시각(unixtime)을 전달
    - 환경 변수 HEAT_FAIL_INTERVAL 에 인터벌 주기를 전달
    - 환경 변수 HEAT_FAIL_PID 에 실행한 ./check의 PID를 전달
4. 옵션 3 (배점 20%)
    - 실패가 누적될 경우 recovery 스크립트 지정하기
        ```shell
        $ heat -i 30 -s ./check --recovery ./recovery.sh --threshold 10 --recovery-timeout 300
        ```
    - --threshold 에서 지정한 만큼 연속 실패할 경우
        - --recovery 에서 지정한 스크립트 실행하기
            - 이 스크립트는 executable 해야 함. 프로그램 시작 시 검사하여 예외 처리해야 함
            - recovery 중에는 --fail 스크립트 실행하지 않아야 함 (옵션 2 구현시)
    - recovery 스크립트 실행 후
        - 검사 스크립트 혹은 명령어를 수행하여 상태를 확인한다.
            - --recovery-timeout 에 지정된 시간 안에 (없으면 --threshold 에서 지정한 횟수 동안)
                - 검사를 지정된 간격으로 계속 수행하고, fail 횟수만 누적
            - 복구되지 않고 지정된 시간이 지나면
                - fail 횟수를 기존에서 누적하고, 다시 recovery 를 호출한다.
            - 검사 결과가 성공이 되면
                - 연속 누적 fail 횟수를 초기화하고 정상 모드로 진입
    - recovery 스크립트 실행
        - 환경 변수 HEAT_FAIL_CODE 에 exit code를 전달
        - 환경 변수 HEAT_FAIL_TIME 에 최초 발생시각(unixtime)을 전달
        - 환경 변수 HEAT_FAIL_TIME_LAST 에 최근 발생시각(unixtime)을 전달
        - 환경 변수 HEAT_FAIL_INTERVAL 에 인터벌 주기를 전달
        - 환경 변수 HEAT_FAIL_PID 에 실행한 ./check 의 PID를 전달
        - 환경 변수 HEAT_FAIL_CNT 에 현재까지 연속 실패한 횟수를 전달
5. 옵션 4 (배점 10%)
    - 실패가 누적될 경우 지정된 시그널 보내기
        ```shell
        $ heat -i 30 -s ./check --pid 1234 --threshold 10 --fault-signal STOP --success-signal CONT
        ```
        - --pid 에서 지정한 프로세스에 시그널 보내기
            - 옵션 3의 조건에 복구에 해당하는 경우 fault-signal을 보낸다.
            - 옵션 3의 복구 성공에 해당하는 경우 success-signal을 보낸다.
6. 옵션 5 (배점 10%)
    - 검사 스크립트 혹은 검사 명령어, 복구 명령어 및 시그널 수신 프로세스가 중단되면 즉시 에러를 출력하고 종료해야 한다.
7. 옵션 6 (배점 10%)
    - 검사 명령어, 검사 스크립트, 복구 스크립트의 출력은 모두 heat.verbose.log에 기록되어야 한다. (기존의 기록에 이어서 기록)
    - 형식
        - 시간(초): 타입 : 명령/프로그램 : 
        - 로그 타입
            - INFO: 일반 정보
            - FAIL: 실패 정보
            - ERR: 처리할 수 없는 실패/에러 정보
