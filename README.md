# XV6-IKR
![grafik](xv6_boot.gif)

## Usage
1. Building

Set the build mode: In the Makefile select correct platform (default is release)

make the kernel: 
    make kimage
make user programs: 
    make kimage
assemble the flash image (not required for simulation): 
    make flash


2. Running
You can either access the emulator directly or via a virtual console.
Direct access is fine for most use cases, wont work with xmodem transfers though.

Direct access:
    make run


Access via a pts:

In a new console, run: 
    socat -d -d pty,raw,echo=0 pty,raw,echo=0
This will give you two /dev/pts/x paths
One of them is for the emulator:
    make run_pts PTS=/dev/pts/a

The other one is for the program wanting to access the emulator
You can then use minicom -D /dev/pts/b to access this pts. (Also in a new console)

If you want to upload  a file using xmodem, you first have to close the minicom session (CTRL-A Z X Enter), then do:
python3 tools/xmodem.py -p /dev/pts/b -f youtfilenamehere.ex
As soon as this is done, you can reconnect using minicom and check the file is really there using teh 'lsh' command in Xv6.



## About shell

The shell commands are user programs, too. Those program should be put in the "/bin" directory in your SD card or the `fs.img`.  
Now we support a few useful commands, such as `cd`, `ls`, `cat` and so on.

In addition, `shell` supports some shortcut keys as below:

- Ctrl-H -- backspace  
- Ctrl-U -- kill a line  
- Ctrl-D -- end of file (EOF)  
- Ctrl-P -- print process list  
- Ctrl-C -- kill all running processes  

## Add my programs on xv6-k210
1. Make a new C source file in `xv6-user/` like `myprog.c`, and put your codes;
2. You can include `user.h` to use the functions declared in it, such as `open`, `gets` and `printf`;
3. Add a line "`$U/_myprog\`" in `Makefile` as below:
    ```Makefile
    UPROGS=\
        $U/_init\
        $U/_sh\
        $U/_cat\
        ...
        $U/_myprog\      # Don't ignore the leading '_'
    ```
4. Then make:
    ```bash
    make fsimage
    ```
    Now you might see `_myprog` in `xv6-user/` if no error detected. Finally you need to copy it into your SD (see [here](#run-on-k210-board))
     or FS image (see [here](#run-on-qemu-system-riscv64)).


