/*
 * Copyright (C) 2020 Université Grenoble Alpes
 */

/**
 * Git information
 * Thanks to https://nullpointer.io/post/easily-embed-version-information-in-software-releases/
 */

#include <stdio.h>

#define UNKNOWN		"UNKNOWN"

static const char* get_software_git_version(void) {
#ifdef GIT_VERSION
	return GIT_VERSION;
#else
	return UNKNOWN;
#endif
}

static const char* get_software_git_commit(void) {
#ifdef GIT_COMMIT
	return GIT_COMMIT;
#else
	return UNKNOWN;
#endif
}

static const char* get_software_git_date(void) {
#ifdef GIT_DATE
	return GIT_DATE;
#else
	return UNKNOWN;
#endif
}

static const char* get_software_build_date(void) {
#ifdef BUILD_DATE
	return BUILD_DATE;
#else
	return UNKNOWN;
#endif
}

/*
 * @brief git command
 *
 * @param argc
 * @param argv
 */
int git_cmd(int argc, char *argv[]) {
	(void) (argc);
	(void) (argv);

#if defined(BANNER)
	printf("%s\n", BANNER);
#elif defined(RIOT_APPLICATION)
	printf("Application: %s\n", RIOT_APPLICATION);
#endif
	printf("Version:     %s\n", get_software_git_version());
	printf("Commit SHA1: %s\n", get_software_git_commit());
	printf("Commit date: %s\n", get_software_git_date());
	printf("Build date:  %s\n", get_software_build_date());

	return 0;
}
