The program consists of only one file: memsim.cpp.

To compile, please use the makefile by executing 'make' at the command line. The makefile simply contains the compile command g++ -std=c++11 -w memsim.cpp -o memsim.

To execute the program after compiling the program using the 'make' command, here is the expected format:
./memsim FRAME_SIZE INPUT_FILE OUTPUT_FILE [CSV=1]

Note that the last argument, when containing only a 1, will output a CSV file with the results. This CSV file was used for making the graphs found in my Lab 3 Analysis document. This argument is entirely optional, so if it is empty or contains anything but a 1, not CSV file will be produced.

Please note that when running, the Optimal algorithm will take a few minutes to complete. I provided a progress bar for the Optimal algorithm. The FIFO, LRU and MFU algorithms take only seconds to complete.

This submission has been compiled and tested at csegrid and no errors occured.