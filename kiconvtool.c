/*-
 * Copyright (c) 2008-2010 Dmitry Marakasov <amdmi3@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <err.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/iconv.h>
#include <sys/sysctl.h>
#include <string.h>
#include <memstat.h>

enum arg_type_t_ {
	ARGTYPE_LOCAL,
	ARGTYPE_FOREIGN,
	ARGTYPE_PAIR,
};

/* flags */
int flag_verbose = 0;
int flag_memstat = 0;
int flag_cslist = 0;

/* arrays of charset/pair names */
char **local_charsets = 0;
char **foreign_charsets = 0;
char **pairs = 0;

/* count of charsets/pairs in corresonding arrays */
int num_local_charsets = 0;
int num_foreign_charsets = 0;
int num_pairs = 0;

static const char usage_string[] = "kiconvtool [-hvmd] [-l charset ...] [-f charset ...] [-p pair ...]";

/* safe free of memory allocated to arrays */
void cleanup() {
	if (local_charsets)
		free(local_charsets);

	if (foreign_charsets)
		free(foreign_charsets);

	if (pairs)
		free(pairs);
}

/* one-line usage */
void usage() {
	fprintf(stderr, "usage: %s\n", usage_string);
	exit(1);
}

/* verbose help */
void help() {
	printf("kiconvtool: load kernel iconv character conversion tables\n");
	printf("usage: %s\n\n", usage_string);
	printf("Arguments:\n");
	printf("  -h    display this help\n");
	printf("  -v    be verbose (report every action)\n");
	printf("  -m    display kernel memory usage statistics for iconv data\n");
	printf("  -d    display list of loaded charset pairs\n");
	printf("  -l    specify one or more local charsets to load\n");
	printf("  -f    specify one or more foreign charsets to load\n");
	printf("  -p    specify one or more charset pairs (in form local:foreign) to load\n");
	printf("  Each local charset (-l) creates a pair with each foreign charset (-f)\n\n");

	printf("Example:\n\n");
	printf("   kiconvtool -p KOI8-R:CP866 KOI8-R:UTF8-BE\n\n");
	printf("   or (does the same):\n\n");
	printf("   kiconvtool -l KOI8-R -f CP866 UTF8-BE\n\n");
	exit(0);
}

/* displays kernel memory allocation statistics for iconv tables */
int memstat() {
	struct memory_type_list *mtlp;
	struct memory_type *mtp;

	if ((mtlp = memstat_mtl_alloc()) == NULL)
		warn("memstat_mtl_alloc");

	if (memstat_sysctl_malloc(mtlp, 0) < 0) {
		warn("memstat_sysctl_malloc");
		memstat_mtl_free(mtlp);
		return 1;
	}

	if ((mtp = memstat_mtl_find(mtlp, ALLOCATOR_MALLOC, "iconv_data")) == NULL) {
		warnx("memstat_mtl_find: %s", memstat_strerror(memstat_mtl_geterror(mtlp)));
		memstat_mtl_free(mtlp);
		return 1;
	}

	printf("%"PRIu64" allocations, %"PRIu64" bytes\n", memstat_get_count(mtp), (uint64_t)memstat_get_bytes(mtp));

	memstat_mtl_free(mtlp);
	return 0;
}

/* list currently loaded cs pairs */
int cslist() {
	size_t len;
	struct iconv_cspair_info *csi;
	struct iconv_cspair_info *curr;
	int i;

	if (sysctlbyname("kern.iconv.cslist", NULL, &len, NULL, 0) == -1) {
		warn("sysctlbyname");
		return 1;
	}

	if ((csi = (struct iconv_cspair_info*)malloc(len)) == NULL) {
		warn("malloc");
		return 1;
	}

	if (sysctlbyname("kern.iconv.cslist", csi, &len, NULL, 0) == -1) {
		warn("sysctlbyname");
		free(csi);
		return 1;
	}

	for (i = 0, curr = csi; i < len/sizeof(struct iconv_cspair_info); i++, curr++)
		printf("%s -> %s\n", curr->cs_from, curr->cs_to);

	free(csi);
	return 0;
}

/* loads charset pair into kernel iconv */
int load_pair(const char *local, const char *foreign) {
	if (kiconv_add_xlat16_cspairs(foreign, local) != 0) {
		warn("kiconv_add_xlat16_cspairs(%s:%s)", local, foreign);
		return 1;
	}

	if (kiconv_add_xlat16_cspair(KICONV_WCTYPE_NAME, local, KICONV_WCTYPE) != 0) {
		warn("kiconv_add_xlat16_cspair(%s:%s)", local, KICONV_WCTYPE_NAME);
	}

	if (flag_verbose)
		fprintf(stderr, "loaded charset pair: %s:%s\n", local, foreign);

	return 0;
}

int main(int argc, char **argv) {
	int i;
	int errors = 0;

	/* parse command line */
	if (argc <= 1) {
		usage();
		return 1;
	}

	if ( ((local_charsets = malloc(argc * sizeof(char*))) == NULL) ||
		((foreign_charsets = malloc(argc * sizeof(char*))) == NULL) ||
		((pairs = malloc(argc * sizeof(char*))) == NULL)) {
		warn("malloc");
		cleanup();
		return 1;
	}

	enum arg_type_t_ arg_type = ARGTYPE_PAIR;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			char *c;
			for (c = argv[i] + 1; *c != '\0'; c++)
				switch(*c) {
				case 'l': arg_type = ARGTYPE_LOCAL; break;
				case 'f': arg_type = ARGTYPE_FOREIGN; break;
				case 'p': arg_type = ARGTYPE_PAIR; break;
				case 'v': flag_verbose = 1; break;
				case 'm': flag_memstat = 1; break;
				case 'd': flag_cslist = 1; break;
				case 'h':
					help();
					cleanup();
					break;
				default:
					usage();
					cleanup();
					break;
				}
		} else {
			switch(arg_type) {
			case ARGTYPE_LOCAL:
				local_charsets[num_local_charsets++] = argv[i];
				break;
			case ARGTYPE_FOREIGN:
				foreign_charsets[num_foreign_charsets++] = argv[i];
				break;
			case ARGTYPE_PAIR:
			default:
				pairs[num_pairs++] = argv[i];
				break;
			}
		}
	}

	/* compose and load all foreign/local pairs */
	int f, l;
	for (f = 0; f < num_foreign_charsets; f++)
		for (l = 0; l < num_local_charsets; l++)
			errors += load_pair(local_charsets[l], foreign_charsets[f]);

	/* load explicit pairs */
	int p;
	for (p = 0; p < num_pairs; p++) {
		char *local = pairs[p];
		char *foreign = index(pairs[p], ':');

		if (foreign == NULL) {
			errors++;
			warnx("malformed charset pair: %s\n", pairs[p]);
			continue;
		}

		*(foreign++) = '\0';

		errors += load_pair(local, foreign);
	}

	cleanup();

	/* memory statistics */
	if (flag_memstat)
		errors += memstat();

	/* charset list statistics */
	if (flag_cslist)
		errors += cslist();

	return errors;
}
