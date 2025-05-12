# ========== MAIN TARGET SELECTION ===========
#mode := debug
mode := release
#mode := simulation
#mode := demo



# image location stuff
fs_size := 33792 # in kB
bootloader_image_loc := 2000000 #in hex
fs_loc := 2020000 #in hex


# folder names
K=kernel
U=xv6-user
T=target
F=fs


# kernel objects to build
OBJS += \
  $K/entry.o \
  $K/printf.o \
  $K/kalloc.o \
  $K/exception.o \
  $K/intr.o \
  $K/spinlock.o \
  $K/string.o \
  $K/main.o \
  $K/vm.o \
  $K/proc.o \
  $K/swtch.o \
  $K/trampoline.o \
  $K/trap.o \
  $K/syscall.o \
  $K/sysproc.o \
  $K/bio.o \
  $K/sleeplock.o \
  $K/file.o \
  $K/pipe.o \
  $K/exec.o \
  $K/sysfile.o \
  $K/kernelvec.o \
  $K/timer.o \
  $K/disk.o \
  $K/fat32.o \
  $K/plic.o \
  $K/console.o \
  $K/flash.o \
  $K/uart.o \
  $K/debug.o \
  $K/video.o \
  $K/syspmu.o \
  $K/sbi/sbi_trap.o \
  $K/sbi/sbi_call.o \
  $K/sbi/sbi_impl.o \
  $K/sbi/sbi_impl_base.o \
  $K/sbi/sbi_impl_pmu.o


TOOLPREFIX	:= $(shell pwd)/../rv64i-gcc-custom/bin/riscv64-unknown-elf-
CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

QUARTUS = /ext/eda/quartus_pro/24_2/quartus/bin/quartus_

CFLAGS = -Wall  -O -fno-omit-frame-pointer -ggdb -g -w
CFLAGS += -march=rv64i_zicsr_zifencei -mabi=lp64 -mstrict-align
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -mno-relax
CFLAGS += -I.
CFLAGS += -I$(MPY_EMBED_DIR)
CFLAGS += -I$(MPY_EMBED_DIR)/port
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)



# ======================= TARGET CONFIGURATIONS ==========================
ifeq ($(mode), demo) 
CFLAGS += -DNOSIM
CFLAGS += -DTERMEMU
CFLAGS += -DRAMDISK
CFLAGS += -DVIDEO
endif

ifeq ($(mode), debug) 
CFLAGS += -DDEBUG
CFLAGS += -DNOSIM
CFLAGS += -DRAMDISK
endif 

ifeq ($(mode), simulation) 
CFLAGS += -DSMALLDEBUG
else
CFLAGS += -DNOSIM
CFLAGS += -DRAMDISK
endif 

# introduce busy loops to make sure simulation, emulation and hw have the same timing
#CFLAGS += -DUNIFORM_TIMING 
# stop xv6 when a user program crashes
#CFLAGS += -DUSERFAULTFATAL





# ============= GCC flags stuff (keep out of here if possible) ==============
LDFLAGS = -z max-page-size=4096 -w
linker = ./linker/kernel.ld
LIBGCC = $(shell $(CC)  --print-libgcc -march=rv64i_zicsr -mabi=lp64)

# Compile Kernel
$T/kernel: $(OBJS) $(linker) $U/initcode
	@if [ ! -d "./target" ]; then mkdir target; fi
	@$(LD) $(LDFLAGS) -T $(linker) -o $T/kernel $(OBJS) $(LIBGCC)
	@$(OBJDUMP) -S $T/kernel > $T/kernel.asm
	@$(OBJDUMP) -t $T/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $T/kernel.sym
	
	awk '{print "0x" $$0}' $T/kernel.sym | sort -g | cut -c 3- > target/kernel_tmp.sym
	mv target/kernel_tmp.sym target/kernel.sym
	- cp target/kernel.sym ../cyclone10gxlib/questasim/work/kernel.sym
build: $T/kernel userprogs




$U/initcode: $U/initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	$(OBJDUMP) -S $U/initcode.o > $U/initcode.asm

tags: $(OBJS) _init
	@etags *.S *.c

# ulib with advanced printf
ULIBPRA = $U/ulib.o $U/usys.o $U/aprintf.o $U/umalloc.o
# ulib with simple printf
ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o


# micropython source stuff
PYSRC += $(wildcard $(MPY_EMBED_DIR)/*/*.c) $(wildcard $(MPY_EMBED_DIR)/*/*/*.c)
PYLIB = $(PYSRC:.c=.o)

# regular user programs
_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^ $(LIBGCC) -l:m
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

$U/usys.S : $U/usys.pl
	@perl $U/usys.pl > $U/usys.S

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S



$U/_forktest: $U/forktest.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o
	$(OBJDUMP) -S $U/_forktest > $U/forktest.asm

$U/_mpy: $U/mpy.o $(ULIBPRA) $(PYLIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^ $(LIBGCC)
	$(OBJDUMP) -S $U/_mpy > $U/mpy.asm

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o


# user programs to build
UPROGS=\
	$U/_init\
	$U/_sh\
	$U/_cat\
	$U/_echo\
	$U/_grep\
	$U/_ls\
	$U/_lsh\
	$U/_kill\
	$U/_mkdir\
	$U/_xargs\
	$U/_sleep\
	$U/_find\
	$U/_rm\
	$U/_wc\
	$U/_test\
	$U/_usertests\
	$U/_strace\
	$U/_mv\
	$U/_rx\
	$U/_flush\
	$U/_mpy\
	$U/_mirror\
	$U/_testauxser\
	$U/_testfb\
	$U/_grafx\
	$U/_testpmu\

	# $U/_perftest\
	# $U/_forktest\
	# $U/_ln\
	# $U/_stressfs\
	# $U/_grind\
	# $U/_zombie\

userprogs: $(UPROGS)


# make kernel image
kimage: $T/kernel userprogs
	@$(OBJCOPY) -O binary target/kernel target/kernel.bin 
	@sleep 0.5
	@python3 tools/dat2mif.py -d 4096 -w 256 -p 0 -ar DEC target/kernel.bin target/kernel.mif

# Make fs image
fsimage: userprogs	
	dd if=/dev/zero of=target/fs.img bs=1k count=$(fs_size)
	mkfs.vfat -F 32 target/fs.img

	@rm -r $F ; true

	mkdir -p $F/bin
	@for file in $$( ls $U/_* ); do \
		cp $$file $F/bin/$${file#$U/_}; done

	cp $U/_init $F/init
	cp $U/_sh $F/sh
	cp $U/_echo $F/echo
	cp README $F/README
	cp $U/test.py $F/test.py

	python3 tools/putfolderinfsimg.py $F target/fs.img

	mdir -i target/fs.img ::

# regular enulation
run: kimage fsimage
	$(shell pwd)/../rvemu/target/release/rvemu-cli -k $T/kernel.bin -f $T/fs.img --fileoffset $(fs_loc)

# run with graphics emulation
rung: kimage fsimage
	$(shell pwd)/../rvemu/target/release/rvemu-cli -k $T/kernel.bin -f $T/fs.img --fileoffset $(fs_loc) -g

run_pts: kimage fsimage
	cat $(PTS) | $(shell pwd)/../rvemu/target/release/rvemu-cli -k $T/kernel.bin -f $T/fs.img --fileoffset $(fs_loc) > $(PTS)

# combine flash and kernel images into bootloader image
flash: kimage fsimage
	@python3 tools/create_flash_img.py target/kernel.bin target/cboot_flash.vmf $(bootloader_image_loc) 40000000 40000000 -p $(fs_loc) target/fs.img


upload_x:
	@python3 tools/xmodem.py --port /dev/ttyUSB0 --file $(FILE)

QUARTUSPROJ = /u/home/stud/csnbrkrt/Documents/FA/cyclone10gxlib/qis/cyclone10_soc_struct/cyclone10_soc.qis

quartus: kimage 
	$(QUARTUS)cdb $(QUARTUSPROJ)  -c $(QUARTUSPROJ) --update_mif
	$(QUARTUS)asm --read_settings_files=on --write_settings_files=off $(QUARTUSPROJ)  -c $(QUARTUSPROJ)
	$(QUARTUS)pgm -m jtag -o "p;../cyclone10gxlib/qis/cyclone10_soc_struct/cyclone10_soc.sof@2"
	
clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym */*.img\
	$T/* \
	$U/initcode $U/initcode.out \
	$K/kernel \
	.gdbinit \
	$U/usys.S \
	$(UPROGS); \
	rm -f $K/sbi/*.o $K/sbi/*.d
	

