#ifndef REVOLC_GLOBAL_CFG_H
#define REVOLC_GLOBAL_CFG_H

// These options largely determine the memory usage and performance of the engine
#define FRAME_MEM_SIZE (1024*1024*10)

#define MAX_ARMATURE_JOINT_COUNT 16

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 512
#define MAX_AUDIO_CHANNELS 64

#define MAX_MODELENTITY_COUNT (1024*15)
#define MAX_COMPOUNDENTITY_COUNT (1024)
#define MAX_ATTACHED_ENTITY_COUNT (8)
#define MAX_DRAW_VERTEX_COUNT (1024*100)
#define MAX_DRAW_INDEX_COUNT (1024*250)
#define MAX_DEBUG_DRAW_VERTICES (1024*50)
#define MAX_DEBUG_DRAW_INDICES (MAX_DEBUG_DRAW_VERTICES*3)
#define TEXTURE_ATLAS_WIDTH 4096
#define TEXTURE_ATLAS_LAYER_COUNT 4

#define MAX_NODE_COUNT (1024*20)
#define MAX_NODE_CMD_COUNT 8
#define MAX_NODE_TYPE_COUNT 1024

#define MAX_RIGIDBODY_COUNT (1024*10)
#define MAX_POLY_VERTEX_COUNT 8
#define MAX_SHAPES_PER_BODY 2
#define GRID_RESO_PER_UNIT 4
#define GRID_WIDTH 100
#define GRID_WIDTH_IN_CELLS (GRID_WIDTH*GRID_RESO_PER_UNIT)
#define GRID_CELL_COUNT (GRID_WIDTH_IN_CELLS*GRID_WIDTH_IN_CELLS)

#define MAX_FUNC_NAME_SIZE 64

#endif // REVOLC_GLOBAL_CFG_H
