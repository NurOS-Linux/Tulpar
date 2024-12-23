/**
 * @file cli.c
 * @brief Command Line Interface for Tulpar Package Manager
 * @author AnmiTaliDev
 * @date 2024-12-23 19:45:43 UTC
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "tulpar.h"

#define VERSION "1.0.0"

static void print_usage(void) {
    printf("Tulpar Package Manager %s\n", VERSION);
    printf("Usage: tulpar <command> [options] [package]\n\n");
    printf("Commands:\n");
    printf("  install, i     Install package(s)\n");
    printf("  remove, r      Remove package(s)\n");
    printf("  update, u      Update package(s)\n");
    printf("  list, l        List installed packages\n");
    printf("  search, s      Search for packages\n");
    printf("  clean, c       Clean package cache\n");
    printf("  help           Show this help\n");
    printf("  version        Show version\n\n");
    printf("Options:\n");
    printf("  -f, --force    Force operation\n");
    printf("  -y, --yes      Assume yes to prompts\n");
    printf("  -q, --quiet    Suppress output\n");
    printf("  -v, --verbose  Verbose output\n");
}

static void print_version(void) {
    printf("Tulpar Package Manager %s\n", VERSION);
}

static int cmd_install(int argc, char **argv) {
    TulparOptions opts = {0};
    int force = 0;
    
    static struct option long_options[] = {
        {"force", no_argument, 0, 'f'},
        {"yes", no_argument, 0, 'y'},
        {"quiet", no_argument, 0, 'q'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "fyq", long_options, NULL)) != -1) {
        switch (opt) {
            case 'f': force = 1; break;
            case 'y': opts.yes = 1; break;
            case 'q': opts.quiet = 1; break;
            default: return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: No package specified\n");
        return 1;
    }

    TulparError err = tulpar_init();
    if (err != TULPAR_OK) {
        fprintf(stderr, "Error: Failed to initialize\n");
        return 1;
    }

    int ret = 0;
    for (int i = optind; i < argc; i++) {
        opts.force = force;
        printf("Installing %s...\n", argv[i]);
        
        err = tulpar_install(argv[i], &opts);
        if (err != TULPAR_OK) {
            fprintf(stderr, "Error installing %s\n", argv[i]);
            ret = 1;
        }
    }

    tulpar_cleanup();
    return ret;
}

static int cmd_remove(int argc, char **argv) {
    if (optind >= argc) {
        fprintf(stderr, "Error: No package specified\n");
        return 1;
    }

    TulparError err = tulpar_init();
    if (err != TULPAR_OK) {
        fprintf(stderr, "Error: Failed to initialize\n");
        return 1;
    }

    int ret = 0;
    for (int i = optind; i < argc; i++) {
        printf("Removing %s...\n", argv[i]);
        
        err = tulpar_remove(argv[i]);
        if (err != TULPAR_OK) {
            fprintf(stderr, "Error removing %s\n", argv[i]);
            ret = 1;
        }
    }

    tulpar_cleanup();
    return ret;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const char *cmd = argv[1];
    argc--;
    argv++;

    if (strcmp(cmd, "install") == 0 || strcmp(cmd, "i") == 0) {
        return cmd_install(argc, argv);
    }
    else if (strcmp(cmd, "remove") == 0 || strcmp(cmd, "r") == 0) {
        return cmd_remove(argc, argv);
    }
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0) {
        print_usage();
        return 0;
    }
    else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "v") == 0) {
        print_version();
        return 0;
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        return 1;
    }
}