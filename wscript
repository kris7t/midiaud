
APPNAME = 'midiaud'

top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_cxx boost')

def configure(conf):
    conf.load('compiler_cxx boost')
    conf.env.append_value('CXXFLAGS', '-std=c++11')
    conf.check_cfg(package = 'jack',
                   args = ['jack >= 1.9.8', 'jack < 2', '--libs', '--cflags'],
                   uselib_store = 'JACK',
                   msg = "Checking for 'jack 1.9.8'")
    conf.check_cfg(package = 'smf',
                   args = ['--libs', '--cflags'],
                   uselib_store = 'SMF')
    conf.check_boost(lib = 'program_options')

def build(bld):
    bld.recurse('src')
