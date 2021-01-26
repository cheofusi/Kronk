project(
        'kronkc',
        'cpp',
        default_options : ['cpp_std=c++17'], #'-stdlib=libc++'],
        version : '0.0.1',
        license : 'MIT'
        )


inc = include_directories('include')

llvm_dep = dependency('llvm', version : '>= 8.0', modules : ['all'])
message(llvm_dep.name() + llvm_dep.version())

subdir('src')

# copy target to top level bin directory
meson.add_install_script('install.sh')

# now run top level test script