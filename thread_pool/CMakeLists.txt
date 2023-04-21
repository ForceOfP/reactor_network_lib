aux_source_directory(. DIR_LIB_SRCS)
aux_source_directory(./adding DIR_LIB_SRCS)

add_library(Thread_Pool_Lib ${DIR_LIB_SRCS})