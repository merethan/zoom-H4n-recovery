#include <iostream>
#include <fstream>
#include <string>
#include <cstring> // memset()

#define ZOOM_FAT32_SECTOR_SIZE 512 // In bytes
#define ZOOM_FAT32_CLUSTER_SIZE 64 // In sectors
#define ZOOM_INTERLEAVE_SIZE 16 // In clusters
#define ZOOM_INTERLEAVE_SIZE_DEVIATION 1 // Zoom H4n v1.90 alternates between writing 16 clusters of each file, writing 17 clusters of each etc.

using namespace std;

int main(int argc, char *argv[])
{
    string infile;
    string outfile[2];

    // Count of arguments is the program name itself, plus the arguments to it;
    // from argv[1] and onward is interesting to us
    switch(argc)
    {
    case 2: // One argument in addition to the program name
        infile = argv[1];
        outfile[0] = infile + ".m.wav";
        outfile[1] = infile + ".i.wav";
        break;
    case 4: // Three arguments in addition to the program name
        outfile[0] = argv[1];
        outfile[1] = argv[2];
        infile = argv[3];
        break;
    default:
        cout << "Usage: [outfile_mics outfile_line] infile" << endl;
        return 1;
    }

    cout << "Demuxing file " <<  infile << endl;
    cout << "to files" << endl << outfile[0] << endl << outfile[1] << endl;

    ifstream interleaved(infile);
    ofstream deinterleaved1(outfile[0], ios::out | ios::app | ios::binary); // Input-output stream,
    ofstream deinterleaved2(outfile[1], ios::out | ios::app | ios::binary); // out, append, binary

    for(uint32_t i = 0; !interleaved.eof(); i++)
    {
        uint32_t chunksize = ZOOM_FAT32_SECTOR_SIZE * ZOOM_FAT32_CLUSTER_SIZE * ZOOM_INTERLEAVE_SIZE;
        if((i / 2) % 2 == 1)
            chunksize += ZOOM_FAT32_SECTOR_SIZE * ZOOM_FAT32_CLUSTER_SIZE * ZOOM_INTERLEAVE_SIZE_DEVIATION;

        char buffer[chunksize];
        memset(buffer, 0, chunksize);

        cout << "Writing chunk " << dec << noshowbase << i; // No endl
        cout << " starting at offset " << hex << showbase << interleaved.tellg() << endl;

        interleaved.read(buffer, chunksize);

        if(i % 2 == 0)
            deinterleaved1.write(buffer, chunksize);
        else
            deinterleaved2.write(buffer, chunksize);

        cout << "Deinterleaved " << dec << noshowbase << interleaved.gcount() << " bytes" << endl;
    }

    return 0;
}
