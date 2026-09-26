#ifndef NUMGAME_STORAGE_H
#define NUMGAME_STORAGE_H
#include "ng.h"
#define NG_MODES 8
#define NG_RECORD_MAX 14000
#define NG_ARCHIVE_HEADER 32u
/* The former archive layout is frozen for read-only import of existing saves. */
#define NG_ARCHIVE_LEGACY_IDS 32u
#define NG_ARCHIVE_BYTES (NG_ARCHIVE_HEADER+(NG_ARCHIVE_LEGACY_IDS+1u)*NG_RECORD_MAX)
#define NG_RECENT_LIMIT 5u
typedef struct {uint32_t completed,wins,losses,draws,best_score,best_moves,best_ms,best_aux;} NgBest;
typedef struct {
 uint32_t started,active_ms;
 NgBest best[NG_MODES][NG_LEVEL_COUNT][2]; /* mode, difficulty, assisted */
} NgStats;
/* One bounded shuffle cycle and four recent stable IDs per mode/level. */
typedef struct {
 uint32_t shuffle;
 uint16_t next,count;
 uint32_t recent[4];
 uint8_t recent_count;
} NgSupply;
typedef struct {
 NgGame game,undo[NG_UNDO];
 NgStats stats;
 NgSupply supply[NG_MODES][NG_LEVEL_COUNT];
 uint8_t undo_count;
 uint32_t generation;
} NgSession;
typedef struct {
 uint32_t played,completed,assisted,active_ms;
 uint8_t exists,difficulty,mode,reserved;
} NgSummary;
typedef struct {
 void *context;
 /* id=1..32 stable runs, id=0 settings. Slot 0/1. -2 means absent; -1 error. */
 int (*open)(void *,unsigned,unsigned,bool);
 ptrdiff_t (*read)(void *,int,void *,size_t);
 ptrdiff_t (*write)(void *,int,const void *,size_t);
 int (*close)(void *,int);
 bool (*prepare)(void *,unsigned,unsigned,size_t); /* exact bytes for a new file */
} NgIO;
enum { NG_LOAD_ABSENT,NG_LOAD_OK,NG_LOAD_RECOVERED,NG_LOAD_INVALID,NG_LOAD_IO_ERROR };
typedef struct {
 uint32_t generation;
 uint8_t last_game,difficulty[NG_ID_MAX],mode[NG_ID_MAX];
 uint8_t first_help,show_time;
 uint8_t recent_count,recent[NG_RECENT_LIMIT];
 uint8_t pending_delete;
 uint8_t migration_complete;
 uint16_t target;
} NgSettings;
uint32_t ng_crc32(const void *data,size_t size);
size_t ng_encode(const NgSession *s,uint8_t *out,size_t capacity);
bool ng_decode(NgSession *s,const uint8_t *data,size_t length,unsigned expected_id);
/* Version-5 resume payload stores only its current mode/level supply. */
size_t ng_single_encode(const NgSession *s,uint8_t *out,size_t capacity);
bool ng_single_decode(NgSession *s,const uint8_t *data,size_t length,unsigned expected_id);
int ng_load_io(NgSession *s,unsigned id,const NgIO *io);
bool ng_save_io(NgSession *s,const NgIO *io);
int ng_settings_load_io(NgSettings *s,const NgIO *io);
bool ng_settings_save_io(NgSettings *s,const NgIO *io);
/* Version 5: one atomic settings + optional unfinished-run snapshot. */
int ng_state_load_io(NgSettings *settings,NgSession *session,bool *active,const NgIO *io);
bool ng_state_save_io(NgSettings *settings,const NgSession *session,bool active,const NgIO *io);
bool ng_state_slot_valid(const NgIO *io,unsigned slot);
size_t ng_settings_encode(const NgSettings *s,uint8_t *out,size_t capacity);
bool ng_settings_decode(NgSettings *s,const uint8_t *data,size_t length);
uint32_t ng_session_fingerprint(const NgSession *s);
bool ng_slot_valid(const NgIO *io,unsigned id,unsigned slot);
void ng_summarize(const NgSession *s,NgSummary *summary);
void ng_record_result(NgSession *s);
/* Native wraps the entire safe main-thread transaction in gint_world_switch. */
int ng_storage_load(NgSession *s,unsigned id);
bool ng_storage_save(NgSession *s);
int ng_settings_load(NgSettings *s);
bool ng_settings_save(NgSettings *s);
bool ng_storage_cleanup(void);
bool ng_storage_delete(unsigned id);
int ng_state_load(NgSettings *settings,NgSession *session,bool *active);
bool ng_state_save(NgSettings *settings,const NgSession *session,bool active);
/* Migrates validated older saves into the two-file version-5 namespace. */
bool ng_state_migrate(NgSession *workspace);
/* Startup only: workspace must not contain a live game. Retryable; false preserves sources. */
bool ng_storage_migrate(NgSession *workspace);
/* RAM fixture only, uses the existing transaction workspace; never writes files. */
bool ng_storage_fixture(unsigned id,unsigned difficulty,unsigned mode,uint32_t seed);
#ifndef FXCG50
void ng_storage_directory(const char *path);
#endif
#endif
