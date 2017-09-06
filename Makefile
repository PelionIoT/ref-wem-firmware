PROG:=fota-demo
CURDIR:=$(shell pwd)

# We use some bashisms like pipefail.  The default GNU Make SHELL is /bin/sh
# which is bash on MacOS but not necessarily on Linux.  Explicitly set bash as
# the SHELL here.
SHELL=/bin/bash

# Specify the default target and toolchain to build.  The defaults are used
# if 'mbed target' and 'mbed toolchain' are not set.
DEFAULT_TARGET:=K64F
DEFAULT_TOOLCHAIN:=GCC_ARM

SRCDIR:=.
PATCHDIR:=${CURDIR}/patches
SRCS:=$(wildcard $(SRCDIR)/*.cpp)
HDRS:=$(wildcard $(SRCDIR)/*.h)
LIBS:=$(wildcard $(SRCDIR)/*.lib)

# The bootloader type and name
BOOTLDR_DIR:=mbed-bootloader
BOOTLDR_PROG:=${BOOTLDR_DIR}

BINDIR:=bin
BOOTLDR_BIN:=${BINDIR}/${BOOTLDR_PROG}.bin
PROG_BIN:=${BINDIR}/${PROG}.bin
# This file is the combination of the bootloader and the app.
# Flash this file via the USB interface, then perform FOTA with
# the app (PROG) binary.
COMBINED_BIN:=${BINDIR}/combined.bin

# Specify the path to the build profile.  If empty, the --profile option will
# not be provided to 'mbed compile' which causes it to use the builtin default.
ifeq (${DEBUG}, )
	BUILD_PROFILE:=mbed-os/tools/profiles/release.json
else
	BUILD_PROFILE:=mbed-os/tools/profiles/debug.json
endif

# Capture the GIT version that we are building from.  Later in the compile
# phase, this value will get built into the code.
DEVTAG:="$(shell git rev-parse --short HEAD)-$(shell git rev-parse --abbrev-ref HEAD)"
ifneq ("$(shell git status -s)","")
	DEVTAG:="${DEVTAG}-dev-${USER}"
endif

# Specifies the name of the target board to compile for
#
# The following methods are checked for a target board type, in order:
# 1. 'mbed target'.  To specify a target using this mechanism, run
# 	'mbed target <target>' in your build environment.
# 2. 'mbed detect'
# 3. otherwise a default target is used as specified at the top of this file
#
# Note this reads the file .mbed directly because 'mbed target'
# returns a "helpful" string instead of an empty string if no value is set.
MBED_TARGET:=$(shell cat .mbed 2>/dev/null | grep TARGET | awk -F'=' '{print $$2}')
ifeq (${MBED_TARGET},)
  MBED_TARGET:=$(shell mbed detect 2>/dev/null | grep "Detected" | awk '{ print $$3 }' | sed 's/[,"]//g')
  ifeq (${MBED_TARGET},)
    MBED_TARGET:=${DEFAULT_TARGET}
  endif
endif

# Make sure the board type is supported
ifneq (${MBED_TARGET},$(filter ${MBED_TARGET},K64F UBLOX_EVK_ODIN_W2))
  $(error Unsupported board type: ${MBED_TARGET})
endif

# Specifies the name of the toolchain to use for compilation
#
# The following methods are checked for a toolchain, in order:
# 1. 'mbed toolchain'.  To specify a toolchain using this mechanism, run
# 	'mbed toolchain <toolchain>' in your build environment.
# 2. otherwise a default toolchain is used as specified at the top of this file
#
# Note this reads the file .mbed directly because 'mbed toolchain'
# returns a "helpful" string instead of an empty string if no value is set.
MBED_TOOLCHAIN:=$(shell cat .mbed 2>/dev/null | grep TOOLCHAIN | awk -F'=' '{print $$2}')
ifeq (${MBED_TOOLCHAIN},)
  MBED_TOOLCHAIN:=${DEFAULT_TOOLCHAIN}
endif

# Specifies the path to the directory containing build output files
MBED_BUILD_DIR:=./BUILD/${MBED_TARGET}/${MBED_TOOLCHAIN}

#
# Determine the correct patches to use
#

# TARGET=all
PATCHES=${PATCHDIR}/mbed-os-dns.diff

ifeq (${MBED_TARGET},K64F)
  BOOTLOADER_SIZE=0x20000
  APP_HEADER_OFFSET:=${BOOTLOADER_SIZE}
  APP_OFFSET:=0x20400
  # The linker script patch allows the compiled application to run
  # after the mbed bootloader.
  # The ram patch gives us more than 130k of ram to use
  ifeq (${MBED_TOOLCHAIN},GCC_ARM)
    PATCHES:=${PATCHDIR}/MK64FN1M0xxx12.ld.diff ${PATCHDIR}/gcc_k64f_ram_patch.diff
  else ifeq (${MBED_TOOLCHAIN},IAR)
    PATCHES:=${PATCHDIR}/MK64FN1M0xxx12.icf.diff
  else ifeq (${MBED_TOOLCHAIN},ARM)
    PATCHES:=${PATCHDIR}/MK64FN1M0xxx12.sct.diff
  endif
else ifeq (${MBED_TARGET},UBLOX_EVK_ODIN_W2)
  BOOTLOADER_SIZE=0x40000
  APP_HEADER_OFFSET:=0x40000
  APP_OFFSET:=0x40400
  # The gcc patch allows the compiled application to run
  # after the mbed bootloader.
  ifeq (${MBED_TOOLCHAIN},GCC_ARM)
    PATCHES:=${PATCHDIR}/ublox-evk-odin-w2-gcc.diff
  endif
endif

# Builds the command to call 'mbed compile'.
# $1: add extra options to the final command line
# $2: override all command line arguments.  final command is 'mbed compile $2'
define Build/Compile
	opts=""; \
	extra_opts=${1}; \
	force_opts=${2}; \
	opts="$${opts} -t ${MBED_TOOLCHAIN}"; \
	opts="$${opts} -m ${MBED_TARGET}"; \
	[ -n "${BUILD_PROFILE}" ] && { \
		opts="$${opts} --profile ${BUILD_PROFILE}"; \
	}; \
	[ -n "$${extra_opts}" ] && { \
		opts="$${opts} $${extra_opts}"; \
	}; \
	[ -n "$${force_opts}" ] && { \
		opts="$${force_opts}"; \
	}; \
	opts="$${opts} -N ${PROG}"; \
	cmd="mbed compile $${opts}"; \
	echo "$${cmd}"; \
	$${cmd}
endef

define Build/Bootloader/Compile
	opts=""; \
	extra_opts=${1}; \
	force_opts=${2}; \
	opts="$${opts} -t ${MBED_TOOLCHAIN}"; \
	opts="$${opts} -m ${MBED_TARGET}"; \
	[ -n "${BUILD_PROFILE}" ] && { \
		opts="$${opts} --profile ${BUILD_PROFILE}"; \
	}; \
	[ -n "$${extra_opts}" ] && { \
		opts="$${opts} $${extra_opts}"; \
	}; \
	[ -n "$${force_opts}" ] && { \
		opts="$${force_opts}"; \
	}; \
	cd ${BOOTLDR_DIR}; \
	BOOTLDR_DEVTAG="$$(git rev-parse --short HEAD)-$$(git rev-parse --abbrev-ref HEAD)"; \
	[ -n "$$(git status -s)" ] && { \
		BOOTLDR_DEVTAG="$${BOOTLDR_DEVTAG}-dev-${USER}"; \
	}; \
	opts="$${opts} -DDEVTAG=$${BOOTLDR_DEVTAG}"; \
	ln -fs ../display ; \
	ln -fs ../TextLCD ; \
	ln -fs ../ws2801  ; \
	ln -fs ../.mbed ; \
	cmd="mbed compile $${opts}"; \
	echo "$${cmd}"; \
	$${cmd};
endef

define Build/Bootloader/CheckSize
	@blsize=$(shell tail -n 1 ${BOOTLDR_DIR}/${MBED_BUILD_DIR}/${BOOTLDR_DIR}_map.csv | awk -F',' '{ print $$NF }'); \
	max=$(shell printf "%d" ${1}); \
	if [ "$${blsize}" -gt "$${max}" ]; then \
		echo "bootloader size $${blsize} is too big.  max=$${max}"; \
		false; \
	fi;
endef

.PHONY: all
all: build

.PHONY: build
build: prepare ${COMBINED_BIN}

${COMBINED_BIN}: ${BOOTLDR_BIN} ${PROG_BIN}
	$(call Build/Bootloader/CheckSize,${BOOTLOADER_SIZE})
	tools/combine_bootloader_with_app.py -b ${BOOTLDR_BIN} -a ${PROG_BIN} --app-offset ${APP_OFFSET} --header-offset ${APP_HEADER_OFFSET} -o ${COMBINED_BIN}

${PROG_BIN}: prepare ${SRCS} ${HDRS} mbed_app.json
	@$(call Build/Compile,"-DDEVTAG=${DEVTAG}")
	cp ${MBED_BUILD_DIR}/${PROG}.bin $@

${BOOTLDR_BIN}: prepare
	@$(call Build/Bootloader/Compile)
	cp ${BOOTLDR_DIR}/${MBED_BUILD_DIR}/${BOOTLDR_PROG}.bin $@

.PHONY: stats
stats:
	@cmd="python mbed-os/tools/memap.py -d -t ${MBED_TOOLCHAIN} ${MBED_BUILD_DIR}/${PROG}.map"; \
	echo "$${cmd}"; \
	$${cmd}

.PHONY: install flash
install flash: .targetpath ${COMBINED_BIN}
	@cmd="cp ${COMBINED_BIN} $$(cat .targetpath)"; \
	echo "$${cmd}"; \
	$${cmd}

tags: Makefile $(SRCS) $(HDRS)
	ctags -R

.PHONY: clean
clean:
	rm -rf BUILD
	rm -fr ${BOOTLDR_DIR}/BUILD
	rm -rf ${BINDIR}

.PHONY: patchclean
patchclean:
	@if [ -d mbed-os ]; then \
		cd mbed-os && git apply -R ${PATCHES}; \
	fi;
	rm -f .patches

.PHONY: distclean
distclean: clean certclean
	for lib in ${LIBS}; do rm -rf $${lib%.lib}; done
	rm -rf manifest-tool-restricted
	rm -rf mbed-cloud-client-restricted
	rm -rf mbed-bootloader-restricted
	rm -f .deps
	rm -f .targetpath
	rm -f .patches
	rm -f .firmware-url
	rm -f .manifest-id
	rm -f ${MANIFEST_FILE}

.PHONY: prepare
prepare: .mbed .deps update_default_resources.c .patches
	mkdir -p ${BINDIR}

.mbed:
	mbed config ROOT .
	mbed target ${MBED_TARGET}
	mbed toolchain ${MBED_TOOLCHAIN}

.deps: .mbed ${LIBS}
	mbed deploy --protocol ssh && touch .deps

# Acquire (and cache) the mount point of the board.
# If this fails, check that the board is mounted, and 'mbed detect' works.
# If the mount point changes, run 'make distclean'
.targetpath: .deps
	@set -o pipefail; TARGETPATH=$$(mbed detect | grep "mounted" | awk '{ print $$NF }') && \
		(echo $$TARGETPATH > .targetpath) || \
		(echo Error: could not detect mount path for the mbed board.  Verify that 'mbed detect' works.; exit 1)

.patches: .deps
	cd mbed-os && git apply ${PATCHES}
	touch .patches

################################################################################
# Update related rules
################################################################################

update_default_resources.c: .deps
	@which manifest-tool || (echo Error: manifest-tool not found.  Install it with \"pip install -r requirements.txt\"; exit 1)
	manifest-tool init -d "arm.com" -m "fota-demo" -q
	touch update_default_resources.c

.mbed-cloud-key:
	@echo "Error: You need to save an mbed cloud API key in .mbed-cloud-key"
	@echo "Please go to https://cloud.mbed.com/docs/v1.2/mbed-cloud-web-apps/access-mbed-cloud-with-api-keys.html"
	@exit 1

.PHONY: campaign
campaign: .deps .mbed-cloud-key .manifest-id
	python mbed-cloud-update-cli/create-campaign.py $$(cat .manifest-id) --key-file .mbed-cloud-key

MANIFEST_FILE=dev-manifest
.manifest-id: .firmware-url .mbed-cloud-key ${COMBINED_BIN}
	@which manifest-tool || (echo Error: manifest-tool not found.  Install it with \"pip install -r requirements.txt\"; exit 1)
	manifest-tool create -u $$(cat .firmware-url) -p ${PROG_BIN} -o ${MANIFEST_FILE}
	python mbed-cloud-update-cli/upload-manifest.py ${MANIFEST_FILE} --key-file .mbed-cloud-key -o $@

.firmware-url: .mbed-cloud-key ${COMBINED_BIN}
	python mbed-cloud-update-cli/upload-firmware.py ${PROG_BIN} --key-file .mbed-cloud-key -o $@

.PHONY: certclean
certclean:
	rm -rf .update-certificates
	rm -rf .manifest_tool.json
	rm -f update_default_resources.c
	rm -f .manifest-id
	rm -f .firmware-url
	rm -f ${MANIFEST_FILE}
