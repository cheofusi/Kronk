sources = [
            'helper_funcs.cpp',
        ]

kronkrt = shared_library(
                        'kronkrt',
                        sources,
                    )

message('Generating bitcode for the kronk runtime...')
r = run_command('bash', 'kronrtbc_gen.sh')
if r.returncode() != 0
  error('Failed to generate bitcode for the kronk runtime!!')
endif
message('bitcode generated')