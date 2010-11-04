#define _XOPEN_SOURCE 500
#include "k_uart.h"
#include "k_process.h"
#include "keyboard_process.h"
#include "crt_process.h"
#include "k_signal_handler.h"
#include "k_globals.h"
#include "k_storage.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#define KEYBOARD_SHMEM_FILE "keyboard.dat"
#define CRT_SHMEM_FILE "crt.dat"
#define FAIL -1

recv_buf_t *kb_buf;
send_buf_t *crt_buf;

pid_t rtx_pid;
pid_t kb_child_pid;
pid_t crt_child_pid;

void k_uart_init()
{
    // Register for uart signals
    sigset(SIGUSR1, handle_signal);
    sigset(SIGUSR2, handle_signal);

    // Initialize memory mapped files
    int kb_fid, crt_fid, status;
    kb_fid = open(KEYBOARD_SHMEM_FILE, O_RDWR | O_CREAT /*| O_EXCL*/, (mode_t) 0755);
    if (kb_fid < 0 )
    {
        printf("Bad open on file %s\n", KEYBOARD_SHMEM_FILE);
        exit(1);
    }

    status = ftruncate(kb_fid, sizeof(recv_buf_t));
    if (status != 0) 
    {
        printf("Could not truncate the file %s to %d bytes. status %d\n", 
                KEYBOARD_SHMEM_FILE, sizeof(recv_buf_t), status);
        exit(1);
    }

    crt_fid = open(CRT_SHMEM_FILE, O_RDWR | O_CREAT /*| O_EXCL*/, (mode_t) 0755);
    if (crt_fid < 0 )
    {
        printf("Bad open on mmap file %s\n", CRT_SHMEM_FILE);
        exit(1);
    }

    status = ftruncate(crt_fid, sizeof(send_buf_t));
    if (status != 0) 
    {
        printf("Could not truncate the file %s to %d bytes. status %d\n", 
                CRT_SHMEM_FILE, sizeof(send_buf_t), status);
        exit(1);
    }
    
    rtx_pid = getpid();

    // Setup mmap file for keyboard and fork it
    void *  mmap_ptr = mmap(NULL, sizeof(recv_buf_t), PROT_READ | PROT_WRITE, MAP_SHARED, kb_fid, 0);
    if (mmap_ptr == MAP_FAILED)
    {
        printf("Could not create mmap pointer to %s\n", KEYBOARD_SHMEM_FILE);
        exit(1);
    }
    kb_buf = (recv_buf_t *) mmap_ptr;
    close(kb_fid);

    kb_child_pid = fork();
    if (kb_child_pid == 0)
    {
        sigset(SIGINT, SIG_DFL);
        start_keyboard_process(rtx_pid, kb_buf);
        exit(0);
    }
    
    
    // Setup mmap file for crt and fork it
    mmap_ptr = mmap(NULL, sizeof(send_buf_t), PROT_READ | PROT_WRITE, MAP_SHARED, crt_fid, 0);
    if (mmap_ptr == MAP_FAILED)
    {
        printf("Could not create mmap pointer to %s\n", CRT_SHMEM_FILE);
        exit(1);
    }
    crt_buf = (send_buf_t *)mmap_ptr;
    close(crt_fid);

    crt_child_pid = fork();
    if (crt_child_pid == 0)
    {
        sigset(SIGINT, SIG_DFL);
        start_crt_process(rtx_pid, crt_buf);
        exit(0);
    }
}

void k_uart_cleanup()
{
    // kill children
    kill(kb_child_pid, SIGINT);
    kill(crt_child_pid, SIGINT);

    // Wait until they die first
    waitpid(kb_child_pid, NULL, 0);
    waitpid(crt_child_pid, NULL, 0);

    // close shared memory
    int status = munmap(kb_buf, sizeof(*kb_buf));
    if (status == FAIL)
    {
        printf("Unmapping the keyboard shared memory failed\n");
    }

    status = munmap(crt_buf, sizeof(*crt_buf));
    if (status == FAIL)
    {
        printf("Unmapping the keyboard shared memory failed\n");
    }

    // Delete shared memory file
    status = unlink(KEYBOARD_SHMEM_FILE);
    if (status == FAIL)
    {
        printf("Deleting the keyboard shared memory file failed\n");
    }

    status = unlink(CRT_SHMEM_FILE);
    if (status == FAIL)
    {
        printf("Deleting the crt shared memory file failed\n");
    }
}
