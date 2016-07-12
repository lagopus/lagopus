.SUFFIXES:	.a .la .ln .o .lo .s .S .c .cc .cpp .i .y .l

.c.o:
	LC_ALL=C $(CC) $(CFLAGS) $(CPPFLAGS) -c $(srcdir)/$< -o $@

.c.lo:
	$(LTCOMPILE_CC) -c $(srcdir)/$< -o $@

.c.i:
	LC_ALL=C $(CC) -E $(CPPFLAGS) $(srcdir)/$*.c | uniq > $(srcdir)/$*.i

.cc.o:
	LC_ALL=C $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $(srcdir)/$< -o $@

.cc.lo:
	$(LTCOMPILE_CXX) -c $(srcdir)/$< -o $@

.cc.i:
	LC_ALL=C $(CXX) -E $(CPPFLAGS) $(srcdir)/$*.cc | uniq > $(srcdir)/$*.i

.cpp.o:
	LC_ALL=C $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $(srcdir)/$< -o $@

.cpp.lo:
	$(LTCOMPILE_CXX) -c $(srcdir)/$< -o $@

.cpp.i:
	LC_ALL=C $(CXX) -E $(CPPFLAGS) $(srcdir)/$*.cpp | uniq > $(srcdir)/$*.i

.s.lo:
	$(LTCOMPILE_CC) -c $(srcdir)/$*.s

.s.o:
	LC_ALL=C $(CC) $(CFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.s

.S.lo:
	$(LTCOMPILE_CC) -c $(srcdir)/$*.S

.S.o:
	LC_ALL=C $(CC) $(CFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.S

.c.s:
	LC_ALL=C $(CC) -S $(CFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.c \
		-o $(srcdir)/$*.s

.cc.s:
	LC_ALL=C $(CXX) -S $(CXXFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.cc \
		-o $(srcdir)/$*.s

.cpp.s:
	LC_ALL=C $(CXX) -S $(CXXFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.cpp \
		-o $(srcdir)/$*.s

.y.c:
	$(YACC) -d -o $@ $?

.l.c:
	$(LEX) -o $@ $?

all::		ALL
ALL::		symlink $(TARGETS)

ifdef INSTALL_EXE_TARGETS
install::	install-exe
install-exe::	$(INSTALL_EXE_TARGETS)
	@if test ! -z "$(INSTALL_EXE_TARGETS)" -a \
		 ! -z "$(INSTALL_EXE_DIR)" ; then \
		$(MKDIR) $(INSTALL_EXE_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_EXE_TARGETS) ; do \
			$(LTINSTALL_EXE) $$i $(INSTALL_EXE_DIR) ; \
		done ; \
	fi
else
install::	install-exe
install-exe::
	@true
endif

ifdef INSTALL_SBIN_EXE_TARGETS
install::	install-sbin-exe
install-sbin-exe::	$(INSTALL_SBIN_EXE_TARGETS)
	@if test ! -z "$(INSTALL_SBIN_EXE_TARGETS)" -a \
		 ! -z "$(INSTALL_SBIN_EXE_DIR)" ; then \
		$(MKDIR) $(INSTALL_SBIN_EXE_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_SBIN_EXE_TARGETS) ; do \
			$(LTINSTALL_EXE) $$i $(INSTALL_SBIN_EXE_DIR) ; \
		done ; \
	fi
else
install::	install-sbin-exe
install-sbin-exe::
	@true
endif

ifdef INSTALL_LIB_TARGETS
install::	install-lib
install-lib::	$(INSTALL_LIB_TARGETS)
	@if test ! -z "$(INSTALL_LIB_TARGETS)" -a \
		 ! -z "$(INSTALL_LIB_DIR)" ; then \
		$(MKDIR) $(INSTALL_LIB_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_LIB_TARGETS) ; do \
			$(LTINSTALL_LIB) $$i $(INSTALL_LIB_DIR) ; \
		done ; \
	fi
else
install::	install-lib
install-lib::
	@true
endif

ifdef INSTALL_HEADER_TARGETS
install::	install-header
install-header::	$(INSTALL_HEADER_TARGETS)
	@if test ! -z "$(INSTALL_HEADER_TARGETS)" -a \
		 ! -z "$(INSTALL_HEADER_DIR)" ; then \
		$(MKDIR) $(INSTALL_HEADER_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_HEADER_TARGETS) ; do \
			$(LTINSTALL_HEADER) $$i $(INSTALL_HEADER_DIR) ; \
		done ; \
	fi
else
install::	install-header
install-header::
	@true
endif

ifdef INSTALL_CONFIG_TARGETS
install::	install-config
install-config::	$(INSTALL_CONFIG_TARGETS)
	@if test ! -z "$(INSTALL_CONFIG_TARGETS)" -a \
		 ! -z "$(INSTALL_CONFIG_DIR)" ; then \
		$(MKDIR) $(INSTALL_CONFIG_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_CONFIG_TARGETS) ; do \
			$(INSTALL_DATA) $$i $(INSTALL_CONFIG_DIR) ; \
		done ; \
	fi
else
install::	install-config
install-config::
	@true
endif

ifdef INSTALL_DOC_TARGETS
install::	install-doc
install-doc::	$(INSTALL_DOC_TARGETS)
	@if test ! -z "$(INSTALL_DOC_TARGETS)" -a \
		 ! -z "$(INSTALL_DOC_DIR)" ; then \
		$(MKDIR) $(INSTALL_DOC_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_DOC_TARGETS) ; do \
			$(INSTALL_DATA) $$i $(INSTALL_DOC_DIR) ; \
		done ; \
	fi
else
install::	install-doc
install-doc::
	@true
endif

ifdef INSTALL_EXAMPLE_TARGETS
install::	install-example
install-example::	$(INSTALL_EXAMPLE_TARGETS)
	@if test ! -z "$(INSTALL_EXAMPLE_TARGETS)" -a \
		 ! -z "$(INSTALL_EXAMPLE_DIR)" ; then \
		$(MKDIR) $(INSTALL_EXAMPLE_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_EXAMPLE_TARGETS) ; do \
			$(INSTALL_DATA) $$i $(INSTALL_EXAMPLE_DIR) ; \
		done ; \
	fi
else
install::	install-example
install-example::
	@true
endif

ifdef SRCS
depend:: symlink generate
	@if test ! -z "$(SRCS)" -a -z "$(TESTS)" -o ! -z "$(HAVE_UNITY)" ; then \
		> .depend ; \
		$(CC) -M $(CPPFLAGS) $(SRCS) | sed 's:\.o\::\.lo\::' \
		> .depend ; \
		if test $$? -ne 0 ; then \
			echo depend in `pwd` failed. ; \
		else \
			echo depend in `pwd` succeeded. ; \
		fi ; \
	fi
else
depend::
	@true
endif

ifdef SYMLINKS
symlink::
	@( \
		for i in $(SYMLINKS); do \
			filename=`$(BASENAME) $$i`; \
			link_target=`$(READLINK) $$filename`; \
			if test ! -f "$$filename" -o \
				\( -f "$$filename" -a $$i != "$$link_target" \); then \
				$(LN_S) $$i $$filename; \
				if test $$? -ne 0 ; then \
					echo symlink failed. : $$i; \
				else \
					echo symlink succeeded. : $$i; \
				fi; \
			fi; \
		done; \
	)
else	# SYMLINK
symlink::
	@true
endif	# SYMLINK

# create debian packages
PKGDIR = $(TOPDIR)/pkg

pkg-clean::
	$(RM) -rf $(PKGDIR)/

pkg-deb:: clean
	sh $(MKRULESDIR)/make_pkg.sh $(TOPDIR) $(PKGDIR) $(RTE_TARGET)

ifdef TARGET_LIB
$(TARGET_LIB):	$(OBJS)
	$(RM) -f $@ .libs/$(@F:.la=.*)
	$(LTLIB_CC) -o $@ $(OBJS) $(LDFLAGS) $(DEP_LIBS)
endif

ifdef TARGET_EXE
$(TARGET_EXE):	$(OBJS)
	$(RM) -f $@ .libs/lt-$@ .libs/$@
	$(LTLINK_CC) -o $@ $(OBJS) $(DEP_LIBS) $(LDFLAGS) 
endif

$(DEP_LAGOPUS_UTIL_LIB)::
	(cd $(BUILD_LIBDIR) && $(MAKE) $(LAGOPUS_UTIL_LIB))

$(DEP_LAGOPUS_DATAPLANE_LIB)::
	(cd $(BUILD_DATAPLANEDIR) && $(MAKE) $(LAGOPUS_DATAPLANE_LIB))

$(DEP_LAGOPUS_DATAPLANE_TEST_LIB)::
	(cd $(BUILD_DATAPLANETESTLIBDIR) && $(MAKE) $(LAGOPUS_DATAPLANE_TEST_LIB))

$(DEP_LAGOPUS_AGENT_LIB)::
	(cd $(BUILD_AGENTDIR) && $(MAKE) $(LAGOPUS_AGENT_LIB))

$(DEP_LAGOPUS_DATASTORE_LIB)::
	(cd $(BUILD_DATASTOREDIR) && $(MAKE) $(LAGOPUS_DATASTORE_LIB))

$(DEP_LAGOPUS_CONFIG_LIB)::
	(cd $(BUILD_CONFIGDIR) && $(MAKE) $(LAGOPUS_CONFIG_LIB))

$(DEP_SNMP_HANDLER_LIB)::
	(cd $(BUILD_SNMPDIR) && $(MAKE) $(SNMP_HANDLER_LIB))

# Obsolete/Depricated modules:
#---------------------------------------------------------------------
#$(DEP_LAGOPUS_OFCONF_LIB)::
#	(cd $(BUILD_OFCONFDIR) && $(MAKE) $(LAGOPUS_OFCONF_LIB))

#$(DEP_LAGOPUS_OVSDB_LIB)::
#	(cd $(BUILD_OVSDBDIR) && $(MAKE) $(LAGOPUS_OVSDB_LIB))

#$(DEP_LAGOPUS_ETHOAM_LIB)::
#	(cd $(BUILD_ETHOAMDIR) && $(MAKE) $(LAGOPUS_ETHOAM_LIB))
#---------------------------------------------------------------------

ifneq ($(RTE_SDK),)

dpdk::
	$(MKRULESDIR)/make_dpdk.sh ${TOPDIR} src/dpdk \
		"${RTE_ARCH}" "${RTE_OS}" ${CC}

dpdk-install::
	$(INSTALL_DATA) $(TOPDIR)/src/dpdk/build/lib/libdpdk.so $(DEST_LIBDIR)
	$(MKDIR) $(DEST_LIBDIR)/dpdk-pmd
	for i in `sed 's/GROUP (\(.*\))/\1/' $(TOPDIR)/src/dpdk/build/lib/libdpdk.so`; do \
	  if [ -f $(TOPDIR)/src/dpdk/build/lib/$$i ]; then \
	    $(INSTALL_DATA) $(TOPDIR)/src/dpdk/build/lib/$$i $(DEST_LIBDIR); \
	    s=`echo $$i | grep librte_pmd_`;\
		if [ -n "$$s" ]; then \
		    $(LN_S) ../$$i $(DEST_LIBDIR)/dpdk-pmd/$$i; \
		fi \
	  fi \
	done

dpdk-clean::
	@if test -r "${TOPDIR}/src/dpdk/build/.config"; then \
		cd ${TOPDIR}/src/dpdk && $(MAKE) clean; \
	fi

$(DEP_DPDK_LIB)::
	@if test ! -f "$(DEP_DPDK_LIB)"; then \
		$(MAKE) dpdk; \
	fi

else

dpdk::
	@true

dpdk-install::
	@true

dpdk-clean::
	@true

endif

submodules::	$(SUBMODULES)
	@true

.PHONY:	prerequisite
prerequisite::
	@( \
		$(MAKE) clean && \
		$(MAKE) generate && \
		$(MAKE) submodules && \
		$(MAKE) depend; \
		exit $$?; \
	)

clean:: pkg-clean
	$(LTCLEAN) $(OBJS) *.i *~ *.~*~ core core.* *.core $(TARGETS) \
		*.o *.lo *.gcda *.gcno *.gcov
	$(RM) -rf ./html ./scan-result ./clang.mk ./scan-build.mk ./icc.mk \
	./fortify.mk ./fortify.fpr ./fortify.rtf ./fortify.pdf \
	./und.mk ./und.txt ./und.udb ./und_html ./cov \
	*gcov.mk *.gcno *.gcov gcovr_result.xml .libs

distclean::	clean symlink-clean
	$(RM) Makefile .depend

symlink-clean::
	@find ./ -type l | xargs ${RM}

beautify-for-py::
	@find . -type f -name '*.py' -o -name '*.py.in'| \
	egrep -v 'src/wip-or-deprecate|test/AutomaticVerificationTool' | \
	xargs sh $(MKRULESDIR)/beautify_for_py

beautify:: beautify-for-py
	@sh $(MKRULESDIR)/beautify 'src/*.c' 'src/*.cpp' 'src/*.h'

revert::
	@git status . | grep modified: | awk '{ print $$NF }' | \
	xargs git checkout

Makefiles::
	@if test -x $(TOPDIR)/config.status; then \
		(cd $(TOPDIR); LC_ALL=C sh ./config.status; \
			$(RM) config.log) ; \
	fi

ifdef LIBTOOL_DEPS
libtool::	$(LIBTOOL_DEPS)
	@if test -x $(TOPDIR)/config.status; then \
		(cd $(TOPDIR); LC_ALL=C sh ./config.status libtool; \
			$(RM) config.log) ; \
	fi
endif

.PHONY: check-syntax
check-syntax:
	${CC} -Wall -fsyntax-only $(CHK_SOURCES)

doxygen::
	sh $(MKRULESDIR)/mkfiles.sh
	$(RM) -rf ./html ./latex
	doxygen $(MKRULESDIR)/doxygen.conf
	$(RM) -rf ./latex ./.files

fortify::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./fortify.mk ./fortify.fpr ; \
		echo 'ifeq ($$(__SITECONF__),.pre.)' > ./fortify.mk ; \
		echo 'ORGCC=$(CC)' >> ./fortify.mk ; \
		echo 'endif' >> ./fortify.mk ; \
		echo 'ifeq ($$(__SITECONF__),.post.)' >> ./fortify.mk ; \
		echo 'CC=sourceanalyzer -scan -f $$(TOPDIR)/fortify.fpr -append $$(ORGCC)' >> ./fortify.mk ; \
		echo 'LINK_CC=$$(ORGCC)' >> ./fortify.mk ; \
		echo 'endif' >> ./fortify.mk ; \
		(SITECONF_MK=`pwd`/fortify.mk $(MAKE)) ; \
		$(RM) -f ./fortify.mk ; \
		if test -f ./fortify.fpr; then \
			ReportGenerator -format rtf -source ./fortify.fpr \
				-f ./fortify.rtf ; \
		fi ; \
	)

scan-build::
	@( \
		$(MAKE) prerequisite > /dev/null 2>&1 ; \
		$(RM) -rf ./scan-build.mk ./scan-result ; \
		scan-build sh -c 'echo CC = $${CC} > ./scan-build.mk' \
			> /dev/null 2>&1 ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/scan-build.mk \
				scan-build -o ./scan-result \
				sh -c "$(MAKE)" ; ) ; \
		fi ; \
		$(RM) -f ./scan-build.mk ; \
	)

scan-build-blame::
	@( \
		$(RM) -f ./m.out ; \
		$(MAKE) scan-build > ./m.out 2>&1 ; \
		if test $$? -eq 0 -a -r ./m.out ; then \
			sh $(MKRULESDIR)/warn-blame.sh ./m.out ; \
		fi ; \
		$(RM) -f ./m.out ; \
	)

gcc-full-opt::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./gcc-full-opt.mk ; \
		echo "DEBUG_CFLAGS = -g0" > ./gcc-full-opt.mk ; \
		echo "DEBUG_CXXFLAGS = -g0" >> ./gcc-full-opt.mk ; \
		echo "OPT_CFLAGS = -O6" >> ./gcc-full-opt.mk ; \
		echo "OPT_CXXFLAGS = -O6" >> ./gcc-full-opt.mk ; \
		echo "CODEGEN_CFLAGS = -fno-keep-inline-functions" >> ./gcc-full-opt.mk ; \
		echo "CODEGEN_CXXFLAGS = -fno-keep-inline-functions" >> ./gcc-full-opt.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/gcc-full-opt.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./gcc-full-opt.mk ; \
	)

clang::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./clang.mk ; \
		echo "CC = clang" > ./clang.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/clang.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./clang.mk ; \
	)

icc::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./icc.mk ; \
		echo "CC = icc" > ./icc.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/icc.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./icc.mk ; \
	)

gcov::
	@( \
		echo "CFLAGS += -fprofile-arcs -ftest-coverage" > ./gcov.mk ; \
		echo "LDFLAGS += -fprofile-arcs" >> ./gcov.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/gcov.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./gcov.mk ; \
	)

und::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./und.mk ./und.udb ./und.txt und_html ; \
		echo "CC = gccwrapper" > ./und.mk ; \
		if test $$? -eq 0; then \
			(LC_ALL=C SITECONF_MK=`pwd`/und.mk \
				buildspy -db `pwd`/und.udb -cmd make) ; \
		fi ; \
		if test -f ./und.udb; then \
			und -db `pwd`/und.udb analyze ; \
		fi ; \
		if test $$? -eq 0; then \
			und -db `pwd`/und.udb report ; \
		fi ; \
		$(RM) -f ./und.mk ; \
	)

cov::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./cov ; \
		cov-build --dir ./cov sh -c "$(MAKE)" > /dev/null 2>&1 && \
		cov-analyze --dir ./cov --all > /dev/null 2>&1 && \
		cov-format-errors --dir ./cov > /dev/null 2>&1 ; \
	)

wc::
	@find . -type f -name '*.c' -o -name '*.cpp' -o -name '*.h' | \
	xargs wc

warn-check::
	@( \
		$(MAKE) prerequisite > /dev/null 2>&1 ; \
		$(MAKE) > ./m.out 2>&1 ; \
		if test $$? -eq 0 -a -f ./m.out ; then \
			grep ' warning:' ./m.out | awk -F: '{ print $$1 }' | \
			sort | uniq ; \
		fi ; \
		$(RM) -f ./m.out ; \
	)

warn-blame::
	@( \
		$(MAKE) prerequisite > /dev/null 2>&1 ; \
		$(MAKE) > ./m.out 2>&1 ; \
		if test $$? -eq 0 -a -f ./m.out ; then \
			sh $(MKRULESDIR)/warn-blame.sh ./m.out ; \
		fi ; \
		$(RM) -f ./m.out ; \
	)

dostext::
	sh $(MKRULESDIR)/doDosText.sh

summary::
	@ if test `pwd` == $(TOPDIR) ; then\
		mkdir -p $(TEST_RESULT_DIR); \
	fi

ifdef DIRS
all depend clean distclean check check-nocolor valgrind helgrind install install-exe install-lib install-header install-config install-sbin exe symlink generate benchmark::
	@for i in / $(DIRS) ; do \
		case $$i in \
			/) continue ;; \
			*) (cd $$i && $(MAKE) $@) || exit 1;; \
		esac ; \
	done
endif

summary:: check-nocolor
	@if test `pwd` = $(TOPDIR) ; then\
		cd $(TEST_RESULT_DIR) ; \
		$(RUBY) $(UNITY_SUMMARY) ; \
		cd $(TOPDIR); \
	fi

check-nocolor::
	@true

check::
	@true

summary::
	@true

valgrind::
	@true

helgrind::
	@true

generate::
	@true

benchmark::
	@true

ifdef HAVE_UNITY
ifdef TESTS

CPPFLAGS += $(UNITY_CPPFLAGS)
LOCAL_CFLAGS	+=	-Wno-missing-prototypes -Wno-missing-declarations

$(DEP_UNITY_LIB)::
	(cd $(BUILDUNITY_DIR); $(MAKE) $(UNITY_LIB))

depend::
	@for i in $(TESTS) ; do \
		echo "$${i}:$${i}.lo $${i}_runner.lo \$$(TEST_DEPS) \$$(DEP_UNITY_LIB)" >> .depend; \
		echo '	$$(RM) -f $$@ .libs/lt-$$@ .libs/$$@' >> .depend; \
		echo '	$$(LTLINK_CC) -o $$@ $$^ $$(DEP_LIBS) $$(LDFLAGS)' >> .depend; \
	done

clean::
	$(LTCLEAN) $(TESTS) *_runner.c *_runner.o *_runner.lo

ifdef SRCS
depend::
	@for i in $(SRCS) ; do \
		case $$i in \
			*_test.c) echo "`echo $$i | sed 's/_test.c/_test_runner.c/g'`:$$i" >> .depend; \
				  echo '	$$(RUBY) $$(UNITY_GEN_RUNNER) $$< $$@' >> .depend;; \
			*) continue ;; \
		esac; \
	done
else
depend::
	@true
endif		# SRCS

all:: $(TESTS)

check-nocolor:: TEST_RESULT_COLOR='false'
check-nocolor:: check

check:: gcov
	@ for runner in $(TESTS); do \
		./$${runner} > ./$${runner}.testresult; \
		ret=$$?; \
		if test $$ret -ge 128; then \
			echo "$${runner}:::FAIL: exit status $$ret" >> ./$${runner}.testresult; \
			testcount=`cat ./$${runner}.testresult|wc -l`; \
			failcount=`grep ':FAIL' ./$${runner}.testresult|wc -l`; \
			echo "-----------------------" >> ./$${runner}.testresult; \
			echo "$$testcount Tests $$failcount Failures 0 Ignored" >> ./$${runner}.testresult; \
			echo "FAIL" >> ./$${runner}.testresult; \
		fi; \
		$(RUBY) -r$(UNITY_REPORT) \
			-e '$$colour_output=$(TEST_RESULT_COLOR)' \
			-e 'report $$stdin.readlines.join' < ./$${runner}.testresult; \
		if test -d $(TEST_RESULT_DIR) ; then \
			if test $$ret -eq 0; then \
				mv ./$${runner}.testresult $(TEST_RESULT_DIR)/$${runner}.testpass; \
			else \
				mv ./$${runner}.testresult $(TEST_RESULT_DIR)/$${runner}.testfail; \
			fi; \
		fi; \
	done

valgrind::	$(TESTS)
	@for i in $(TESTS) ; do \
		if test -x ./.libs/$${i} ; then \
			rm -f ./vgcore.* ; \
			LD_LIBRARY_PATH=$(UNITY_DIR)/src/.libs:$(BUILD_LIBDIR)/.libs:$(BUILD_AGENTDIR)/.libs:$(BUILD_DATAPLANEDIR)/.libs:$(BUILD_CONFIGDIR)/.libs:$(BUILD_EDITDIR)/.libs:$(BUILD_OFCONFDIR)/.libs:$(BUILD_OVSDBDIR)/.libs:$(BUILD_ETHOAMDIR)/.libs valgrind \
				-v \
				--leak-check=full \
				--track-origins=yes \
				./.libs/$${i} ; \
			if test $$? -ne 0 -a -f ./vgcore.* ; then \
				mv ./vgcore.* $${i}.core ; \
			fi ; \
		fi ; \
	done

helgrind::	$(TESTS)
	@for i in $(TESTS) ; do \
		if test -x ./.libs/$${i} ; then \
			rm -f ./vgcore.* ; \
			LD_LIBRARY_PATH=$(UNITY_DIR)/src/.libs:$(BUILD_LIBDIR)/.libs:$(BUILD_AGENTDIR)/.libs:$(BUILD_DATAPLANEDIR)/.libs:$(BUILD_CONFIGDIR)/.libs:$(BUILD_EDITDIR)/.libs:$(BUILD_OFCONFDIR)/.libs:$(BUILD_OVSDBDIR)/.libs:$(BUILD_ETHOAMDIR)/.libs valgrind --tool=helgrind \
				-v \
				./.libs/$${i} ; \
			if test $$? -ne 0 -a -f ./vgcore.* ; then \
				mv ./vgcore.* $${i}.core ; \
			fi ; \
		fi ; \
	done

clean::
	rm -f *.testresult

endif			# TESTS

clean::
	@ if test `pwd` = $(TOPDIR); then rm -rf $(TEST_RESULT_DIR); else true; fi

else			# HAVE_UNITY
check::
	@echo "HINT:" 1>&2
	@echo "  unit tests are not available" 1>&2
	@echo "  you should configure with --enable-developer for unit tests" 1>&2
	@false

valgrind::	check

endif			# HAVE_UNITY

ifdef HAVE_GCOVR
coverage coverage-xmldump::
	@(find -name '*.gcda' | xargs -r $(RM))

coverage:: check-nocolor
	@gcovr -r $(TOPDIR) --gcov-exclude="(unity|.*runner|.*_test)"

coverage-xmldump:: check-nocolor
	@gcovr --xml --output=gcovr_result.xml -r $(TOPDIR) --gcov-exclude="(.*unity|.*runner|.*_test)"

summary::
	@gcovr --xml --output=gcovr_result.xml -r $(TOPDIR) --gcov-exclude="(.*unity|.*runner|.*_test)"

else
clean-gcda coverage coverage-xmldump::
	@echo "HINT:" 1>&2
	@echo "  taking coverage requires gcovr" 1>&2
endif
