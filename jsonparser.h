#ifndef __JSON_PARSER_H
#define __JSON_PARSER_H

#include "gamedata.h"


struct GameData         *json_parser_read_file          (const char *filename);
bool                     json_parser_write_file         (const struct GameData *game,
                                                         const char *filename);

#endif /* __JSON_PARSER_H */
