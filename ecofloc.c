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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s --cpu|--sd|--ram|--nic [options]\n", argv[0]);
        return 1;
    }

    char *app;
    // Adjust the array size based on the maximum number of arguments any app can take.
    char *args[argc + 1];  // +1 to ensure space for NULL termination

    // CPU, RAM, SD, and NIC options all include an interval (in milliseconds) and a timeout (in seconds).
    if (strcmp(argv[1], "--cpu") == 0 || strcmp(argv[1], "--ram") == 0 || strcmp(argv[1], "--sd") == 0 || strcmp(argv[1], "--nic") == 0) 
    {
        // Expecting 7 arguments plus the program name, total 8 for minimal setup without optional -d and -f
        if (argc < 8) {
            printf("Usage for CPU/RAM/SD/NIC: %s %s [ -p [PID] or -n [ProcessName] ] -i [interval in milliseconds] -t [timeout in seconds] [-d [true|false]] [-f [filename]]\n", argv[0], argv[1]);
            return 1;
        }

        if (strcmp(argv[1], "--cpu") == 0) {
            app = "ecofloc-cpu";
        } else if (strcmp(argv[1], "--ram") == 0) {
            app = "ecofloc-ram";
        } else if (strcmp(argv[1], "--sd") == 0) {
            app = "ecofloc-sd";
        } else if (strcmp(argv[1], "--nic") == 0) {
            app = "ecofloc-nic";
        }

        args[0] = app;

        for (int i = 2; i < argc; i++) {
            args[i - 1] = argv[i];
        }
        args[argc - 1] = NULL; // NULL-terminate the arguments.

    } else {
        printf("Invalid option: %s\n", argv[1]);
        return 1;
    }

    // Execute the chosen application with the provided arguments.
    if (execvp(app, args) == -1) {
        perror("Error executing the application");
        return 1;
    }

    return 0; // This point is never reached.
}