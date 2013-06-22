
from waflib.Configure import conf
from waflib.TaskGen import feature, before, after
from waflib import Task, Logs

APPNAME = 'midiaud'

top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_cxx boost')

class lockfree_check_task(Task.Task):
    def run(self):
        from subprocess import Popen, PIPE, STDOUT
        from re import search, MULTILINE
        bld = self.generator.bld
        dest = self.inputs[0]
        p = Popen(dest.abspath(), shell = False, stdin = PIPE, stdout = PIPE,
                  stderr = STDOUT, close_fds = True)
        output = p.stdout.read()
        output = output.decode()
        atomics_are_lockfree = True
        for t in self.generator.atomic_types:
            bld.start_msg('Checking for lock-free std::atomic<%s>' % t)
            if search('^%s$' % t, output, MULTILINE) == None:
                atomics_are_lockfree = False
                bld.end_msg(False)
            else:
                bld.end_msg(True)
        bld.retval = atomics_are_lockfree

@feature('lockfree')
@after('process_source')
def perform_lockfree_check(self):
    self.create_task('lockfree_check', self.link_task.outputs)

@conf
def check_lockfree(self, **kw):
    fragment = '''
#include <iostream>
#include <atomic>

int main() {
'''
    for t in kw['atomic_types']:
        fragment += 'if (std::atomic<%s>().is_lock_free()) std::cout << "%s\\n";\n' \
                    % (t, t)
    fragment += 'return 0;\n}'
    kw.update({'features': 'cxx cxxprogram lockfree',
               'fragment': fragment});
    self.validate_c(kw)
    return self.run_c_code(**kw)

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
    if not conf.check_lockfree(atomic_types = ['bool', 'std::ptrdiff_t']):
        Logs.warn('Some atomics are not lock-free. Proceed at your own peril!')

def build(bld):
    bld.recurse('src')
