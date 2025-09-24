project('saxamaphone', 'c', default_options: ['c_std=c99'])

c_args = []

if build_machine.system() == 'windows'
    c_args = ['/W4', '/WX']
else
    c_args = ['-Wall', '-Wextra', '-Werror']
endif

tests_misc = executable('saxamaphone-tests',
    sources: [
        'src/saxamaphone.c',
        'test/arena.c', 
        'test/main-misc.c'],
    include_directories: include_directories(['include', 'src']),
    c_args: c_args
        + '-DSAXAMAPHONE_TEST'
        # + '-DSAXAMAPHONE_DEBUG'
    )

tests_obj_map = executable('saxamaphone-tests-obj-map',
    sources: [
        'src/saxamaphone.c',
        'test/arena.c',
        'test/main-obj-map.c'],
    include_directories: include_directories(['include', 'src']),
    c_args: c_args)

tests_alloc = executable('saxamaphone-tests-alloc',
    sources: [
        'src/saxamaphone.c',
        'test/main-alloc.c'],
    include_directories: include_directories(['include', 'src']),
    c_args: c_args)


test('tests-alloc', tests_alloc)
test('tests-misc', tests_misc)
test('tests-obj-map', tests_obj_map)
