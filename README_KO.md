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

새 게임 6종도 각각 전용 아이콘과 규칙, 플레이·저장 기능을 갖습니다. 모든 게임에 EASY/NORMAL/HARD/MASTER가 있고, LOGIC 6종에는 HELL도 있습니다. 난이도는 기존의 사람 기준 평가가 아닌 규칙·문제 생성 구조 또는 독립 검증된 풀이 지표를 기준으로 구분합니다. [게임 목록](docs/GAME_CATALOG.md), [실제 앱 규칙](docs/RULES.md), [화면 모음](docs/captures/README.md)을 확인할 수 있습니다.

메인·카테고리 화면에서 방향키 또는 숫자 1–6으로 이동하고 EXE/F6 OPEN으로 엽니다. 게임 직전 화면에서는 UP/DOWN으로 NEW GAME, 저장된 게임, 난이도, 필요한 설정을 고르고 LEFT/RIGHT로 설정을 바꿉니다. 하단에는 선택한 항목의 짧은 설명이 표시됩니다. F6 OPEN은 선택한 행의 동작을 실행하고 F5 RULES는 규칙을 엽니다. STATS와 별도 MODE 선택창은 없습니다. LOGIC에서 F3는 HELL 진입/복귀이며 HELL일 때 색으로 구분된 ENHM 안내가 표시됩니다. 전략 게임은 FIRST 행에서 YOU/CPU만 선택합니다.

Number Baseball은 반복 숫자를 허용하며 EASY/NORMAL/HARD/MASTER가 각각 4/5/6/7자리입니다. Make Target은 목표를 **1–1000**에서 직접 입력하고 EASY/NORMAL 4장, HARD 5장, MASTER 6장의 카드를 모두 사용합니다. Countdown은 여전히 카드를 일부 남겨도 됩니다. 2048은 작은 타일의 회색부터 연두·시안·파랑·노랑·주황·마젠타·빨강 계열로 색이 변합니다. [전체 조작표](docs/CONTROLS.md).

진행 데이터는 **최근 플레이한 최대 5개 게임**만 유지합니다. 여섯 번째 게임을 시작하면 가장 오래된 저장을 지우기 전에 확인합니다. 새 저장은 게임마다 필요한 길이만큼 두 사본을 만들며, 최초 실행에 462,032바이트짜리 아카이브 두 개를 미리 만들지 않습니다. 기존 `NGARCA.dat`/`NGARCB.dat`는 처음 실행할 때 옮깁니다. 기존 파일에는 정확한 마지막 플레이 시각이 없으므로 **마지막 게임을 우선**, 나머지는 저장 세대 번호가 높은 순서로 최대 5개를 선택합니다. 이전 통계는 새 저장으로 옮기지 않습니다. [저장 형식](docs/STORAGE_FORMAT.md), [이관·복구](docs/STORAGE_ARCHIVE_AUDIT.md).

일반판과 진단판은 `NG`/`ND` 저장 파일을 따로 사용합니다. 진단판은 Main→F2 SET→F3 DIAG에서 RAM/시간 계측, 1,000회 stress, `NDDIAG.txt` 내보내기를 제공합니다. 실기 지연·메모리 최대치는 아직 **HARDWARE TEST REQUIRED**입니다. [현재 호스트 측정](docs/PERFORMANCE_36.md) · [진단 안내](docs/DIAGNOSTICS.md).

빌드에는 설치된 fxSDK/gint 2.11, SH GCC 14.1, CMake, fxgxa, Python 3을 사용합니다. SDK 위치가 다르면 `NUMGAME_SDK_ROOT`를 설정하세요.

```sh
bash tools/test.sh
bash tools/verify_content.sh
bash tools/clean_build.sh
bash tools/clean_build.sh --diagnostic
python3 tools/captures.py
```

macOS의 현재 ASan 실행 환경은 최소 프로그램에서도 `main` 전에 멈추므로 **ASan은 미검증**입니다. UBSan·호스트 mock·SH 빌드는 실기 시험을 대체하지 않습니다. 자체 코드·문제·아이콘은 [MIT](LICENSE)이고 글꼴·런타임·재사용 코드 고지는 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)에 있습니다.
