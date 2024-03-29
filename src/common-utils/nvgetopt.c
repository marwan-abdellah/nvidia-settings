/*
 * Copyright (C) 2004-2010 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *      Free Software Foundation, Inc.
 *      59 Temple Place - Suite 330
 *      Boston, MA 02111-1307, USA
 *
 *
 * nvgetopt.c - portable getopt_long() replacement; removes the need
 * for the stupid optstring argument.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nvgetopt.h"


int nvgetopt(int argc,
             char *argv[],
             const NVGetoptOption *options,
             char **strval,
             int *boolval,
             int *intval,
             double *doubleval,
             int *disable_val)
{
    char *c, *a, *arg, *name = NULL, *argument=NULL;
    int i, found = NVGETOPT_FALSE;
    int ret = 0;
    int negate = NVGETOPT_FALSE;
    int disable = NVGETOPT_FALSE;
    const NVGetoptOption *o = NULL;
    static int argv_index = 0;

    if (strval) *strval = NULL;
    if (boolval) *boolval = NVGETOPT_FALSE;
    if (intval) *intval = 0;
    if (doubleval) *doubleval = 0.0;
    if (disable_val) *disable_val = NVGETOPT_FALSE;

    argv_index++;

    /* if no more options, return -1 */

    if (argv_index >= argc) return -1;

    /* get the argument in question */

    arg = strdup(argv[argv_index]);

    /* look for "--" or "-" */

    if ((arg[0] == '-') && (arg[1] == '-')) {
        name = arg + 2;
    } else if (arg[0] == '-') {
        name = arg + 1;
    } else {
        fprintf(stderr, "%s: invalid option: \"%s\"\n", argv[0], arg);
        goto done;
    }

    /*
     * if there is an "=" in the string, then assign argument and zero
     * out the equal sign so that name will match what is in the
     * option table.
     */

    c = name;
    while (*c) {
        if (*c == '=') { argument = c + 1; *c = '\0'; break; }
        c++;
    }

    /*
     * if the string is terminated after one character, interpret it
     * as a short option.  Otherwise, interpret it as a long option.
     */

    if (name[1] == '\0') { /* short option */
        for (i = 0; options[i].name; i++) {
            if (options[i].val == name[0]) {
                o = &options[i];
                break;
            }
        }
    } else { /* long option */
        for (i = 0; options[i].name; i++) {
            const char *tmpname;
            int tmp_negate;

            /*
             * if this option allows negation by prepending with
             * "--no-" (true for IS_BOOLEAN and ALLOW_DISABLE), then
             * skip any leading "no-" in the argument
             */

            if ((options[i].flags & (NVGETOPT_IS_BOOLEAN |
                                     NVGETOPT_ALLOW_DISABLE)) &&
                (name[0] == 'n') &&
                (name[1] == 'o') &&
                (name[2] == '-')) {
                tmpname = name + 3;
                tmp_negate = NVGETOPT_TRUE;
            } else {
                tmpname = name;
                tmp_negate = NVGETOPT_FALSE;
            }

            if (strcmp(tmpname, options[i].name) == 0) {
                o = &options[i];
                negate = tmp_negate;
                break;
            }
        }
    }

    /*
     * if we didn't find a match, maybe this is multiple short options
     * stored together; is each character a short option?
     */

    if (!o) {
        for (c = name; *c; c++) {
            found = NVGETOPT_FALSE;
            for (i = 0; options[i].name; i++) {
                if (options[i].val == *c) {
                    found = NVGETOPT_TRUE;
                    break;
                }
            }
            if (!found) break;
        }

        if (found) {

            /*
             * all characters individually are short options, so
             * interpret them that way
             */

            for (i = 0; options[i].name; i++) {
                if (options[i].val == name[0]) {

                    /*
                     * don't allow options with arguments to be
                     * processed in this way
                     */

                    if (options[i].flags & NVGETOPT_HAS_ARGUMENT) break;

                    /*
                     * remove the first short option from
                     * argv[argv_index]
                     */

                    a = argv[argv_index];
                    if (a[0] == '-') a++;
                    if (a[0] == '-') a++;
                    if (a[0] == '+') a++;

                    while(a[0]) { a[0] = a[1]; a++; }

                    /*
                     * decrement argv_index so that we process this
                     * entry again
                     */

                    argv_index--;

                    o = &options[i];
                    break;
                }
            }
        }
    }

    /* if we didn't find an option, return */

    if (!o) {
        fprintf(stderr, "%s: unrecognized option: \"%s\"\n", argv[0], arg);
        goto done;
    }


    /* if the option is boolean, record !negate as the boolean value */

    if (o->flags & NVGETOPT_IS_BOOLEAN) {
        if (boolval) *boolval = !negate;
    }


    /*
     * if this option is flagged as "disable-able", then let the
     * "--no-" prefix get interpreted to mean that the option should
     * be disabled
     */

    if ((o->flags & NVGETOPT_ALLOW_DISABLE) && (negate == NVGETOPT_TRUE)) {
        disable = NVGETOPT_TRUE;
    }


    /*
     * if the option takes an argument (either string or integer), and
     * we haven't already decided to disable the option, then we
     * either need to use what was after the "=" in this argv[] entry,
     * or we need to pull the next entry off of argv[]
     */

    if ((o->flags & NVGETOPT_HAS_ARGUMENT) && !disable) {
        if (argument) {
            if (!argument[0]) {
                fprintf(stderr, "%s: option \"%s\" requires an "
                        "argument.\n", argv[0], arg);
                goto done;
            }
        } else {

            /*
             * if the argument is optional, and we're either at the
             * end of the argv list, or the next argv starts with '-',
             * then assume there is no argument for this option
             */

            if ((o->flags & NVGETOPT_ARGUMENT_IS_OPTIONAL) &&
                ((argv_index == (argc - 1)) ||
                 (argv[argv_index + 1][0] == '-'))) {
                argument = NULL;
                goto argument_processing_done;
            } else {
                argv_index++;
                if (argv_index >= argc) {
                    fprintf(stderr, "%s: option \"%s\" requires an "
                            "argument.\n", argv[0], arg);
                    goto done;
                }
                argument = argv[argv_index];
            }
        }

        /* argument is now a valid string: parse it */

        if ((o->flags & NVGETOPT_INTEGER_ARGUMENT) && (intval)) {

            /* parse the argument as an integer */

            char *endptr;
            *intval = (int) strtol(argument, &endptr, 0);
            if (*endptr) {
                fprintf(stderr, "%s: \"%s\" is not a valid argument for "
                        "option \"%s\".\n", argv[0], argument, arg);
                goto done;
            }
        } else if ((o->flags & NVGETOPT_STRING_ARGUMENT) && (strval)) {

            /* treat the argument as a string */

            *strval = strdup(argument);
        } else if ((o->flags & NVGETOPT_DOUBLE_ARGUMENT) && (doubleval)) {

            /* parse the argument as a double */

            char *endptr;
            *doubleval = (double) strtod(argument, &endptr);
            if (*endptr) {
                fprintf(stderr, "%s: \"%s\" is not a valid argument for "
                        "option \"%s\".\n", argv[0], argument, arg);
                goto done;
            }
        } else {
            fprintf(stderr, "%s: error while assigning argument for "
                    "option \"%s\".\n", argv[0], arg);
            goto done;
        }

    } else {

        /* if we have an argument when we shouldn't; complain */

        if (argument) {
            fprintf(stderr, "%s: option \"%s\" does not take an argument, but "
                    "was given an argument of \"%s\".\n",
                    argv[0], arg, argument);
            goto done;
        }
    }

 argument_processing_done:

    ret = o->val;

    /* fall through */

 done:

    if (disable_val) *disable_val = disable;

    free(arg);
    return ret;

} /* nvgetopt() */
