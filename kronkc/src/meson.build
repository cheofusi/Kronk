sources = [
            'Attributes/Attributes.cpp',
            'CompileDriver/Driver.cpp',
            'TheJIT/Kronkjit.cpp',
            'Names/Names.cpp',
            'Lexer/Lexer.cpp',
            'Main.cpp',
            
            'Parser/Constants.cpp',
            'Parser/Declrs.cpp',
            'Parser/Entities.cpp',
            'Parser/Functions.cpp',
            'Parser/GenericExprs.cpp',
            'Parser/Identifiers.cpp',
            'Parser/Imports.cpp',
            'Parser/Iteration.cpp',
            'Parser/Lists.cpp',
            'Parser/ParserInterfaceImpl.cpp',
            'Parser/Selection.cpp', 
            'Parser/ParserUtils.cpp',

            'IRGen/OperatorExprs.cpp',
            'IRGen/ConstantExprs.cpp',
            'IRGen/Declrs.cpp',
            'IRGen/Entities.cpp',
            'IRGen/Functions.cpp',
            'IRGen/Imports.cpp',
            'IRGen/Iteration.cpp',
            'IRGen/Lists.cpp',
            'IRGen/Selection.cpp',
            'IRGen/BlockStmt.cpp',
            'IRGen/TypeIds.cpp',

            'IRGen/IRGenAide.cpp',
            'IRGen/Types.cpp'
        ]

executable(
            'kronkc',
            sources,
            include_directories : inc,
            dependencies: llvm_dep,
            cpp_args: ['-w', '-g'],
            link_args: '-lstdc++fs'
        )
