project('saxamaphone', 'c', default_options: ['c_std=c99'])

c_args = []

if build_machine.system() == 'windows'
    c_args = ['/W4', '/WX']
else
    c_args = ['-Wall', '-Wextra', '-Werror']
endif

includes =  include_directories(['include', 'src'])
srcs = [
    'src/saxamaphone.c'
]

# === Tests === #

tests = executable('saxamaphone-tests',
    sources: srcs + 'test/main.c',
    include_directories: includes,
    c_args: c_args)
