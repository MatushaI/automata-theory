cmake_minimum_required(VERSION 3.28)
project(lab2)

set(CMAKE_CXX_STANDARD 23)

add_executable(lab2 main.cpp syntaxTree.cpp syntaxTree.h HiearchyOperations.cpp HiearchyOperations.h
        NFA.cpp NFA.h
        DFA.cpp
        DFA.h
        LangOperations.cpp
        LangOperations.h
        myRegex.cpp
        myRegex.h)
