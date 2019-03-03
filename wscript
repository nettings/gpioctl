APPNAME = 'gpioctl'
VERSION = '0.0.4'

top = '.'


def options(opt):
	opt.load('compiler_c')
	opt.add_option(
		'--disable-jack',
		default = False,
		action = 'store_true',
		dest = 'nojack',
		help = 'Do not use JACK even if library is present.')
	opt.add_option(
		'--disable-alsa',
		default = False,
		action = 'store_true',
		dest = 'noalsa',
		help = 'Do not use ALSA even if library is present.')
	opt.add_option(
		'--disable-osc',
		default = False,
		action = 'store_true',
		dest = 'noosc',
		help = 'Do not use OSC even if liblo is present.')

def configure(cnf):
	cnf.env.libs = ['GPIOD', 'PTHREAD']
        cnf.env.objs = ['parse_cmdline', 'gpiod_process', 'stdout_process']
	cnf.load('compiler_c',
		cache = True)
	cnf.check(
		features = 'c cshlib',
		lib = 'gpiod',
		uselib_store = 'GPIOD',
		mandatory = True)
	cnf.check(
		header_name = 'gpiod.h',
		mandatory = True)
	cnf.check(
		features = 'c cshlib',
		lib = 'pthread',
		uselib_store = 'PTHREAD',
		mandatory = True)
  	cnf.check(
		header_name = 'pthread.h',
		mandatory = True)
	if not cnf.options.nojack:
		lib = cnf.check(
			features = 'c cshlib', 
			lib = 'jack', 
			uselib_store = 'JACK',
			define_name = 'HAVE_JACK')
		header = cnf.check(
			header_name = 'jack/jack.h')
		if lib and header:
			cnf.env.libs += ['JACK']
			cnf.env.objs += ['jack_process', 'ringbuffer', 'jack_cmdline']
	if not cnf.options.noalsa:
		lib = cnf.check(
			features = 'c cshlib',
			lib = 'asound',
			uselib_store = 'ASOUND',
			define_name = 'HAVE_ALSA')
		header = cnf.check(
			header_name = 'alsa/asoundlib.h')
		if lib and header:
			cnf.env.libs += ['ASOUND']
			cnf.env.objs += ['alsa_process', 'alsa_cmdline']
	if not cnf.options.noosc:
		lib = cnf.check(
			features = 'c cshlib',
			lib = 'lo',
			uselib_store = 'LO',
			define_name = 'HAVE_OSC')
		header = cnf.check(
			header_name = 'lo/lo.h')
		if lib and header:
			cnf.env.libs += ['LO']
			cnf.env.objs += ['osc_process', 'osc_cmdline']
	cnf.cc_add_flags()
	cnf.link_add_flags()
	cnf.cxx_add_flags()
	cnf.add_os_flags('DESTDIR');
	cnf.add_os_flags('PREFIX');
	cnf.write_config_header('config.h');
	

def build(bld):
	if 'JACK' in bld.env.libs:
		bld.objects(
			source = 'jack_process.c',
			target = 'jack_process')
		bld.objects(
			source = 'ringbuffer.c',
			target = 'ringbuffer')
		bld.objects(
			source = 'jack_cmdline.c',
			target = 'jack_cmdline')
	if 'ASOUND' in bld.env.libs:
		bld.objects(
			source = 'alsa_process.c',
			target = 'alsa_process')
		bld.objects(
			source = 'alsa_cmdline.c',
			target = 'alsa_cmdline')
	if 'LO' in bld.env.libs:
		bld.objects(
			source = 'osc_process.c',
			target = 'osc_process')
		bld.objects(
			source = 'osc_cmdline.c',
			target = 'osc_cmdline')
	bld.objects(
		source = 'parse_cmdline.c',
		target = 'parse_cmdline')
	bld.objects(
		source = 'gpiod_process.c',
		target = 'gpiod_process')
	bld.objects(
		source = 'stdout_process.c',
		target = 'stdout_process')
	bld.program(
		source = 'main.c', 
		target = 'gpioctl',
		use = bld.env.objs,
		uselib = bld.env.libs,
		install_path = '${DESTDIR}/${PREFIX}/bin')
	