project('saxamaphone', 
    'c', 
    default_options: ['c_std=c99'],
    version: '2.0.0')

# meson setup -Db_coverage=true -Db_sanitize=address builddir

c_args = []

if build_machine.system() == 'windows'
    c_args = ['/W4', '/WX']
else
    c_args = ['-Wall', '-Wextra', '-Werror']
endif

libsaxamaphone = library(
    'saxamaphone', 
    files(
        './src/saxamaphone.c',
        './src/saxmapper.c'),
    include_directories: include_directories('./include/'),
    version: meson.project_version(),
    soversion: '2',
    c_args: c_args
        + '-DSAXAMAPHONE_EXPORTS')

saxamaphone_dep = declare_dependency(
  include_directories: include_directories('./include/'),
  link_with: libsaxamaphone)

ex_custom_alloc = executable('example-custom-alloc', 
    sources: files('./example/customallocator.c'),
    dependencies: [saxamaphone_dep])

ex_xml_file = executable('example-xml-file', 
    sources: files('./example/xmlfile.c'),
    dependencies: [saxamaphone_dep])

ex_xml_string = executable('example-xml-string', 
    sources: files('./example/xmlstring.c'),
    dependencies: [saxamaphone_dep])

tests = executable('saxamaphone-tests',
    sources: [
        'src/saxalloc.c',
        'src/saxamaphone.c',
        'src/saxmapper.c',
        'src/saxstream.c',
        'test/arena.c', 
        'test/main.c',
        'test/suite/alloctests.c',
        'test/suite/attrtests.c',
        'test/suite/cdatatests.c',
        'test/suite/codepttests.c',
        'test/suite/commenttests.c',
        'test/suite/contenttests.c',
        'test/suite/endtagtests.c',
        'test/suite/gettertests.c',
        'test/suite/mappertests.c',
        'test/suite/mapalphatests.c',
        'test/suite/mapencodetests.c',
        'test/suite/mapmalformedtests.c',
        'test/suite/mapviewtests.c',
        'test/suite/objmaptests.c',
        'test/suite/parsertests.c',
        'test/suite/procinsttests.c',
        'test/suite/starttagclosedtests.c',
        'test/suite/starttagtests.c',
        'test/suite/strtests.c',
        'test/suite/streamtests.c',
        'test/suite/trimtests.c',
        'test/suite/unescapetests.c'],
    include_directories: include_directories(['include', 'src', 'test']),
    c_args: c_args
        + '-DSAXAMAPHONE_TEST'
        + '-DSAXAMAPHONE_DEBUG'
)
# `ninja coverage` to generate coverage reports

test('saxamaphone-tests', tests)
test('example-custom-alloc', ex_custom_alloc)
test('example-xml-file', ex_xml_file)
test('example-xml-string', ex_xml_string)
