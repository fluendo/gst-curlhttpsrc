# Libtool isn't very helpful when it comes to link partially a set of static
# libraries. This utility helps to create a link command where all the gstreamer
# libraries and gstreamer plugins are statically linked while the other dependencies
# are linked dynamically. It takes as input a list of gstreamer plugins
# in the form of -lgstfoo and returns the output to be used in the link
# command

# -----------------------------------------------------------------------------
# Function : libtool-link
# Arguments: 1: list of GStreamer plugins to link
#            2: list of Libraries
#            3: search path for gstreamer static plugins
#            4: Set to 1 to link GStreamer libs static
# Returns  : a link command with all the dependencies resolved as done by libtool
# Usage    : $(call libtool-link,<lib>)
# -----------------------------------------------------------------------------
define libtool-link
  $(call libtool-clear-vars)\
  $(if $(findstring 1, $4),\
    $(call __libtool_log, Linking with static gstreamer $4)\
    $(eval __libtool.static_gst_lib := "yes"),\
    $(call __libtool_log, Linking with dynamic gstreamer $4)\
  )\
  $(eval __libtool.gst_plugins := $(patsubst %,gst%, $1))\
  $(eval __libtool.link.command := $(patsubst %,-lgst%, $1) $2 -L$3)\
  $(call __libtool_log, original link command = $(__libtool.link.command))\
  $(eval __libtool.link.Lpath := $(call libtool-get-search-paths,$(__libtool.link.command)))\
  $(call __libtool_log, Library Search Paths = $(__libtool.link.Lpath))\
  $(eval __libtool.link.libs := $(call libtool-get-libs,$(__libtool.link.command)))\
  $(call __libtool_log, Libraries = $(__libtool.link.libs))\
  $(foreach library,$(__libtool.link.libs),$(call libtool-parse-lib,$(library)))\
  $(call libtool-gen-link-command)
endef


###############################################################################
#                                                                             #
#            These functions are private, don't use them directly             #
#                                                                             #
###############################################################################

# -----------------------------------------------------------------------------
# Function : libtool-parse-library
# Arguments: 1: library name
# Returns  : ""
# Usage    : $(call libtool-parse-library,<libname>)
# Note     : Tries to find a libtool library for this name in the libraries search
#            path and parses it as well as its dependencies
# -----------------------------------------------------------------------------
define libtool-parse-lib
  $(eval __tmpvar := $(strip $(call libtool-find-lib,$(patsubst -l%,%,$1))))\
  $(if $(__tmpvar), \
    $(call libtool-parse-file,$(__tmpvar),$(strip $(call libtool-name-from-filepath,$(__tmpvar)))),\
    $(call __libtool_log, libtool file not found for "$1" and will be added to the shared libs)\
    $(if $(findstring __-l, __$1),\
      $(eval __libtool.link.shared_libs += $1),\
      $(eval __libtool.link.shared_libs += -l$1)\
    )\
  )
endef

# -----------------------------------------------------------------------------
# Function : libtool-parse-file
# Arguments: 1: libtool file
#            2: library name
# Returns  : ""
# Usage    : $(call libtool-parse-file,<file>,<libname>)
# Note     :
#            Parses a libtool library and its dependencies recursively
#
#            For each library it sets the following variables:
#            __libtool_libs.libname.LIBS              -> non-libtool libraries linked with -lfoo
#            __libtool_libs.libname.STATIC_LIB        -> link statically this library
#            __libtool_libs.libname.DYN_LIB           -> link dynamically this library
#            __libtool_libs.libname.LIBS_SEARCH_PATH  -> libraries search path
#
#            Processed libraries are stored in __libtool_libs.processed, and
#            the list of libraries ordered by dependencies are stored in
#            __libtool_lbs.ordered
# -----------------------------------------------------------------------------
define libtool-parse-file
  $(call __libtool_log, parsing file $1)\
  $(if $(strip $(call libtool-lib-processed,$2)),\
      $(call __libtool_log, library "$2" already parsed),\
    $(eval __libtool_libs.$2.STATIC_LIB := $(patsubst %.la,%.a,$1))\
    $(eval __libtool_libs.$2.DYN_LIB := -l$2)\
    $(eval __libtool_libs.$2.FLAGS := $(call libtool-get-inherited-linker-flags,$1))\
    $(eval __tmpvar.$2.dep_libs := $(call libtool-get-dependency-libs,$1))\
    $(eval __tmpvar.$2.dep_libs := $(call libtool-replace-prefixes,$(__tmpvar.$2.dep_libs)))\
    $(eval __libtool_libs.$2.LIBS := $(call libtool-get-libs,$(__tmpvar.$2.dep_libs)))\
    $(eval __libtool_libs.$2.LIBS_SEARCH_PATH := $(call libtool-get-search-paths,$(__tmpvar.$2.dep_libs)))\
    $(call __libtool_log, $2.libs = $(__libtool_libs.$2.LIBS))\
    $(eval __tmpvar.$2.file_deps := $(call libtool-get-libtool-deps,$(__tmpvar.$2.dep_libs)))\
    $(eval __libtool_libs.$2.DEPS := $(foreach path,$(__tmpvar.$2.file_deps), $(call libtool-name-from-filepath,$(path))))\
    $(call __libtool_log, $2.deps = $(__libtool_libs.$2.DEPS)) \
    $(eval __libtool_libs.processed += $2) \
    $(call __libtool_log, parsed libraries: $(__libtool_libs.processed))\
    $(foreach library,$(__libtool_libs.$2.DEPS), $(call libtool-parse-lib,$(library)))\
    $(eval __libtool_libs.ordered += $2)\
    $(call __libtool_log, ordered list of libraries: $(__libtool_libs.ordered))\
  )
endef

define __libtool_log
  $(if $(strip $(LIBTOOL_DEBUG)),\
    $(call __libtool_info,$1),\
  )
endef

define __libtool_info
 $(info LIBTOOL: $1)
endef

define libtool-clear-vars
  $(foreach lib,$(__libtool_libs.processed),\
    $(eval __libtool_libs.$(lib).LIBS := $(empty))\
    $(eval __libtool_libs.$(lib).STATIC_LIB := $(empty))\
    $(eval __libtool_libs.$(lib).DYN_LIB := $(empty))\
    $(eval __libtool_libs.$(lib).LIBS_SEARCH_PATH := $(empty))\
  )\
  $(eval __libtool_libs.ordered := $(empty))\
  $(eval __libtool_libs.processed := $(empty))\
  $(eval __libtool.link.Lpath := $(empty))\
  $(eval __libtool.link.command := $(empty))\
  $(eval __libtool.link.libs := $(empty))\
  $(eval __libtool.link.shared_libs := $(empty))
endef

define libtool-lib-processed
  $(findstring ___$1___, $(foreach lib,$(__libtool_libs.processed), ___$(lib)___))
endef

define libtool-gen-link-command
  $(eval __tmpvar.cmd := $(filter-out -L%,$(__libtool.link.command)))\
  $(eval __tmpvar.cmd := $(filter-out -l%,$(__tmpvar.cmd)))\
  $(eval __tmpvar.cmd += $(__libtool.link.Lpath))\
  $(eval __tmpvar.cmd += $(call libtool-get-libs-search-paths))\
  $(eval __tmpvar.cmd += $(call libtool-get-all-libs))\
  $(eval __tmpvar.cmd += $(call libtool-get-flags))\
  $(eval __tmpvar.cmd += $(__libtool.link.shared_libs))\
  $(call __libtool_log, "Link Command:" $(__tmpvar.cmd))\
  $(__tmpvar.cmd)
endef

define libtool-get-libs-search-paths
  $(eval __tmpvar.paths := $(empty))\
  $(foreach library,$(__libtool_libs.ordered),\
    $(foreach path,$(__libtool_libs.$(library).LIBS_SEARCH_PATH),\
      $(if $(findstring $(path), $(__tmpvar.paths)), ,\
        $(eval __tmpvar.paths += $(subst =,, $(path)))\
      )\
    )\
  )\
  $(call __libtool_log, search paths $(__tmpvar.paths))\
  $(strip $(__tmpvar.paths))
endef

define libtool-get-libs-search-paths
  $(eval __tmpvar.flags := $(empty))\
  $(foreach library,$(__libtool_libs.ordered),\
    $(foreach flag,$(__libtool_libs.$(library).FLAGS),\
      $(if $(findstring $(flag), $(__tmpvar.flags)), ,\
        $(eval __tmpvar.flags += $(subst =,, $(flag)))\
      )\
    )\
  )\
  $(call __libtool_log, flags $(__tmpvar.flags))\
  $(strip $(__tmpvar.flags))
endef

define libtool-get-all-libs
  $(eval __tmpvar.static_libs_reverse := $(empty))\
  $(eval __tmpvar.static_libs := $(empty))\
  $(eval __tmpvar.libs := $(empty))\
  $(foreach library,$(__libtool_libs.ordered),\
    $(if $(findstring gst, $(library)),\
      $(call __libtool_log, Found GStreamer library $(library))\
      $(if $(__libtool.static_gst_lib),\
        $(call __libtool_log, Linking lib as static)\
        $(eval __tmpvar.static_libs_reverse += $(__libtool_libs.$(library).STATIC_LIB)),\
        $(if $(findstring $(library), $(__libtool.gst_plugins)),\
          $(call __libtoo Adding plugin to satic)\
          $(eval __tmpvar.static_libs_reverse += $(__libtool_libs.$(library).STATIC_LIB)),\
          $(call __libtoo Adding library to dynamic)\
          $(eval __tmpvar.libs += -l$(library)))),\
      $(call __libtoo Adding library to dynamic)\
      $(eval __tmpvar.libs += -l$(library))\
    )\
    $(foreach dylib,$(__libtool_libs.$(library).LIBS),\
      $(if $(findstring $(dylib), $(__tmpvar.libs)), ,\
        $(eval __tmpvar.libs += $(dylib))\
      )\
    )\
  )\
  $(foreach path,$(__tmpvar.static_libs_reverse),\
    $(call __libtool_log, static libs $(__tmpvar.static_libs))\
    $(eval __tmpvar.static_libs := $(path) $(__tmpvar.static_libs))\
  )\
  $(strip $(__tmpvar.static_libs) $(__tmpvar.libs))
endef

define libtool-find-lib
  $(eval __tmpvar := $(empty))\
  $(foreach path,$(__libtool.link.Lpath),\
    $(eval __tmpvar += $(wildcard $(patsubst -L%,%,$(path))/lib$1.la))\
  ) \
  $(firstword $(__tmpvar))
endef

define libtool-name-from-filepath
  $(patsubst lib%.la,%,$(notdir $1))
endef

define libtool-get-libtool-deps
  $(filter %.la,$1)
endef

define libtool-get-deps
  $(filter %.la,$1)
endef

define libtool-get-libs
  $(filter -l%,$1)
endef

define libtool-get-search-paths
  $(filter -L%,$1)
endef

define libtool-get-dependency-libs
  $(shell sed -n "s/^dependency_libs='\(.*\)'/\1/p" $1)
endef

define libtool-get-inherited-linker-flags
  $(shell sed -n "s/^inherited_linker_flags='\(.*\)'/\1/p" $1)
endef

define libtool-replace-prefixes
  $(subst $(BUILD_PREFIX),$(prefix),$1 )
endef

define libtool-get-static-library
  $(shell sed -n "s/^old_library='\(.*\)'/\1/p" $1)
endef
