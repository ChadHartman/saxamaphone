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

tests_misc = executable('saxamaphone-tests',
    sources: srcs 
        + 'test/arena.c' 
        + 'test/main.c',
    include_directories: includes,
    c_args: c_args
        + '-DSAXAMAPHONE_TEST')

tests_obj_map = executable('saxamaphone-tests-obj-map',
    sources: srcs 
        + 'test/arena.c' 
        + 'test/main-obj-map.c',
    include_directories: includes,
    c_args: c_args)

test('tests-misc', tests_misc)
test('tests-obj-map', tests_obj_map)
