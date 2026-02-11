#include <ultra64.h>
#include "sm64.h"
#include "behavior_data.h"
#include "model_ids.h"
#include "seq_ids.h"
#include "dialog_ids.h"
#include "segment_symbols.h"
#include "level_commands.h"

#include "game/level_update.h"

#include "levels/scripts.h"

#include "make_const_nonconst.h"
#include "levels/castle_grounds/header.h"

/* Fast64 begin persistent block [scripts] */
/* Fast64 end persistent block [scripts] */

const LevelScript level_castle_grounds_entry[] = {
	INIT_LEVEL(),
	LOAD_MIO0(0x07, _castle_grounds_segment_7SegmentRomStart, _castle_grounds_segment_7SegmentRomEnd), 
	ALLOC_LEVEL_POOL(),
	MARIO(MODEL_MARIO, 0x00000001, bhvMario), 
	/* Fast64 begin persistent block [level commands] */
	/* Fast64 end persistent block [level commands] */

	AREA(1, castle_grounds_area_1),
		WARP_NODE(0x0A, LEVEL_BOB, 0x01, 0x0A, WARP_NO_CHECKPOINT),
		WARP_NODE(0xF0, LEVEL_BOB, 0x01, 0x0A, WARP_NO_CHECKPOINT),
		WARP_NODE(0xF1, LEVEL_BOB, 0x01, 0x0A, WARP_NO_CHECKPOINT),
		WARP_NODE(0x0C, LEVEL_WF, 0x01, 0x0D, WARP_NO_CHECKPOINT),
		WARP_NODE(0x0D, LEVEL_BOB, 0x01, 0x0C, WARP_NO_CHECKPOINT),
		MARIO_POS(0x01, 0, 0, 200, 0),
		OBJECT(MODEL_NONE, 0, 200, 0, 0, 0, 0, 0x000A0000, bhvSpinAirborneWarp),
		OBJECT(E_MODEL_AMP, -3477, -45, 2566, 0, 0, 0, 0x00000000, bhvCirclingAmp),
		OBJECT(E_MODEL_AMP, 1438, -45, -2795, 0, 0, 0, 0x00030000, bhvCirclingAmp),
		OBJECT(MODEL_BUTTERFLY, -5110, -111, -377, 0, 0, 0, 0x00000000, bhvTripletButterfly),
		OBJECT(MODEL_BUTTERFLY, -115, 102, -929, 0, 0, 0, 0x00000000, bhvButterfly),
		OBJECT(MODEL_BUTTERFLY, 1087, -106, -1230, 0, 0, 0, 0x00000000, bhvButterfly),
		OBJECT(MODEL_BUTTERFLY, 1087, -106, -1872, 0, 0, 0, 0x00000000, bhvButterfly),
		OBJECT(MODEL_BUTTERFLY, -1883, -106, -856, 0, 0, 0, 0x00000000, bhvButterfly),
		OBJECT(MODEL_BUTTERFLY, -4045, -111, -747, 0, 0, 0, 0x00000000, bhvTripletButterfly),
		OBJECT(MODEL_HEAVE_HO, -4914, -103, -5676, 0, 0, 0, 0x00000000, bhvHeaveHo),
		OBJECT(E_MODEL_GOOMBA, 1808, -189, 1303, 0, 0, 0, 0x00000000, bhvGoomba),
		OBJECT(E_MODEL_GOOMBA, 1808, -189, -2001, 0, 0, 0, 0x00000000, bhvGoomba),
		OBJECT(E_MODEL_GOOMBA, -3657, -189, 1025, 0, 0, 0, 0x00000000, bhvGoomba),
		OBJECT(MODEL_MIPS, -1070, 1052, -2239, 0, 0, 0, 0x00000000, bhvMips),
		OBJECT(E_MODEL_STAR, 1552, 1, -3559, 0, 0, 0, 0, bhvStar),
		OBJECT(E_MODEL_STAR, -1788, 1, -3062, 0, 0, 0, 0, bhvStar),
		OBJECT(E_MODEL_STAR, -2095, 1, 1543, 0, 0, 0, 0, bhvStar),
		TERRAIN(castle_grounds_area_1_collision),
		MACRO_OBJECTS(castle_grounds_area_1_macro_objs),
		STOP_MUSIC(0),
		TERRAIN_TYPE(TERRAIN_GRASS),
		/* Fast64 begin persistent block [area commands] */
		/* Fast64 end persistent block [area commands] */
	END_AREA(),
	FREE_LEVEL_POOL(),
	MARIO_POS(0x01, 0, 0, 200, 0),
	CALL(0, lvl_init_or_update),
	CALL_LOOP(1, lvl_init_or_update),
	CLEAR_LEVEL(),
	SLEEP_BEFORE_EXIT(1),
	EXIT(),
};