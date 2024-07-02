 /*
Licensed to the Apache Software Foundation (ASF) under one
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



//watch -n 1 nvidia-smi --query-gpu=power.draw --format=csv,noheader,nounits
// nvidia-smi pmon -s um -d 1 -c 3600 ()



#include"pid_energy.h"

int main()
{
    int pid = 13234;           
    double interval_ms = 1000;
    double timeout_s = 5;   

    double energy_consumed = pid_energy(pid, interval_ms, timeout_s);
    printf("Estimated energy consumed by PID %d over %.2f seconds: %.2f Joules\n", pid, timeout_s, energy_consumed);

    return 0;
}