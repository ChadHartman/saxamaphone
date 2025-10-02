project('saxamaphone', 'c', default_options: ['c_std=c99'])

# meson setup -Db_coverage=true builddir

c_args = []

if build_machine.system() == 'windows'
    c_args = ['/W4', '/WX']
else
    c_args = ['-Wall', '-Wextra', '-Werror']
endif

includes = include_directories(['include', 'src', 'test'])

tests = executable('saxamaphone-tests',
    sources: [
        'src/saxamaphone.c',
        'test/arena.c', 
        'test/main.c',
        'test/suite/alloctests.c',
        'test/suite/attrtests.c',
        'test/suite/cdatatests.c',
        'test/suite/codepttests.c',
        'test/suite/commenttests.c',
        'test/suite/contenttests.c',
        'test/suite/endtagtests.c',
        'test/suite/filebufsizetests.c',
        'test/suite/gettertests.c',
        'test/suite/objmaptests.c',
        'test/suite/parsertests.c',
        'test/suite/procinsttests.c',
        'test/suite/starttagclosedtests.c',
        'test/suite/starttagtests.c',
        'test/suite/strtests.c',
        'test/suite/trimtests.c',
        'test/suite/unescapetests.c'],
    include_directories: includes,
    c_args: c_args
        + '-DSAXAMAPHONE_TEST'
        # + '-DSAXAMAPHONE_DEBUG'
)

test('saxamaphone-tests', tests)

# `ninja coverage` to generate coverage reports

libsaxamaphone = library(
    'saxamaphone', 
    files('./src/saxamaphone.c')
)

saxamaphone_dep = declare_dependency(
  include_directories: include_directories('./include/'),
  link_with: libsaxamaphone,
)
