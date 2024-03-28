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

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s --cpu|--sd|--ram|--nic [options]\n", argv[0]);
        return 1;
    }

    char *app;
    // Adjust the array size based on the maximum number of arguments any app can take.
    char *args[8]; 

    // CPU, SD, and NIC options now all include an interval (in milliseconds) and a timeout (in seconds).
    if (strcmp(argv[1], "--cpu") == 0 || strcmp(argv[1], "--sd") == 0 || strcmp(argv[1], "--nic") == 0) {
        if (argc != 8) { // Expecting 7 arguments plus the program name, total 8
            printf("Usage for CPU/SD/NIC: %s %s -p [PID] -i [interval in milliseconds] -t [timeout in seconds]\n", argv[0], argv[1]);
            return 1;
        }

        if (strcmp(argv[1], "--cpu") == 0) {
            app = "ePerfCPU";
        } else if (strcmp(argv[1], "--sd") == 0) {
            app = "ePerfSD";
        } else { // --nic
            app = "ePerfNIC";
        }

        args[0] = app;
        args[1] = argv[2]; // "-p"
        args[2] = argv[3]; // PID
        args[3] = argv[4]; // "-i"
        args[4] = argv[5]; // Interval in milliseconds
        args[5] = argv[6]; // "-t"
        args[6] = argv[7]; // Timeout in seconds
        args[7] = NULL; // NULL-terminate the arguments.
        
    } else if (strcmp(argv[1], "--ram") == 0) {
        // RAM -> perf does not accept the "-i"
        if (argc != 6) { // Expecting 4 arguments plus the program name, total 5
            printf("Usage for RAM: %s %s -p [PID] -t [timeout in seconds]\n", argv[0], argv[1]);
            return 1;
        }
        app = "ePerfRAM";
        args[0] = app;
        args[1] = argv[2]; // "-p"
        args[2] = argv[3]; // PID
        args[3] = argv[4]; // "-t"
        args[4] = argv[5]; // Timeout in seconds
        args[5] = NULL; // NULL-terminate the arguments.
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
