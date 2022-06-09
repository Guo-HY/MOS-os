/*
 * operations on IDE disk.
 */

#include "fs.h"
#include "lib.h"
#include <mmu.h>

// Overview:
// 	read data from IDE disk. First issue a read request through
// 	disk register and then copy data from disk buffer
// 	(512 bytes, a sector) to destination array.
//
// Parameters:
//	diskno: disk number.
// 	secno: start sector number.
// 	dst: destination for data read from IDE disk.
// 	nsecs: the number of sectors to read.
//
// Post-Condition:
// 	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
void
ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs)
{
	// 0x200: the size of a sector: 512 bytes.
	int offset_begin = secno * 0x200;
	int offset_end = offset_begin + nsecs * 0x200;
	int offset = 0;
    u_int ide = 0x13000000;
    u_int now_offset;
    u_char read_value = 0;
    u_char status = 0;
	while (offset_begin + offset < offset_end) {
		// Your code here
		// error occurred, then panic.
        now_offset = offset_begin + offset;
        if (syscall_write_dev(&diskno, ide + 0x10, 4) < 0) {
            user_panic("ide_read error 1");
        }
        if (syscall_write_dev(&now_offset, ide, 4) < 0) {
            user_panic("ide_read error 2");
        }
        if (syscall_write_dev(&read_value, ide + 0x20, 1) < 0) {
            user_panic("ide_read error 3");
        }
        status = 0;
        if (syscall_read_dev(&status, ide + 0x30, 1) < 0) {
            user_panic("ide_read error 4");
        }
        if (status == 0) {
            user_panic("ide_read failure");
        }
        if (syscall_read_dev(dst + offset, ide + 0x4000, 0x200) < 0) {
            user_panic("ide_read error 5");
        }
        offset += 0x200;
	}
}


// Overview:
// 	write data to IDE disk.
//
// Parameters:
//	diskno: disk number.
//	secno: start sector number.
// 	src: the source data to write into IDE disk.
//	nsecs: the number of sectors to write.
//
// Post-Condition:
//	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
void
ide_write(u_int diskno, u_int secno, void *src, u_int nsecs)
{
	// Your code here
	int offset_begin = secno * 0x200;
	int offset_end = offset_begin + nsecs * 0x200;
	int offset = 0;
    u_int ide = 0x13000000;
    u_int now_offset;
    u_char status = 0;
    u_char write_value = 1;
	// DO NOT DELETE WRITEF !!!
	writef("diskno: %d\n", diskno);

	while (offset_begin + offset < offset_end) {
		// copy data from source array to disk buffer.
		// if error occur, then panic.
        now_offset = offset_begin + offset;
        if (syscall_write_dev(&diskno, ide + 0x10, 4) < 0) {
            user_panic("ide_write error 1");
        }
        if (syscall_write_dev(&now_offset, ide, 4) < 0) {
            user_panic("ide_write error 2");
        }
        if (syscall_write_dev(src + offset, ide + 0x4000, 0x200) < 0) {
            user_panic("ide_write error 3");
        }
        if (syscall_write_dev(&write_value, ide + 0x20, 1) < 0) {
            user_panic("ide_write error 4");
        }
        status = 0;
        if (syscall_read_dev(&status, ide + 0x30, 1) < 0) {
            user_panic("ide_write error 5");
        }

        if (status == 0) {
            user_panic("ide write failure");
        }
        offset += 0x200;
	}
}

