sources = [
            'Attributes.cpp',
            'Main.cpp',
            'Lexer.cpp',
            'Parser.cpp',

            'IRGen/irGenAide.cpp',
            'IRGen/BinOps.cpp',
            'IRGen/ConstantExprs.cpp',
            'IRGen/Declrs.cpp',
            'IRGen/Entities.cpp',
            'IRGen/Functions.cpp',
            'IRGen/Iteration.cpp',
            'IRGen/Lists.cpp',
            'IRGen/Selection.cpp',
            'IRGen/Types.cpp'
        ]

executable(
            'kronkc',
            sources,
            include_directories : inc,
            dependencies: llvm_dep,
            cpp_args: '-w',
        )
