project('saxamaphone', 'c', default_options: ['c_std=c99'])

c_args = []

if build_machine.system() == 'windows'
    c_args = ['/W4', '/WX']
else
    c_args = ['-Wall', '-Wextra', '-Werror']
endif

includes = include_directories(['include', 'src', 'test'])

tests_arena = executable('arena-tests',
    sources: [
        'test/arena.c', 
        'test/main-arena.c'
    ],
    include_directories: includes,
    c_args: c_args
)

tests_sax = executable('saxamaphone-tests',
    sources: [
        'src/saxamaphone.c',
        'test/arena.c', 
        'test/objmaptests.c',
        'test/procinsttests.c',
        'test/starttagclosedtests.c',
        'test/starttagtests.c',
        'test/main.c'],
    include_directories: includes,
    c_args: c_args
        + '-DSAXAMAPHONE_TEST'
        # + '-DSAXAMAPHONE_DEBUG'
    )

test('tests-arena', tests_arena)
test('tests-sax', tests_sax)

# `ninja coverage` to generate coverage reports