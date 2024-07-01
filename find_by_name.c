/*Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
*/


#include "find_by_name.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>

bool isPID(const struct dirent *entry)   
{
    for (const char * p = entry->d_name; *p; p++) 
    {
        if (!isdigit(*p)) 
        {
            return false;
        }
    }
    return true;
}

int * getAllPID ()
{
    int arraySize = INITIAL_ARRAY_SIZE;
    int elementPosition = 1;

    DIR *procdir;
    struct dirent *entry;

    int *storePID = (int *) malloc(arraySize * sizeof(int));
    if (storePID == NULL) 
    {
        printf("Something wrong happened allocating memory");
        exit(1);
    }

    procdir = opendir("/proc");
    if (procdir == NULL) 
    {
        printf("/proc couldn´t be opened");
        exit(2);
    }

    while ((entry = readdir(procdir))) 
    {
        if (!isPID(entry)) 
        {
            continue;
        } else 
        {
            storePID[elementPosition] = atoi(entry->d_name);
            elementPosition++;
            if (elementPosition == arraySize) 
            {
                arraySize += INITIAL_ARRAY_SIZE;
                storePID = (int *)realloc(storePID, arraySize * sizeof(int));
            }
        }
    }
    storePID[0] = elementPosition;

    closedir(procdir);
    return storePID;
}

int * getAllNamePID (char * commToFind) 
{
    char * commToFindFixed = (char*) malloc(strlen(commToFind) + 2 * sizeof(char));
    strcpy(commToFindFixed, commToFind);
    strcat(commToFindFixed, "\n");
    int arraySize = INITIAL_ARRAY_SIZE;
    int elementPosition = 1;

    FILE* fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    char path[256];
    int *storeMatchesPID = (int *)malloc(arraySize *sizeof(int));

    

    int * storeAllPID = getAllPID();
    int size = storeAllPID[0];
    int pid = 0;

    for (int i=1; i < size; i++) 
    {
        pid = storeAllPID[i];
        snprintf(path, sizeof(path), PROC_COMM_PATH_FORMAT, pid);

        fp = fopen(path, "r");
        if (fp == NULL) {continue;}

        if (read = getline(&line, &len, fp) != -1) 
        {
            if (strcmp(commToFindFixed, line) == 0) 
            {
                storeMatchesPID[elementPosition] = pid;
                elementPosition++;
                if (elementPosition == arraySize) 
                {
                    arraySize += INITIAL_ARRAY_SIZE;
                    storeMatchesPID = (int *)realloc(storeMatchesPID, arraySize * sizeof(int));
                }
            }
        }
        fclose(fp);
    }
    storeMatchesPID[0] = elementPosition;

    
    return storeMatchesPID;
}









