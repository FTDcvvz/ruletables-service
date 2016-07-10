# -*- Makefile -*-

AM_CFLAGS        = ${regular_CFLAGS}
AM_CPPFLAGS      = ${regular_CPPFLAGS} -I${top_builddir}/include -I${top_srcdir}/include ${kinclude_CPPFLAGS}

if ENABLE_STATIC
xtables_multi_CFLAGS  += -DALL_INCLUSIVE
endif
if ENABLE_IPV4
ruletables_SOURCES += iptables.c
ruletables_CFLAGS  += -DENABLE_IPV4
ruletables_LDADD   += ../libiptc/libip4tc.la ../extensions/libext4.a
endif
if ENABLE_IPV6
ruletablesi_SOURCES += ip6tables.c
ruletables_CFLAGS  += -DENABLE_IPV6
ruletables_LDADD   += ../libiptc/libip6tc.la ../extensions/libext6.a
endif
ruletables_SOURCES += xshared.c
ruletables_LDADD   += ../libxtables/libxtables.la -lm

ruletables_SOURCES += ruletables.c conn.c operate_tables.c

sbin_PROGRAMS    = ruletables

pkgconfig_DATA = xtables.pc

# Using if..fi avoids an ugly "error (ignored)" message :)
install-exec-hook:
	-if test -z "${DESTDIR}"; then /sbin/ldconfig; fi;
	${INSTALL} -dm0755 "${DESTDIR}${bindir}";
	for i in ${vx_bin_links}; do ${LN_S} -f "${sbindir}/xtables-multi" "${DESTDIR}${bindir}/$$i"; done;
	for i in ${v4_sbin_links}; do ${LN_S} -f xtables-multi "${DESTDIR}${sbindir}/$$i"; done;
	for i in ${v6_sbin_links}; do ${LN_S} -f xtables-multi "${DESTDIR}${sbindir}/$$i"; done;
