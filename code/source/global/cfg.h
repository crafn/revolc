#ifndef REVOLC_GLOBAL_CFG_H
#define REVOLC_GLOBAL_CFG_H

// These options largely determine the memory usage and performance of the engine.
// Many of these will at some point be removed or moved to (game dependent) cfg files.

#define FRAME_MEM_SIZE (1024*1024*20)

#define MAX_BLOB_SIZE (1024*1024*512) // 0.5 Gb
#define MAX_RES_FILES 64
#define DEFAULT_RES_ROOT "../../resources/"
#define MISSING_RES_FILE "../../resources/revolc/basic/missing"

#define MAX_ARMATURE_JOINT_COUNT 16
#define MAX_ARMATURE_CLIP_COUNT 32

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 512
#define MAX_AUDIO_CHANNELS 64

#define MAX_DRAW_CMD_COUNT (1024*100)
#define MAX_MODELENTITY_COUNT (1024*15)
#define MAX_COMPENTITY_COUNT (512)
#define MAX_SUBENTITY_COUNT (16)
#define MAX_DRAW_VERTEX_COUNT (1024*200)
#define MAX_DRAW_INDEX_COUNT (1024*300)
#define MAX_DEBUG_DRAW_VERTICES (1024*100)
#define MAX_DEBUG_DRAW_INDICES (MAX_DEBUG_DRAW_VERTICES*3)
#define TEXTURE_ATLAS_WIDTH 4096
#define TEXTURE_ATLAS_LAYER_COUNT 4
#define MAX_TEXTURE_LOD_COUNT 2
#define MAX_SHADER_VARYING_COUNT 8

#define MAX_NODE_COUNT (1024*20)
#define MAX_NODE_CMD_COUNT 8
#define MAX_NODE_TYPE_COUNT 1024

#define MAX_RIGIDBODY_COUNT (1024*10)
#define MAX_POLY_VERTEX_COUNT 8
#define MAX_SHAPES_PER_BODY 2
#define GRID_RESO_PER_UNIT 4
#define GRID_WIDTH 10
#define GRID_WIDTH_IN_CELLS (GRID_WIDTH*GRID_RESO_PER_UNIT)
#define GRID_CELL_COUNT (GRID_WIDTH_IN_CELLS*GRID_WIDTH_IN_CELLS)

#define MAX_FUNC_NAME_SIZE 64
#define MAX_PATH_SIZE 256

#define RES_NAME_SIZE 16

#define SYM_NAME_SIZE 256
#define MAX_SYM_COUNT 1024*4

#define MAX_GUI_ELEM_COUNT 1024
#define MAX_GUI_STACK_SIZE 32

#define UDP_MAX_PACKET_SIZE 512
#define UDP_MAX_PACKET_DATA_SIZE (UDP_MAX_PACKET_SIZE - sizeof(UdpPacketHeader)) 
#define UDP_CONNECTION_TIMEOUT 3.0
#define UDP_PACKET_ID_COUNT 256 // Limited by size of U8
#define UDP_MAX_BUFFERED_PACKET_COUNT (1024*4)
#define UDP_OUTGOING_LIMIT_FRAME (1024*8)
#define UDP_ACK_COUNT 33 // Limited by header
#define UDP_HEARTBEAT_INTERVAL 0.05 // Determines minimum rtt
//#define UDP_OUTGOING_LIMIT_SECOND

#endif // REVOLC_GLOBAL_CFG_H
