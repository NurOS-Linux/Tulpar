// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <sys/wait.h>
#include <unistd.h>

#include "proc.h"

bool
run_command(char *const argv[])
{
    pid_t pid = fork();
    if (pid < 0)
        return false;

    if (pid == 0)
    {
        execvp(argv[0], argv);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0)
        return false;

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
