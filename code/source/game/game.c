#include "game.h"

void make_main_blob(const char *blob_path, const char *game)
{
	const char *engine_res_root = frame_str("%srevolc/", DEFAULT_RES_ROOT);
	const char *game_res_root = frame_str("%s%s/", DEFAULT_RES_ROOT, game);

	// @todo Fix crappy api!
#if CONVERSION_FROM_JSON_TO_C99
	char **game_res_paths = plat_find_paths_with_end(game_res_root, ".res");
	char **engine_res_paths = plat_find_paths_with_end(engine_res_root, ".res");
#else
	char **game_res_paths = plat_find_paths_with_end(game_res_root, ".cres");
	char **engine_res_paths = plat_find_paths_with_end(engine_res_root, ".cres");
#endif

	U32 res_count = 0;
	char *res_paths[MAX_RES_FILES] = {0};
	for (U32 i = 0; res_count < MAX_RES_FILES && engine_res_paths[i]; ++i)
		res_paths[res_count++] = engine_res_paths[i];
	for (U32 i = 0; res_count < MAX_RES_FILES && game_res_paths[i]; ++i)
		res_paths[res_count++] = game_res_paths[i];

	make_blob(blob_path, res_paths);
	for (U32 i = 0; res_paths[i]; ++i)
		FREE(gen_ator(), res_paths[i]);
	FREE(gen_ator(), engine_res_paths);
	FREE(gen_ator(), game_res_paths);
}

const char *blob_path(const char *game)
{ return frame_str("%s.blob", game); }

