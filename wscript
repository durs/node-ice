srcdir = '.'
blddir = 'build'
VERSION = '0.1.2'

def set_options(opt):
	opt.tool_options('compiler_cxx')

def configure(conf):
	conf.env.CCFLAGS = ['-g']
	conf.check_tool('compiler_cxx')
	conf.check_tool('node_addon')

def build(bld):
	obj = bld.new_task_gen('cxx', 'shlib', 'node_addon',use = ['Ice'])
	obj.target = 'main'
	obj.lib = ['Ice']
	obj.libpath = ['/opt/Ice-3.4/lib']
	obj.linkflags = ['-g']
	obj.source = [
		'src/main.cpp',
		'src/nodeice.cpp'
	]
	obj.includes = ['/opt/Ice-3.4/include','.']