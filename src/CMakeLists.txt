add_executable (disksurvey main.c diskscan.c disklist.c diskmon.c fileutils.c)
target_link_libraries(disksurvey wire)
target_link_libraries(disksurvey scsicmd)
