#!/bin/sh
#
# PROVIDE: kiconv
# REQUIRE: mountcritremote ldconfig
# BEFORE:  DAEMON
# KEYWORD: nojail shutdown
#
# Add the following lines to /etc/rc.conf to enable kernel iconv
# tables preloading:
# kiconv_preload (bool):         Set to "YES" to enable preloading.
#                                Default is "NO".
# kiconv_local_charsets (str):   Space separated list of local charsets.
#                                Usually list of locales used on a system.
# kiconv_foreign_charsets (str): Space separated list of foreign charsets.
#                                Usually UTF-16BE + some location specific
#                                charsets
# kiconv_charset_pairs (str):    Space separated list of additional charset
#                                pairs in form of LOCAL:REMOTE
#
# See /usr/local/share/doc/kiconvtool/README for details
#

. /etc/rc.subr

name="kiconv"
rcvar="kiconv_preload"

load_rc_config $name

: ${kiconv_preload="NO"}
: ${kiconv_local_charsets=""}
: ${kiconv_foreign_charsets=""}
: ${kiconv_charset_pairs=""}
: ${kiconv_fstypes=""}

command="/usr/local/sbin/kiconvtool"
command_args="-l ${kiconv_local_charsets} -f ${kiconv_foreign_charsets} -p ${kiconv_charset_pairs}"
start_cmd="kiconv_start"
stop_cmd=":"

kiconv_start()
{
	echo 'Loading kernel iconv modules.'
	if ! kldstat -qm iconv; then # sic! we test for iconv, not libiconv
		if kldload libiconv; then
			info "libiconv module loaded"
		else
			warn "libiconv module failed to load"
			return 1
		fi
	fi

	for fs in ${kiconv_fstypes}; do
		if ! kldstat -qm ${fs}_iconv; then
			if kldload ${fs}_iconv; then
				info "${fs}_iconv module loaded"
			else
				warn "${fs}_iconv module failed to load"
			fi
		fi
	done

	echo 'Loading kernel iconv tables.'
	${command} ${command_args}
}

run_rc_command "$1"
