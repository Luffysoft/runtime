// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "createdump.h"

#ifdef HOST_WINDOWS
#define DEFAULT_DUMP_PATH "%TEMP%\\"
#define DEFAULT_DUMP_TEMPLATE "dump.%d.dmp"
#else
#define DEFAULT_DUMP_PATH "/tmp/"
#define DEFAULT_DUMP_TEMPLATE "coredump.%d"
#endif

const char* g_help = "createdump [options] pid\n"
"-f, --name - dump path and file name. The pid can be placed in the name with %d. The default is '" DEFAULT_DUMP_PATH DEFAULT_DUMP_TEMPLATE "'\n"
"-n, --normal - create minidump.\n"
"-h, --withheap - create minidump with heap (default).\n"
"-t, --triage - create triage minidump.\n"
"-u, --full - create full core dump.\n"
"-d, --diag - enable diagnostic messages.\n";

bool CreateDump(const char* dumpPathTemplate, int pid, MINIDUMP_TYPE minidumpType);

//
// Main entry point
//
int __cdecl main(const int argc, const char* argv[])
{
    MINIDUMP_TYPE minidumpType = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
                                                 MiniDumpWithDataSegs |
                                                 MiniDumpWithHandleData |
                                                 MiniDumpWithUnloadedModules |
                                                 MiniDumpWithFullMemoryInfo |
                                                 MiniDumpWithThreadInfo |
                                                 MiniDumpWithTokenInformation);
    const char* dumpPathTemplate = nullptr;
    int exitCode = 0;
    int pid = 0;

#ifdef HOST_UNIX
    exitCode = PAL_InitializeDLL();
    if (exitCode != 0)
    {
        fprintf(stderr, "PAL initialization FAILED %d\n", exitCode);
        return exitCode;
    }
#endif

    // Parse the command line options and target pid
    argv++;
    for (int i = 1; i < argc; i++)
    {
        if (*argv != nullptr)
        {
            if ((strcmp(*argv, "-f") == 0) || (strcmp(*argv, "--name") == 0))
            {
                dumpPathTemplate = *++argv;
            }
            else if ((strcmp(*argv, "-n") == 0) || (strcmp(*argv, "--normal") == 0))
            {
                minidumpType = (MINIDUMP_TYPE)(MiniDumpNormal |
                                               MiniDumpWithThreadInfo);
            }
            else if ((strcmp(*argv, "-h") == 0) || (strcmp(*argv, "--withheap") == 0))
            {
                minidumpType = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
                                               MiniDumpWithDataSegs |
                                               MiniDumpWithHandleData |
                                               MiniDumpWithUnloadedModules |
                                               MiniDumpWithFullMemoryInfo |
                                               MiniDumpWithThreadInfo |
                                               MiniDumpWithTokenInformation);
            }
            else if ((strcmp(*argv, "-t") == 0) || (strcmp(*argv, "--triage") == 0))
            {
                minidumpType = (MINIDUMP_TYPE)(MiniDumpFilterTriage |
                                               MiniDumpWithThreadInfo);
            }
            else if ((strcmp(*argv, "-u") == 0) || (strcmp(*argv, "--full") == 0))
            {
                minidumpType = (MINIDUMP_TYPE)(MiniDumpWithFullMemory |
                                               MiniDumpWithDataSegs |
                                               MiniDumpWithHandleData |
                                               MiniDumpWithUnloadedModules |
                                               MiniDumpWithFullMemoryInfo |
                                               MiniDumpWithThreadInfo |
                                               MiniDumpWithTokenInformation);
            }
            else if ((strcmp(*argv, "-d") == 0) || (strcmp(*argv, "--diag") == 0))
            {
                g_diagnostics = true;
            }
            else {
                pid = atoi(*argv);
            }
            argv++;
        }
    }

    if (pid != 0)
    {
        ArrayHolder<char> tmpPath = new char[MAX_LONGPATH];
        ArrayHolder<char> dumpPath = new char[MAX_LONGPATH];

        if (dumpPathTemplate == nullptr)
        {
            if (::GetTempPathA(MAX_LONGPATH, tmpPath) == 0)
            {
                fprintf(stderr, "GetTempPath failed (0x%08x)", ::GetLastError());
                return ::GetLastError();
            }
            exitCode = strcat_s(tmpPath, MAX_LONGPATH, DEFAULT_DUMP_TEMPLATE);
            if (exitCode != 0)
            {
                fprintf(stderr, "strcat_s failed (%d)", exitCode);
                return exitCode;
            }
            dumpPathTemplate = tmpPath;
        }

        snprintf(dumpPath, MAX_LONGPATH, dumpPathTemplate, pid);

        const char* dumpType = "minidump";
        switch (minidumpType)
        {
            case MiniDumpWithPrivateReadWriteMemory:
                dumpType = "minidump with heap";
                break;

            case MiniDumpFilterTriage:
                dumpType = "triage minidump";
                break;

            case MiniDumpWithFullMemory:
                dumpType = "full dump";
                break;

            default:
                break;
        }
        printf("Writing %s to file %s\n", dumpType, (char*)dumpPath);

        if (CreateDump(dumpPath, pid, minidumpType))
        {
            printf("Dump successfully written\n");
        }
        else
        {
            exitCode = -1;
        }
    }
    else
    {
        // if no pid or invalid command line option
        fprintf(stderr, "%s", g_help);
        exitCode = -1;
    }
#ifdef HOST_UNIX
    PAL_TerminateEx(exitCode);
#endif
    return exitCode;
}
