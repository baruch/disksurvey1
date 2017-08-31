add_library(scsicmd STATIC ata.c ata_smart.c cdb.c parse_inquiry.c parse_read_cap.c parse_sense.c log_sense.c parse.c str_map.c smartdb/smartdb.c smartdb/smartdb_gen.c)
target_include_directories (scsicmd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
