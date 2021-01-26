sources = [
            'Attributes.cpp',
            'Main.cpp',
            'Driver.cpp',
            'Lexer.cpp',
            
            'Parser/Constants.cpp',
            'Parser/Declrs.cpp',
            'Parser/Entities.cpp',
            'Parser/Functions.cpp',
            'Parser/GenericExprs.cpp',
            'Parser/Identifiers.cpp',
            'Parser/Iteration.cpp',
            'Parser/Lists.cpp',
            'Parser/ParserInterfaceImpl.cpp',
            'Parser/Selection.cpp', 
            'Parser/ParserUtils.cpp',

            'IRGen/BinOps.cpp',
            'IRGen/ConstantExprs.cpp',
            'IRGen/Declrs.cpp',
            'IRGen/Entities.cpp',
            'IRGen/Functions.cpp',
            'IRGen/Iteration.cpp',
            'IRGen/Lists.cpp',
            'IRGen/Selection.cpp',
            'IRGen/StmtBlock.cpp',
            'IRGen/Types.cpp',

            'IRGen/irGenAide.cpp',
            'IRGen/typeInfo.cpp'
        ]

executable(
            'kronkc',
            sources,
            include_directories : inc,
            dependencies: llvm_dep,
            cpp_args: '-w',
            link_args: '-lstdc++fs'
        )
