cmake_minimum_required(VERSION 4.2)
project(ALD C)

set(CMAKE_C_STANDARD 23)

add_executable(ALD main.c
        al_string.c
        al_string.h
        al_alloc.h
        al_fixed_size_pool.c
        al_fixed_size_pool.h
        al_lexer.c
        al_lexer.h
        al_fileutils.c
        al_fileutils.h
        al_reader.c
        al_reader.h
        al_ast.c
        al_ast.h
        al_cdefs.h
        al_charutils.h
        al_symbol.c
        al_symbol.h
        al_stringview.c
        al_stringview.h
        al_environment.c
        al_environment.h
        al_symtab.h
        al_symtab.c
        al_elastic_fixed_size_pool.h
        al_elastic_fixed_size_pool.c
        al_streams.c
        al_streams.h
        al_vector.c
        al_vector.h
        al_eval.c
        al_eval.h
        al_object.c
        al_object.h
        al_objtab.c
        al_objtab.h
        al_assert.h)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
