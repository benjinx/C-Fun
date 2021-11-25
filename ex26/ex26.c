/*

Name: Logfind

Description: Search through log files for text. This tool is a specialized version of another tool called grep, but
designed only for log files on a system. The idea is that i can type:

    logfind zedshaw

The list of file names can be anything that the glob function allows. Refer to man 3 glob to see how this works.
I suggest starting with just a flat list of exact files, and then add glob functionality.

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <glob.h>

int readLogFileList(char *logFileNameList[], int maxLogFiles)
{
    int numOfLogFiles = 0;

    // Create filename
    char filename[PATH_MAX];

    // Actually create our file name/location of logfind via HOME for '~/'
    snprintf(filename, sizeof(filename),
        "%s/.logfind", getenv("HOME"));

    // Print where we're checking
    printf("Checking: %s\n", filename);

    // Read in the file
    FILE* file = fopen(filename, "rt");

    // Error handling
    if (!file) {
        perror("fopen");
        goto error;
    }

    char line[4096];

    // Loop thru
    for (int i = 0; i < maxLogFiles; i++)
    {
        // Read
        char *result = fgets(line, sizeof(line), file);
        if (!result) {
            break;
        }

        // Fix line ending
        char * foundChar = strchr(line, '\n');

        if (foundChar) {
            *foundChar = '\0';
        }

        logFileNameList[i] = strndup(line, sizeof(line));

        numOfLogFiles++;
    }

    // Close the file
    fclose(file);

    return numOfLogFiles;

error:

    return -1;
}

int searchFile(char *filename, int numSearchTerms, char *searchTerms[], bool isOr)
{
    // Open passed in file
    FILE* file = fopen(filename, "rt");

    // Error handling
    if (!file) {
        //perror("fopen");
        goto error;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Create a chunk
    char *buffer = malloc((size + 1) * sizeof(char));

    if (!buffer) {
        printf("Please sir may I have some more memory?\n");
        printf("Who are you?\n");
    }

    // Read in the data from the file
    fread(buffer, sizeof(char), size, file);

    buffer[size] = '\0';

    bool found = (isOr ? false : true);

    // Parse the data for any of the "keywords" in argv
    for (int i = 0; i < numSearchTerms; i++)
    {
        char *result = strstr(buffer, searchTerms[i]);

        if (isOr) {
            if (result) {
                found = true;
                break;
            }
        }
        else
        {
            if (!result) {
                found = false;
                break;
            }
        }
    }

    // if it has any print out the filename
    if (found) {
        printf("Found match in: %s\n", filename);
    }

    free(buffer);

    fclose(file);

    return 0;
error:
    return -1;
}

int main(int argc, char *argv[])
{
    // Store log filenames found
    char *logFileNameList[256] = { NULL };

    int numOfLogFiles = readLogFileList(logFileNameList, sizeof(logFileNameList) / sizeof(char*));

    char *searchTerms[10] = { NULL };

    bool isOr = false;

    int numSearchTerms = 0;

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'o')
            {
                isOr = true;
            }
        }
        else {
            searchTerms[numSearchTerms] = strndup(argv[i], 256);
            ++numSearchTerms;
        }
    }

    // Search
    for (int i = 0; i < numOfLogFiles; i++)
    {
        glob_t slimeBall;
        int result = glob(logFileNameList[i], 0, NULL, &slimeBall);

        for (unsigned long j = 0; j < slimeBall.gl_pathc; ++j) {
            searchFile(slimeBall.gl_pathv[j], numSearchTerms, searchTerms, isOr);
        }

        globfree(&slimeBall);
    }

    // Clean up
    for (int i = 0; i < numSearchTerms; ++i) {
        free(searchTerms[i]);
    }

    for (int i = 0; i < numOfLogFiles; ++i) {
        free(logFileNameList[i]);
    }

    return 0;

// error:

//     return -1;
}