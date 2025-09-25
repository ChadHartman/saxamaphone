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


tests_misc = executable('saxamaphone-tests',
    sources: [
        'src/saxamaphone.c',
        'test/arena.c', 
        'test/main-misc.c'],
    include_directories: includes,
    c_args: c_args
        + '-DSAXAMAPHONE_TEST'
        # + '-DSAXAMAPHONE_DEBUG'
    )

tests_obj_map = executable('saxamaphone-tests-obj-map',
    sources: [
        'src/saxamaphone.c',
        'test/arena.c',
        'test/main-obj-map.c'],
    include_directories: includes,
    c_args: c_args
        + '-DSAXAMAPHONE_DEBUG'
    )

tests_alloc = executable('saxamaphone-tests-alloc',
    sources: [
        'src/saxamaphone.c',
        'test/main-alloc.c'],
    include_directories: includes,
    c_args: c_args)


test('tests-arena', tests_arena)
test('tests-alloc', tests_alloc)
test('tests-misc', tests_misc)
test('tests-obj-map', tests_obj_map)
