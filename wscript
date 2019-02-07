APPNAME = 'gpioctl'
VERSION = '0.0.1'

top = '.'


def options(opt):
	opt.load('compiler_c')

def configure(cnf):
	cnf.load('compiler_c', cache=True)
	cnf.parse_flags('-g -Wall -Werror', 'DEBUG')
	cnf.check(features='c cshlib', lib='pthread', uselib_store='PTHREAD')
	cnf.check(features='c cshlib', lib='jack', uselib_store='JACK')
	cnf.check(features='c cshlib', lib='asound', uselib_store='ASOUND')
	cnf.check(features='c cshlib', lib='gpiod', uselib_store='GPIOD')
	cnf.cc_add_flags()
	cnf.link_add_flags()
	cnf.cxx_add_flags()
	
def build(bld):
	bld.program(source='main.c', 
		target='gpioctl', use='gpiod_process jack_process alsa_process ringbuffer parse_cmdline',
		uselib='PTHREAD JACK ASOUND GPIOD')
	bld.objects(source='gpiod_process.c', target='gpiod_process')
	bld.objects(source='jack_process.c', target='jack_process')
	bld.objects(source='alsa_process.c', target='alsa_process')
	bld.objects(source='ringbuffer.c', target='ringbuffer')
	bld.objects(source='parse_cmdline.c', target='parse_cmdline')
	