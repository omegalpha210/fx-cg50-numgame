# NUM GAME

CASIO **fx-CG50**에서 오프라인으로 실행하는 네이티브 숫자 게임30종입니다.
C와 fxSDK/gint를 사용하며, Python은 PC의 문제 생성·독립 검증에만 사용합니다.

**실험적 사전 배포판입니다.** 호스트·네이티브 빌드 검증을 통과했으나 실제 fx-CG50
확인은 남아 있습니다. 배포 파일의 체크섬은 `dist`에 있습니다.

**실험판: 실기 검증이 남아 있습니다.** 보고된 MENU 깜빡임은 실기 미재현·원인
미확정입니다. 별도로 수정한 입력 경계 결함을 이 현상의 완치라고 주장하지 않습니다.
[실기 확인 절차](docs/HARDWARE_RETEST.md) · [진단 사용 안내](docs/DIAGNOSTICS.md).

[NUMGAME.g3a](dist/NUMGAME.g3a)를 USB 저장소에 복사하고 안전하게 연결을 해제한 뒤
CASIO 메뉴의 **NUM GAME**을 엽니다. [SHA256SUMS.txt](dist/SHA256SUMS.txt)로 확인합니다.
[NUMGDIAG.g3a](dist/NUMGDIAG.g3a)는 별도 **NUM DIAG** 앱이며, 측정 코드와 ND 저장 공간을
사용합니다. 일반판의 NG 진행 파일을 이관하거나 덮어쓰지 않습니다. 계산기에는 별도
문제 파일이나 Python을 설치하지 않습니다.

![메인 메뉴](docs/captures/00-main.png)

## 게임과 난이도

| 분류 | 게임5종 |
|---|---|
| GUESS | Number Baseball, Equation Guess, Number Mind, Clue Lock, Sequence |
| CALC | Make Target, Countdown, Missing Operators, Cross Math, Prime Factor |
| LOGIC | Sudoku, Calcudoku, Kakuro, Futoshiki, Skyscrapers |
| PUZZLE | Hitori, Binary, Numbrix, Magic Square, Sum Grid |
| STRATEGY | Nim, Wythoff, Euclid, Make Fifteen, Race to Target |
| BOARD | 2048, Sliding, Lights Out, Shikaku, Slitherlink |

MASTER는 코드·등식 길이, 산술 제약, 논리 추론, 검증된 전략 시작 포지션, 8192 목표,
어려운 Sliding/Lights Out 시작 상태 등 실제 도전 내용을 바꿉니다. 이미 최선의 수를
두는 전략 HARD CPU는 MASTER에서도 같은 exact AI를 사용합니다. 더 강한 완전 AI를
새로 만들었다고 표현하지 않습니다. Magic MASTER는4×4 panmagic이며 여러 유효 답을
허용합니다. 사람 기준 난이도는 잠정이며, 기계 검증 지표와 한계를 따로 기록합니다.

LOGIC5종은 일반 EASY/NORMAL/HARD/MASTER 외에 별도 **HELL**을 제공합니다.
MASTER는 기존 저장값3, HELL은 새 값4입니다. 각 게임은 실제 내장 문제를 일반 단계별
50개, HELL30개씩 갖습니다.
[콘텐츠 감사표](docs/CONTENT_INVENTORY.md)는 게임별 변경 전후 실제 내장 수량,
중복·변형 기준, 모드, 생성 함수, 데이터 경로·해시, 라이선스와 메모리를 기록합니다.
외부에서 받은 문제 파일·파싱한 문제·번들한 외부 문제는 모두0입니다. 규칙 참고 문헌,
허가된 글꼴과 런타임 코드는 문제 데이터 수량과 구분합니다.

저렴한 생성은 기기에서, 고비용 유일성·난이도 검증은 PC에서 수행하는 혼합 정책입니다.
논리 퍼즐은 검증된 압축 은행을 사용하고 한 문제만 해제합니다. 저장되는 작은 순회
상태로 은행 내 반복을 피하며, 다음 순회 첫 문제도 직전 문제와 같지 않게 합니다.
Magic 부분 단서·대칭 변형·규칙 게임·전략 포지션을 독립 퍼즐 수로 부풀리지 않습니다.
[전체 목록](docs/GAME_CATALOG.md) · [실제 앱 규칙](docs/RULES.md).

## 조작과 화면

메인·카테고리는2열×3행입니다. 숫자1–6 또는 방향키와 EXE/F6 OPEN을 사용합니다.
게임 직전 화면의 기본 선택은 NEW GAME입니다. 숫자는 행으로 이동만 하고,
UP/DOWN으로 행 선택, LEFT/RIGHT로 설정을 바꿉니다. 양끝에서는 멈춥니다.
EXE/F6 OPEN은 NEW·설정 행에서 새 게임을 시작하고, RESUME 행에서만 저장된 설정
그대로 재개합니다. 기존 진행 교체는 확인창을 거칩니다. F4 STATS/F5 RULES는 게임을
시작하지 않습니다.

LOGIC 난이도 행에서만 빨강·흰색 **F3 HELL**이 보입니다. 누르면 그 행에 빨간 HELL
하나만 표시됩니다. F3를 다시 누르면 직전 일반 단계, LEFT는 MASTER, RIGHT는 그대로입니다.
일반 MASTER에서 RIGHT만 눌러 HELL에 들어가지 않습니다. 다른 행으로 이동해도 선택은
유지됩니다. MASTER를 HELL로 줄여 쓰는 경로는 제거했습니다.

짧은 MODE는 한 줄에 표시합니다. 길거나 많은 모드는 해당 행의 **F3 MODE**로 작은
선택창을 엽니다. 방향키로 선택하고 EXE/F6 OK로 설정을 확정해 진입 화면으로 돌아오며,
EXIT는 취소합니다. 선택창의 EXE로 곧바로 게임을 실행하지 않습니다.

플레이 중 F1 INIT은 같은 문제/게임을 다시 시작하고, F2 UNDO는 지원되는 기록이 있을
때만 표시합니다. F3 HINT/REVEAL은 명시된 도움, F4는 게임별 NOTES/CHECK/ANSWER,
F5 RULES, F6는 화면의 기본 동작입니다. INIT·undo·힌트·공개 답은 assisted 기록입니다.
통계는 게임/모드/난이도/assisted를 분리합니다. 2048 CLASSIC 규칙은 유지하며 TARGET은
512/1024/2048/8192 목표와 별도 통계를 사용합니다.

**SHIFT+DOT은 `=`**, DOT 단독은 `.`, 거듭제곱 키는 `^`, 음수 부호 키는 `-`입니다.
EXIT는 저장 후 진입 화면으로 돌아가되 Shikaku 모서리 선택 중에는 그 선택만 취소합니다.
MENU와 SHIFT+AC/ON은 저장을 시도하고 실제 OS 메뉴/전원 기능을 호출합니다.
일반 AC/ON은 종료하지 않습니다. [전체 조작표](docs/CONTROLS.md).

일반 글꼴은 DIFF EQ와 같은 native gint8×9입니다.30개 게임 아이콘은 작은 선·도형으로
작성했습니다.2048 색은 회색→연두→시안→파랑→노랑→주황→마젠타→빨강→짙은 빨강입니다.
[화면 모음](docs/captures/README.md)은 실제 C 렌더러의 호스트 RGB565 출력이며 실기 사진은 아닙니다.

## 저장·시간·진단

일반 저장은 **NGARCA.dat/NGARCB.dat 두 개**, 각각462,032바이트, 합계924,064바이트입니다.
현재 게임의 작은 record만 읽고 쓰며 archive 전체를 RAM에 올리지 않습니다. 새 record
형식v3는 기존3·4단계 저장을 읽고 문제 ID·진행·통계·MASTER 의미를 유지합니다.
기존 자료 이관은 두 새 사본 검증 후에만 소유권이 확인된 옛 파일을 정리합니다.
충돌·손상·소유권 불명 자료는 보존합니다. 두 archive와 남은 옛 파일을 함께 백업하세요.
[저장 형식](docs/STORAGE_FORMAT.md) · [복구·이관 한계](docs/STORAGE_ARCHIVE_AUDIT.md).

공통 타이머 하나로 입력 없이도 플레이 시간이 증가합니다. 진입·규칙·모달·OS 밖·OFF
시간은 제외합니다. dim/APO는 실제 사용자 입력 기준이며 타이머 tick으로 연장되지
않습니다. 지원 OS 설정을 읽고, 미지원이면60초 dim/10분 APO를 사용합니다.

NUM DIAG의 Main→F2 SET→F3 DIAG에서 Memory/Runtime, RESET,1,000회 제한 RAM stress,
고정 `NDDIAG.txt` 내보내기를 제공합니다. fixture는 사용자 진행 파일을 덮어쓰지 않습니다.
앱 함수 진입 SP 표본, 두 gint arena, codec 사용량, handle·timer·동작 시간을 구분해 봅니다.
**실제 계산기 메모리 peak: NOT MEASURED.** 호스트 ABI·속도·표본을 SH 수치로 바꾸지 않으며,
heap에 포함된 VRAM이나 BSS 작업 공간을 다시 합산하지 않습니다.
[측정 방법](docs/MEMORY_METHOD.md) · [정적 분석](docs/MEMORY_AUDIT.md) ·
[호스트 성능](docs/PERFORMANCE_AUDIT.md) · [진단 절차](docs/DIAGNOSTICS.md).

## 빌드·검증

기존 fxSDK/gint2.11, SH GCC14.1, CMake, fxgxa, Python3을 사용합니다.
SDK 위치가 다르면 `NUMGAME_SDK_ROOT`를 설정합니다. SDK를 재설치하거나 수정하지 않습니다.

```sh
bash tools/test.sh
bash tools/verify_content.sh
bash tools/clean_build.sh
bash tools/clean_build.sh --diagnostic
source tools/env.sh
python3 tools/captures.py
python3 tools/memory_report.py --build-dir <native-build> --g3a dist/NUMGAME.g3a
```

별도 호스트 CMake 디렉터리에 `-DNG_DIAGNOSTIC=ON`을 지정하면 진단 회귀와
`benchmark_host`를 실행할 수 있습니다. 정상 동작하는 환경에서는 `-DNG_ASAN=ON`으로
ASan을 재시험합니다. 현재 macOS의 ASan 런타임은 최소 프로그램도 main 전에 멈추므로
**ASan 미검증**입니다. UBSan·native mock·SH 빌드 통과가 실기 시험을 대신하지 않습니다.
일반판 목표는1,000,000바이트, 절대 상한은 압축 전 실제 G3A의1,200,000바이트입니다.

자체 코드·아이콘·문제는 [MIT](LICENSE)이며 글꼴·런타임·재사용 기반 코드의 고지는
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)와 [출처](docs/ASSET_PROVENANCE.md)에 있습니다.
사용자 진행, SDK 설치본, 개인 문서와 상업용 문제집 데이터는 공개 소스에 필요하지 않습니다.
