#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_REPO=https://git.busybox.net/busybox/
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
LIBC_TOOLCHAIN=~/Documents/LinuxSysProgramAndIntrodBuildroot/arm-gnu-toolchain/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    	sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
	echo "---> Creating the rootfs base directories..."
	mkdir -p "${OUTDIR}/rootfs"
	cd "${OUTDIR}/rootfs"
	mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
	mkdir -p usr/bin usr/lib usr/sbin
	mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
	echo "---> Cloning busybox..."
	# Replaced git://busybox.net/busybox.git with https://git.busybox.net/busybox/ , as the first option was throwing the error "fatal: read error: connection reset by peer"
	# Added --branch ${BUSYBOX_VERSION} as the full-test was throwing the error "pathspec '1_33_1' did not match any file(s) known to git" when doing the git checkout
	git clone ${BUSYBOX_REPO} --depth 1 --single-branch --branch ${BUSYBOX_VERSION}
	echo "---> Checking out busybox..."
	cd busybox
	git checkout ${BUSYBOX_VERSION}
	
	# TODO:  Configure busybox
    	echo "---> Configuring busybox..."
    	echo "-----> make distclean..."
	make distclean
	echo "-----> make defconfig..."
	make defconfig 	
else
    	cd busybox
fi

# TODO: Make and install busybox
	echo "---> Making and installing busybox..."
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
	make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
	
echo "Library dependencies"
# Added cd command for moving to the rootfs directory
cd "${OUTDIR}/rootfs"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
	echo "---> Adding library dependencies to rootfs..."
	cp ${LIBC_TOOLCHAIN}/lib/ld-linux-aarch64.so.1 "${OUTDIR}/rootfs/lib"
	cp ${LIBC_TOOLCHAIN}/lib64/libm.so.6 "${OUTDIR}/rootfs/lib64"
	cp ${LIBC_TOOLCHAIN}/lib64/libresolv.so.2 "${OUTDIR}/rootfs/lib64"
	cp ${LIBC_TOOLCHAIN}/lib64/libc.so.6 "${OUTDIR}/rootfs/lib64"

# TODO: Make device nodes
	echo "---> Making device nodes..."
	sudo mknod -m 666 $OUTDIR/rootfs/dev/null c 1 3
	sudo mknod -m 666 $OUTDIR/rootfs/dev/console c 5 1

# TODO: Clean and build the writer utility
	echo "---> Building the writer utility..."
	cd $FINDER_APP_DIR
	make clean
	make CROSS_COMPILE=$CROSS_COMPILE
	mv writer "${OUTDIR}/rootfs/home"

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
	echo "---> Copying the finder related scripts and helper files..."
	cp $FINDER_APP_DIR/finder.sh "${OUTDIR}/rootfs/home"
	cp $FINDER_APP_DIR/finder-test.sh "${OUTDIR}/rootfs/home"
	mkdir ${OUTDIR}/rootfs/home/conf
	cp $FINDER_APP_DIR/conf/username.txt "${OUTDIR}/rootfs/home/conf"
	cp $FINDER_APP_DIR/conf/assignment.txt "${OUTDIR}/rootfs/home/conf"
	echo "---> Replacing ../conf/assignment.txt with conf/assignment.txt in finder-test.sh..."
	sed -i 's/..\/conf\/assignment.txt/conf\/assignment.txt/g' "${OUTDIR}/rootfs/home/finder-test.sh"
	echo "---> Copying the autorun-qemu script..."
	cp $FINDER_APP_DIR/autorun-qemu.sh "${OUTDIR}/rootfs/home"

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
	#Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
	cd linux-stable
	echo "Checking out version ${KERNEL_VERSION}"
	git checkout ${KERNEL_VERSION}

    	# TODO: Add your kernel build steps here
	    	echo "---> Building the kernel..."
	    	# Clean
		make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
	    	# Build and set up default config
	    	make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
	    	# Build kernel image for booting with QEMU
	    	make -j8 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
	    	# Build module
	    	# Step skipped because the modules generated with the default kernel build are too large to fit in the initramfs with default memory
	    	# make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
	    	# Build devicetree
	    	make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
	    	
	    	echo "Adding the Image in outdir"
	    	cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image	    	    	
fi

# TODO: Chown the root directory
	echo "---> Chowning the root directory..."
	cd ${OUTDIR}/rootfs
	find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

# TODO: Create initramfs.cpio.gz
	echo "---> Creating initramfs.cpio.gz..."
	gzip -f ${OUTDIR}/initramfs.cpio
