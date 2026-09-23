# 글꼴·게임 아이콘 수정

이 문서는 `5cd9849`의 글꼴·아이콘 변경 기록이다. 이후의 설정 화면·문구 배치·
2048 색상 변경은 [UI_LAYOUT_KO.md](UI_LAYOUT_KO.md)에 정리했다.

일반 UI 글꼴을 DIFF EQ와 같은 gint 8×9 비례 글꼴로 교체했다. 8×11 raster
셀에 들어 있는 원본 픽셀과 간격을 그대로 쓰며, DIFF EQ의 host 글꼴 테이블과
byte 단위로 일치함을 확인했다. 이전 5×7의 1.5배 확대는 제거했다.
작은 cage/Kakuro 단서와 보조 문구는 5×7, Sudoku 메모는 3×5를 유지한다.

30종 각각에 원본 40×32 픽셀 도상을 추가했다. 숫자야구 공과 피드백, 방정식,
코드 단서, 자물쇠, 수열, 숫자 카드, 타이머, 빠진 연산자, 교차식, 소인수 나무,
각 퍼즐의 격자·부등호·건물·경로, 전략 게임의 더미·카드·트랙, 타일·불빛·기억
카드를 사용한다. 통계에는 별도 막대그래프를 넣었다. 여섯 메인 카테고리는
대표 게임 아이콘을 사용한다.

아이콘은 이름 왼쪽, shortcut은 우측 상단, RESUME은 이름 아래에 표시한다.
메뉴 이름도 DIFF EQ와 같은 기본 글꼴로 맞췄다. 화면 캡처는 실제 C 렌더러 출력이다.

- [전체 메뉴와 아이콘 1×](captures/menus-native.png)
- [전체 메뉴와 아이콘 2×](captures/menus-2x.png)
- [30종 플레이 화면](captures/contact-native.png)
- [작은 단서가 있는 hard 격자](captures/hard-grids-native.png)

프로덕션 코드 변경은 `src/ui/`와 `include/ui.h`의 표시 계층에만 있다.
게임 엔진·입력 처리·AI·문제팩·저장·timer 코드는 수정하지 않았다.
DIFF EQ는 읽기 전용으로 참조했다.

기존 8개 호스트 테스트/UBSan, SH strict build, G3A 패키지 검사와 렌더러 검토를
수행했다. 최종 크기/해시는 [메모리 보고](MEMORY_AUDIT.md)와
[SHA256SUMS.txt](../dist/SHA256SUMS.txt)에 있다. 실기 LCD 검증은 계속 필요하다.
