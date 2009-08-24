/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007, 2008, 2009 Marc Noirot <marc.noirot AT gmail.com>

    This file is part of FLVMeta.

    FLVMeta is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLVMeta is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLVMeta; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flvmeta.h"
#include "dump.h"
#include "inject.h"

/*
    Command-line options
*/
static struct option long_options[] = {
    { "dump",          no_argument,         NULL, 'D'},
    { "full-dump",     no_argument,         NULL, 'F'},
    { "check",         no_argument,         NULL, 'C'},
    { "inject",        no_argument,         NULL, 'I'},
    { "add",           required_argument,   NULL, 'a'},
    { "no-lastsecond", no_argument,         NULL, 'l'},
    { "preserve",      no_argument,         NULL, 'p'},
    { "fix",           no_argument,         NULL, 'f'},
    { "ignore",        no_argument,         NULL, 'i'},
    { "dump-format",   required_argument,   NULL, 'd'},
    { "json",          no_argument,         NULL, 'j'},
    { "raw",           no_argument,         NULL, 'r'},
    { "yaml",          no_argument,         NULL, 'y'},
    { "xml",           no_argument,         NULL, 'x'},
    { "verbose",       no_argument,         NULL, 'v'},
    { "version",       no_argument,         NULL, 'V'},
    { "help",          no_argument,         NULL, 'h'},
    { 0, 0, 0, 0 }
};

void version(void) {
    fprintf(stderr, "%s\n", PACKAGE_STRING);
    fprintf(stderr, "%s\n", COPYRIGHT_STR);
    fprintf(stderr, "This is free software; see the source for copying conditions. There is NO\n"
                    "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

void usage(const char * name) {
    fprintf(stderr, "Usage: %s [COMMAND] [OPTIONS] FILE [OUTPUT_FILE]\n", name);
    fprintf(stderr, "Try `%s --help' for more information.\n", name);
}

static void help(const char * name) {
    version();
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [COMMAND] [OPTIONS] FILE [OUTPUT_FILE]\n", name);
    fprintf(stderr, "\nCommands:\n");
    fprintf(stderr, "  -D, --dump                dump onMetaData tag (default without output file)\n");
    fprintf(stderr, "  -F, --full-dump           dump all tags\n");
    fprintf(stderr, "  -C, --check               check the validity of FILE\n");
    fprintf(stderr, "  -I, --inject              inject computed onMetaData tag from FILE\n");
    fprintf(stderr, "                            into OUTPUT_FILE (default with output file)\n");
    fprintf(stderr, "  -A, --extract-audio       extract raw audio data into OUTPUT_FILE\n");
    fprintf(stderr, "  -E, --extract-video       extract raw video data into OUTPUT_FILE\n");
    fprintf(stderr, "\nOutput control options:\n");
    fprintf(stderr, "  -a, --add=NAME=VALUE      add a metadata string value to the output file\n");
    fprintf(stderr, "  -l, --no-lastsecond       do not create the onLastSecond tag\n");
    fprintf(stderr, "  -p, --preserve            preserve input file existing onMetadata tags\n");
    fprintf(stderr, "  -f, --fix                 fix invalid tags from the input file\n");
    fprintf(stderr, "  -i, --ignore              ignore invalid tags from the input file\n");
    fprintf(stderr, "                            (the default is to stop with an error)\n");
    fprintf(stderr, "  -d, --dump-format=TYPE    dump format is of type TYPE\n");
    fprintf(stderr, "                            TYPE is 'xml' (default), 'raw', 'json', or 'yaml'\n");
    fprintf(stderr, "  -j, --json                equivalent to --dump-format=json\n");
    fprintf(stderr, "  -r, --raw                 equivalent to --dump-format=raw\n");
    fprintf(stderr, "  -y, --yaml                equivalent to --dump-format=yaml\n");
    fprintf(stderr, "  -x, --xml                 equivalent to --dump-format=xml\n");
    fprintf(stderr, "\nAdvanced options:\n");
    fprintf(stderr, "  -v, --verbose             display informative messages\n");
    fprintf(stderr, "\nMiscellaneous:\n");
    fprintf(stderr, "  -V, --version             print version information and exit\n");
    fprintf(stderr, "  -h, --help                display this information and exit\n");
    fprintf(stderr, "\nPlease report bugs to <%s>\n", PACKAGE_BUGREPORT);
}

int main(int argc, char ** argv) {
    int option, option_index, errcode;

    /* flvmeta default options */
    static flvmeta_opts options;
    options.command = FLVMETA_DEFAULT_COMMAND;
    options.input_file = NULL;
    options.output_file = NULL;
    options.metadata = NULL;
    options.insert_onlastsecond = 1;
    options.preserve_metadata = 0;
    options.error_handling = FLVMETA_EXIT_ON_ERROR;
    options.dump_format = FLVMETA_FORMAT_XML;
    options.verbose = 0;

    /*
        Command-line parsing
    */
    option_index = 0;
    do {
        option = getopt_long(argc, argv, "DFCIa:lpfid:jyxvVh", long_options, &option_index);
        switch (option) {
            /*
                commands
            */
            case 'D':
                if (options.command != FLVMETA_DEFAULT_COMMAND) {
                    fprintf(stderr, "%s: only one command can be specified -- %s\n", argv[0], argv[optind]);
                    exit(EXIT_FAILURE);
                }
                options.command = FLVMETA_DUMP_COMMAND;
                break;
            case 'F':
                if (options.command != FLVMETA_DEFAULT_COMMAND) {
                    fprintf(stderr, "%s: only one command can be specified -- %s\n", argv[0], argv[optind]);
                    exit(EXIT_FAILURE);
                }
                options.command = FLVMETA_FULL_DUMP_COMMAND;
                break;
            case 'C':
                if (options.command != FLVMETA_DEFAULT_COMMAND) {
                    fprintf(stderr, "%s: only one command can be specified -- %s\n", argv[0], argv[optind]);
                    exit(EXIT_FAILURE);
                }
                options.command = FLVMETA_CHECK_COMMAND;
                break;
            case 'I':
                if (options.command != FLVMETA_DEFAULT_COMMAND) {
                    fprintf(stderr, "%s: only one command can be specified -- %s\n", argv[0], argv[optind]);
                    exit(EXIT_FAILURE);
                }
                options.command = FLVMETA_INJECT_COMMAND;
                break;
            /*
                options
            */
            /* add metadata */
            case 'a':
                {
                    char * eq_pos;
                    eq_pos = strchr(optarg, '=');
                    if (eq_pos != NULL) {
                        *eq_pos = 0;
                        if (options.metadata == NULL) {
                            options.metadata = amf_associative_array_new();
                        }
                        amf_object_add(options.metadata, amf_str(optarg), amf_str(eq_pos + 1));
                    }
                    else {
                        fprintf(stderr, "%s: invalid metadata format -- %s\n", argv[0], optarg);
                        usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                } break;
            case 'l': options.insert_onlastsecond = 0;                  break;
            case 'p': options.preserve_metadata = 1;                    break;
            case 'f': options.error_handling = FLVMETA_FIX_ERRORS;      break;
            case 'i': options.error_handling = FLVMETA_IGNORE_ERRORS;   break;
            /* choose dump format */
            case 'd':
                if (!strcmp(optarg, "xml")) {
                    options.dump_format = FLVMETA_FORMAT_XML;
                }
                if (!strcmp(optarg, "raw")) {
                    options.dump_format = FLVMETA_FORMAT_RAW;
                }
                else if (!strcmp(optarg, "json")) {
                    options.dump_format = FLVMETA_FORMAT_JSON;
                }
                else if (!strcmp(optarg, "yaml")) {
                    options.dump_format = FLVMETA_FORMAT_YAML;
                }
                else {
                    fprintf(stderr, "%s: invalid output format -- %s\n", argv[0], optarg);
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'j': options.dump_format = FLVMETA_FORMAT_JSON;    break;
            case 'r': options.dump_format = FLVMETA_FORMAT_RAW;     break;
            case 'y': options.dump_format = FLVMETA_FORMAT_XML;     break;
            case 'x': options.dump_format = FLVMETA_FORMAT_YAML;    break;
            /*
                advanced options
            */
            case 'v': options.verbose = 1;  break;
            /*
                Miscellaneous
            */
            case 'V':
                version();
                exit(EXIT_SUCCESS);
                break;
            case 'h':
                help(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            /* last option */
            case EOF: break;
            /*
                error cases
            */
            /* unknown option */
            case '?':
            /* missing argument */
            case ':':
            /* we should not be here */
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    } while (option != EOF);

    /* input filename */
    if (optind > 0 && optind < argc) {
        options.input_file = argv[optind];
    }
    else {
        fprintf(stderr, "%s: no input file\n", argv[0]);
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* output filename */
    if (++optind < argc) {
        options.output_file = argv[optind];
    }

    /* determine command if default */
    if (options.command == FLVMETA_DEFAULT_COMMAND && options.output_file != NULL) {
        options.command = FLVMETA_INJECT_COMMAND;
    }
    else if (options.command == FLVMETA_DEFAULT_COMMAND && options.output_file == NULL) {
        options.command = FLVMETA_DUMP_COMMAND;
    }

    /* execute command */
    errcode = OK;
    switch (options.command) {
        case FLVMETA_DUMP_COMMAND: errcode = dump_metadata(&options); break;
        case FLVMETA_FULL_DUMP_COMMAND: errcode = dump_flv_file(&options); break;
        case FLVMETA_CHECK_COMMAND: break;
        case FLVMETA_INJECT_COMMAND: errcode = inject_metadata(&options); break;
    }

    /* error report */
    switch (errcode) {
        case ERROR_SAME_FILE: fprintf(stderr, "%s: input file and output file must be different\n", argv[0]); break;
        case ERROR_OPEN_READ: fprintf(stderr, "%s: cannot open %s for reading\n", argv[0], options.input_file); break;
        case ERROR_OPEN_WRITE: fprintf(stderr, "%s: cannot open %s for writing\n", argv[0], options.output_file); break;
        case ERROR_NO_FLV: fprintf(stderr, "%s: %s is not a valid FLV file\n", argv[0], options.input_file); break;
        case ERROR_EOF: fprintf(stderr, "%s: unexpected end of file\n", argv[0]); break;
        case ERROR_INVALID_TAG: fprintf(stderr, "%s: invalid FLV tag\n", argv[0]); break;
        case ERROR_WRITE: fprintf(stderr, "%s: unable to write to %s\n", argv[0], options.output_file); break;
    }

    return errcode;
}
