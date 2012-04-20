/*
    $Id: flvmeta.c 231 2011-06-27 13:46:19Z marc.noirot $

    FLV Metadata updater

    Copyright (C) 2007-2012 Marc Noirot <marc.noirot AT gmail.com>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flvmeta.h"
#include "check.h"
#include "dump.h"
#include "update.h"

/*
    Command-line options
*/
static struct option long_options[] = {
    { "dump",               no_argument,        NULL, 'D'},
    { "full-dump",          no_argument,        NULL, 'F'},
    { "check",              no_argument,        NULL, 'C'},
    { "update",             no_argument,        NULL, 'U'},
    { "dump-format",        required_argument,  NULL, 'd'},
    { "json",               no_argument,        NULL, 'j'},
    { "raw",                no_argument,        NULL, 'r'},
    { "xml",                no_argument,        NULL, 'x'},
    { "yaml",               no_argument,        NULL, 'y'},
    { "event",              required_argument,  NULL, 'e'},
    { "level",              required_argument,  NULL, 'l'},
    { "quiet",              no_argument,        NULL, 'q'},
    { "print-metadata",     no_argument,        NULL, 'm'},
    { "add",                required_argument,  NULL, 'a'},
    { "no-lastsecond",      no_argument,        NULL, 's'},
    { "preserve",           no_argument,        NULL, 'p'},
    { "fix",                no_argument,        NULL, 'f'},
    { "ignore",             no_argument,        NULL, 'i'},
    { "reset-timestamps",   no_argument,        NULL, 't'},
    { "all-keyframes",      no_argument,        NULL, 'k'},
    { "verbose",            no_argument,        NULL, 'v'},
    { "version",            no_argument,        NULL, 'V'},
    { "help",               no_argument,        NULL, 'h'},
    { 0, 0, 0, 0 }
};

/* short options */
#define DUMP_COMMAND                "D"
#define FULL_DUMP_COMMAND           "F"
#define CHECK_COMMAND               "C"
#define UPDATE_COMMAND              "U"
#define DUMP_FORMAT_OPTION          "d:"
#define JSON_OPTION                 "j"
#define RAW_OPTION                  "r"
#define XML_OPTION                  "x"
#define YAML_OPTION                 "y"
#define EVENT_OPTION                "e:"
#define LEVEL_OPTION                "l:"
#define QUIET_OPTION                "q"
#define PRINT_METADATA_OPTION       "m"
#define ADD_OPTION                  "a:"
#define NO_LASTSECOND_OPTION        "s"
#define PRESERVE_OPTION             "p"
#define FIX_OPTION                  "f"
#define IGNORE_OPTION               "i"
#define RESET_TIMESTAMPS_OPTION     "t"
#define ALL_KEYFRAMES_OPTION        "k"
#define VERBOSE_OPTION              "v"
#define VERSION_OPTION              "V"
#define HELP_OPTION                 "h"

static void version(void) {
    printf("%s\n\n", PACKAGE_STRING);
    printf("%s\n", COPYRIGHT_STR);
    printf("This is free software; see the source for copying conditions. There is NO\n"
           "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
}

static void usage(const char * name) {
    fprintf(stderr, "Usage: %s [COMMAND] [OPTIONS] INPUT_FILE [OUTPUT_FILE]\n", name);
    fprintf(stderr, "Try `%s --help' for more information.\n", name);
}

static void help(const char * name) {
    printf("Usage: %s [COMMAND] [OPTIONS] INPUT_FILE [OUTPUT_FILE]\n", name);
    printf("\nIf OUTPUT_FILE is omitted for commands expecting it, INPUT_FILE will be overwritten instead.\n"
           "\nCommands:\n"
           "  -D, --dump                dump onMetaData tag (default without output file)\n"
           "  -F, --full-dump           dump all tags\n"
           "  -C, --check               check the validity of INPUT_FILE, returning 0 if\n"
           "                            the file is valid, or 10 if it contains errors\n"
           "  -U, --update              update computed onMetaData tag from INPUT_FILE\n"
           "                            into OUTPUT_FILE (default with output file)\n"
           /*    "  -A, --extract-audio       extract raw audio data into OUTPUT_FILE\n"*/
           /*    "  -E, --extract-video       extract raw video data into OUTPUT_FILE\n"*/
           "\nDump options:\n"
           "  -d, --dump-format=TYPE    dump format is of type TYPE\n"
           "                            TYPE is 'xml' (default), 'json', 'raw', or 'yaml'\n"
           "  -j, --json                equivalent to --dump-format=json\n"
           "  -r, --raw                 equivalent to --dump-format=raw\n"
           "  -x, --xml                 equivalent to --dump-format=xml\n"
           "  -y, --yaml                equivalent to --dump-format=yaml\n"
           "  -e, --event=EVENT         specify the event to be dumped instead of 'onMetadata'\n"
           "\nCheck options:\n"
           "  -l, --level=LEVEL         print only messages where level is at least LEVEL\n"
           "                            LEVEL is 'info', 'warning' (default), 'error', or 'fatal'\n"
           "  -q, --quiet               do not print messages, only return the status code\n"
           "  -x, --xml                 generate an XML report\n"
           "\nUpdate options:\n"
           "  -m, --print-metadata      print metadata to stdout after update using\n"
           "                            the specified format\n"
           "  -a, --add=NAME=VALUE      add a metadata string value to the output file\n"
           "  -s, --no-lastsecond       do not create the onLastSecond tag\n"
           "  -p, --preserve            preserve input file existing onMetadata tags\n"
           "  -f, --fix                 fix invalid tags from the input file\n"
           "  -i, --ignore              ignore invalid tags from the input file\n"
           "                            (the default is to stop with an error)\n"
           "  -t, --reset-timestamps    reset timestamps so OUTPUT_FILE starts at zero\n"
           "  -k, --all-keyframes       index all keyframe tags, including duplicate timestamps\n"
           "\nCommon options:\n"
           "  -v, --verbose             display informative messages\n"
           "\nMiscellaneous:\n"
           "  -V, --version             print version information and exit\n"
           "  -h, --help                display this information and exit\n");
    printf("\nPlease report bugs to <%s>\n", PACKAGE_BUGREPORT);
}

static int parse_command_line(int argc, char ** argv, flvmeta_opts * options) {
    int option, option_index;

    option_index = 0;
    do {
        option = getopt_long(argc, argv, 
            DUMP_COMMAND
            FULL_DUMP_COMMAND
            CHECK_COMMAND
            UPDATE_COMMAND
            DUMP_FORMAT_OPTION
            JSON_OPTION
            RAW_OPTION
            XML_OPTION
            YAML_OPTION
            EVENT_OPTION
            LEVEL_OPTION
            QUIET_OPTION
            PRINT_METADATA_OPTION
            ADD_OPTION
            NO_LASTSECOND_OPTION
            PRESERVE_OPTION
            FIX_OPTION
            IGNORE_OPTION
            RESET_TIMESTAMPS_OPTION
            ALL_KEYFRAMES_OPTION
            VERBOSE_OPTION
            VERSION_OPTION
            HELP_OPTION,
            long_options, &option_index);
        switch (option) {
            /*
                commands
            */
            case 'D':
                if (options->command != FLVMETA_DEFAULT_COMMAND) {
                    fprintf(stderr, "%s: only one command can be specified -- %s\n", argv[0], argv[optind]);
                    return EXIT_FAILURE;
                }
                options->command = FLVMETA_DUMP_COMMAND;
                break;
            case 'F':
                if (options->command != FLVMETA_DEFAULT_COMMAND) {
                    fprintf(stderr, "%s: only one command can be specified -- %s\n", argv[0], argv[optind]);
                    return EXIT_FAILURE;
                }
                options->command = FLVMETA_FULL_DUMP_COMMAND;
                break;
            case 'C':
                if (options->command != FLVMETA_DEFAULT_COMMAND) {
                    fprintf(stderr, "%s: only one command can be specified -- %s\n", argv[0], argv[optind]);
                    return EXIT_FAILURE;
                }
                options->command = FLVMETA_CHECK_COMMAND;
                break;
            case 'U':
                if (options->command != FLVMETA_DEFAULT_COMMAND) {
                    fprintf(stderr, "%s: only one command can be specified -- %s\n", argv[0], argv[optind]);
                    return EXIT_FAILURE;
                }
                options->command = FLVMETA_UPDATE_COMMAND;
                break;
            /*
                options
            */
            /* check options */
            case 'l':
                if (!strcmp(optarg, "info")) {
                    options->check_level = FLVMETA_CHECK_LEVEL_INFO;
                }
                else if (!strcmp(optarg, "warning")) {
                    options->check_level = FLVMETA_CHECK_LEVEL_WARNING;
                }
                else if (!strcmp(optarg, "error")) {
                    options->check_level = FLVMETA_CHECK_LEVEL_ERROR;
                }
                else if (!strcmp(optarg, "fatal")) {
                    options->check_level = FLVMETA_CHECK_LEVEL_FATAL;
                }
                else {
                    fprintf(stderr, "%s: invalid message level -- %s\n", argv[0], optarg);
                    usage(argv[0]);
                    return EXIT_FAILURE;
                }
                break;
            case 'q': options->quiet = 1; break;
            /* dump options */
            case 'd':
                if (!strcmp(optarg, "xml")) {
                    options->dump_format = FLVMETA_FORMAT_XML;
                }
                if (!strcmp(optarg, "raw")) {
                    options->dump_format = FLVMETA_FORMAT_RAW;
                }
                else if (!strcmp(optarg, "json")) {
                    options->dump_format = FLVMETA_FORMAT_JSON;
                }
                else if (!strcmp(optarg, "yaml")) {
                    options->dump_format = FLVMETA_FORMAT_YAML;
                }
                else {
                    fprintf(stderr, "%s: invalid output format -- %s\n", argv[0], optarg);
                    usage(argv[0]);
                    return EXIT_FAILURE;
                }
                break;
            case 'j': options->dump_format = FLVMETA_FORMAT_JSON;    break;
            case 'r': options->dump_format = FLVMETA_FORMAT_RAW;     break;
            case 'x':
                /* xml dump format, or generation of xml report */
                options->dump_format = FLVMETA_FORMAT_XML;
                options->check_xml_report = 1;
                break;
            case 'y': options->dump_format = FLVMETA_FORMAT_YAML;    break;
            case 'e': options->metadata_event = optarg;              break;
            /* update options */
            case 'm': options->dump_metadata = 1;                    break;
            case 'a':
                {
                    char * eq_pos;
                    eq_pos = strchr(optarg, '=');
                    if (eq_pos != NULL) {
                        *eq_pos = 0;
                        if (options->metadata == NULL) {
                            options->metadata = amf_associative_array_new();
                        }
                        amf_associative_array_add(options->metadata, optarg, amf_str(eq_pos + 1));
                    }
                    else {
                        fprintf(stderr, "%s: invalid metadata format -- %s\n", argv[0], optarg);
                        usage(argv[0]);
                        return EXIT_FAILURE;
                    }
                } break;
            case 's': options->insert_onlastsecond = 0;                  break;
            case 'p': options->preserve_metadata = 1;                    break;
            case 'f': options->error_handling = FLVMETA_FIX_ERRORS;      break;
            case 'i': options->error_handling = FLVMETA_IGNORE_ERRORS;   break;
            case 't': options->reset_timestamps = 1;                     break;
            case 'k': options->all_keyframes = 1;                        break;
            
            /*
                common options
            */
            case 'v': options->verbose = 1;  break;
            /*
                Miscellaneous
            */
            case 'V':
                options->command = FLVMETA_VERSION_COMMAND;
                return OK;
                break;
            case 'h':
                options->command = FLVMETA_HELP_COMMAND;
                return OK;
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
                return EXIT_FAILURE;
        }
    } while (option != EOF);

    /* input filename */
    if (optind > 0 && optind < argc) {
        options->input_file = argv[optind];
    }
    else {
        fprintf(stderr, "%s: no input file\n", argv[0]);
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* output filename */
    if (++optind < argc) {
        options->output_file = argv[optind];
    }

    /* determine command if default */
    if (options->command == FLVMETA_DEFAULT_COMMAND && options->output_file != NULL) {
        options->command = FLVMETA_UPDATE_COMMAND;
    }
    else if (options->command == FLVMETA_DEFAULT_COMMAND && options->output_file == NULL) {
        options->command = FLVMETA_DUMP_COMMAND;
    }

    /* output file is input file if not specified */
    if (options->output_file == NULL) {
        options->output_file = options->input_file;
    }

    return OK;
}

int main(int argc, char ** argv) {
    int errcode;

    /* flvmeta default options */
    static flvmeta_opts options;
    options.command = FLVMETA_DEFAULT_COMMAND;
    options.input_file = NULL;
    options.output_file = NULL;
    options.metadata = NULL;
    options.check_level = FLVMETA_CHECK_LEVEL_WARNING;
    options.quiet = 0;
    options.check_xml_report = 0;
    options.dump_metadata = 0;
    options.insert_onlastsecond = 1;
    options.reset_timestamps = 0;
    options.all_keyframes = 0;
    options.preserve_metadata = 0;
    options.error_handling = FLVMETA_EXIT_ON_ERROR;
    options.dump_format = FLVMETA_FORMAT_XML;
    options.verbose = 0;
    options.metadata_event = NULL;


    /* Command-line parsing */
    errcode = parse_command_line(argc, argv, &options);

    /* free metadata if necessary */
    if ((errcode != OK || options.command != FLVMETA_UPDATE_COMMAND) && options.metadata != NULL) {
        amf_data_free(options.metadata);
    }

    if (errcode == OK) {
        /* execute command */
        switch (options.command) {
            case FLVMETA_DUMP_COMMAND: errcode = dump_metadata(&options); break;
            case FLVMETA_FULL_DUMP_COMMAND: errcode = dump_flv_file(&options); break;
            case FLVMETA_CHECK_COMMAND: errcode = check_flv_file(&options); break;
            case FLVMETA_UPDATE_COMMAND: errcode = update_metadata(&options); break;
            case FLVMETA_VERSION_COMMAND: version(); break;
            case FLVMETA_HELP_COMMAND: help(argv[0]); break;
        }

        /* error report */
        switch (errcode) {
            case ERROR_OPEN_READ: fprintf(stderr, "%s: cannot open %s for reading\n", argv[0], options.input_file); break;
            case ERROR_NO_FLV: fprintf(stderr, "%s: %s is not a valid FLV file\n", argv[0], options.input_file); break;
            case ERROR_EOF: fprintf(stderr, "%s: unexpected end of file\n", argv[0]); break;
            case ERROR_MEMORY: fprintf(stderr, "%s: memory allocation error\n", argv[0]); break;
            case ERROR_EMPTY_TAG: fprintf(stderr, "%s: empty FLV tag\n", argv[0]); break;
            case ERROR_OPEN_WRITE: fprintf(stderr, "%s: cannot open %s for writing\n", argv[0], options.output_file); break;
            case ERROR_INVALID_TAG: fprintf(stderr, "%s: invalid FLV tag\n", argv[0]); break;
            case ERROR_WRITE: fprintf(stderr, "%s: unable to write to %s\n", argv[0], options.output_file); break;
            case ERROR_SAME_FILE: fprintf(stderr, "%s: input file and output file must be different\n", argv[0]); break;
        }
    }

    return errcode;
}
