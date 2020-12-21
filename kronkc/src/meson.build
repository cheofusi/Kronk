sources = [
            'main.cpp',
            'Lexer.cpp',
            'Parser.cpp',
            'iRGenerator.cpp',
            'iRGenHelpers.cpp'
        ]


executable(
            'kronkc',
            sources,
            include_directories : inc,
            dependencies: llvm_dep,
            cpp_args: '-w',
        )
