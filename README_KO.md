# NUM GAME

CASIO **fx-CG50**용 오프라인 네이티브 숫자 게임 **36종**입니다. C와 fxSDK/gint로 실행하며, Python은 PC에서 문제 생성·검증할 때만 필요합니다.

**실험적 사전 배포판입니다.** 호스트 검증과 SH 빌드는 통과했지만 실제 fx-CG50 시험은 남아 있습니다. 이전에 보고된 MENU 깜빡임은 실기에서 재현·원인 확인이 되지 않았습니다. [실기 확인 절차](docs/HARDWARE_RETEST.md)를 참고하세요.

[NUMGAME.g3a](dist/NUMGAME.g3a)를 계산기 USB 저장소에 복사하고 안전하게 연결 해제한 뒤 **NUM GAME**을 엽니다. [SHA256SUMS.txt](dist/SHA256SUMS.txt)로 확인할 수 있습니다. [NUMGDIAG.g3a](dist/NUMGDIAG.g3a)는 별도 계측용 **NUM DIAG** 앱이며 일반판 진행 데이터를 건드리지 않습니다. 계산기에 별도 문제 파일은 필요하지 않습니다.

![메인 메뉴](docs/captures/00-main.png)

| 분류 | 게임 6종 |
|---|---|
| GUESS | Number Baseball, Equation Guess, Number Mind, Clue Lock, Sequence, Black Box |
| CALC | Make Target, Countdown, Missing Operators, Cross Math, Prime Factor, Cryptarithm |
| LOGIC | Sudoku, Calcudoku, Kakuro, Futoshiki, Skyscrapers, Hashi |
| PUZZLE | Hitori, Binary, Numbrix, Magic Square, Sum Grid, Nonogram |
| STRATEGY | Nim, Wythoff, Euclid, Make Fifteen, Race to Target, Reversi |
| BOARD | 2048, Sliding, Lights Out, Shikaku, Slitherlink, Net |

새 게임 6종도 각각 전용 아이콘과 규칙, 플레이·저장 기능을 갖습니다. 모든 게임에 EASY/NORMAL/HARD/MASTER가 있고, LOGIC 6종에는 HELL도 있습니다. 난이도는 기존의 사람 기준 평가가 아닌 규칙·문제 생성 구조 또는 독립 검증된 풀이 지표를 기준으로 구분합니다. [사용 안내](docs/USER_GUIDE.md), [게임 목록](docs/GAME_CATALOG.md), [실제 앱 규칙](docs/RULES.md), [화면 모음](docs/captures/README.md)을 확인할 수 있습니다.

메인·카테고리 화면에서 방향키 또는 숫자 1–6으로 이동하고 EXE/F6 OPEN으로 엽니다. 미완료 기록이 있는 게임 타일 하나에 작은 RESUME 표시가 붙고 메인 F1 RESUME으로 바로 열 수 있습니다. 게임 직전 화면에서는 UP/DOWN으로 **NEW GAME**, 난이도, 필요한 설정을 고릅니다. 같은 게임에 미완료 기록이 있으면 RESUME 행이 맨 위에 나타나고 기본 선택됩니다. LEFT/RIGHT로 설정을 바꾸며 하단에 항목 설명이 표시됩니다. F5 RULES, F6 OPEN을 사용합니다. STATS와 별도 MODE 선택창은 없습니다. LOGIC에서 F3는 HELL 진입/복귀이며 HELL일 때 색으로 구분된 ENHM 안내가 표시됩니다. 전략 게임은 FIRST 행에서 YOU/CPU만 선택합니다.

Number Baseball은 반복 숫자를 허용하며 EASY/NORMAL/HARD/MASTER가 각각 4/5/6/7자리, **총 20/30/40/50회**입니다. 마지막 시도 전에는 ONE LAST TRY가 한 번 표시되고 전체 시도 기록을 스크롤할 수 있습니다. Make Target은 난이도별로 카드 4/4/5/6장을 모두 사용합니다. TARGET 행에서 좌우 방향키로 **10, 24, 50, 100, 200, RANDOM**을 순환 없이 선택하고 EXE 또는 F6 OPEN으로 시작합니다. 고정 목표마다 난이도별 문제 200개, RANDOM에서는 다섯 목표를 합친 1,000개에 접근합니다. Countdown은 카드를 일부 남겨도 되며 난이도별 문제 200개를 제공합니다. Prime Factor의 새 합성수 은행은 난이도 순서대로 3자리 / 3·4자리 / 4·5자리 / 5·6자리입니다. MAGIC SQUARE FREE는 난이도 순서대로 빈 3×3/4×4/5×5/6×6 보드입니다. NONOGRAM은 정답의 검은 칸만 정확히 칠하면 완료되며 나머지 빈칸은 그대로 두거나 X로 표시해도 됩니다. 2048은 작은 타일의 회색부터 연두·시안·파랑·노랑·주황·마젠타·빨강 계열로 색이 변합니다. [전체 조작표](docs/CONTROLS.md) · [36게임 콘텐츠 목록](docs/CONTENT_INVENTORY.md) · [난이도 감사](docs/DIFFICULTY_AUDIT.md).

beta.6에서는 Make Target·Countdown의 문제 수를 늘리면서 기존 카드 규칙과 정확한 풀이 난도 검증을 유지했습니다. Prime Factor를 자릿수와 소인수 구조로 재구성하고, Baseball의 시도 횟수·기록을 늘렸으며 긴 추측·단서 목록에 스크롤 위치를 표시합니다. NEW는 다음 문제를 선택하고 RESUME은 출제 순서를 소비하지 않고 현재 문제를 복원합니다. 이전 미완료 게임은 기존 문제 리비전을 유지합니다. [출제 순환](docs/CONTENT_CYCLE.md) · [카드 가독성](docs/MAKE_TARGET_READABILITY_AUDIT.md) · [화면 정렬](docs/UI_ALIGNMENT_AUDIT.md).

앱 전체에서 **미완료 게임 하나**만 이어할 수 있습니다. NEW GAME은 새 게임을 안전하게 저장한 뒤 기존 이어하기를 확인창 없이 교체합니다. 완료 후에는 이어하기가 사라지고, 완료 창에서 EXIT을 누르면 최종 화면을 수정할 수 없는 상태로 볼 수 있습니다. 여기서 EXIT은 게임 시작 화면으로, F6 NEW는 다음 판으로 갑니다. `NGSTATEA/B.dat` 두 파일은 하나의 논리 저장을 위한 복구 사본입니다. 빈 파일 둘은 총 252바이트, 새 게임 저장 둘은 총 3,996바이트입니다. 기존 최근 5개·구형 아카이브에서는 가장 최근의 유효한 미완료 게임 하나와 설정을 이관합니다. [저장 형식](docs/STORAGE_FORMAT.md), [이어하기 정책](docs/RESUME_POLICY.md), [문제 순환](docs/CONTENT_CYCLE.md).

일반판과 진단판은 `NG`/`ND` 저장 파일을 따로 사용합니다. 진단판은 Main→F2 SET→F3 DIAG에서 RAM/시간 계측, 1,000회 stress, `NDDIAG.txt` 내보내기를 제공합니다. 실기 지연·메모리 최대치는 아직 **HARDWARE TEST REQUIRED**입니다. [beta.6 호스트 측정](docs/PERFORMANCE_BETA6.md) · [진단 안내](docs/DIAGNOSTICS.md).

빌드에는 설치된 fxSDK/gint 2.11, SH GCC 14.1, CMake, fxgxa, Python 3을 사용합니다. SDK 위치가 다르면 `NUMGAME_SDK_ROOT`를 설정하세요.

```sh
bash tools/test.sh
bash tools/verify_content.sh
bash tools/clean_build.sh
bash tools/clean_build.sh --diagnostic
python3 tools/captures.py
```

macOS의 현재 ASan 실행 환경은 최소 프로그램에서도 `main` 전에 멈추므로 **ASan은 미검증**입니다. UBSan·호스트 mock·SH 빌드는 실기 시험을 대체하지 않습니다. 자체 코드·문제·아이콘은 [MIT](LICENSE)이고 글꼴·런타임·재사용 코드 고지는 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)에 있습니다.
