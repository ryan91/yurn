#ifndef __JSON_PARSER_H
#define __JSON_PARSER_H

#include "gamedata.h"

GameData               *json_parser_read_file          (const char *filename);

#endif /* __JSON_PARSER_H */
