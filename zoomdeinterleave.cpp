#include <iostream>
#include <fstream>
#include <string>
#include <cstring> // memset()

#define ZOOM_FAT_SECTOR_SIZE 512 // In bytes
#define ZOOM_FAT_CLUSTER_SIZE 64 // In sectors
#define ZOOM_INTERLEAVE_SIZE 16 // In clusters
#define ZOOM_INTERLEAVE_SIZE_DEVIATION 1 // Zoom H4n v1.90 alternates between writing 16 clusters of each file, writing 17 clusters of each etc.

using namespace std;

int main(int argc, char *argv[])
{
    string infile;
    string outfile[2];
    uint32_t offset = 0;

    // Count of arguments is the program name itself, plus the arguments to it;
    // from argv[1] and onward is interesting to us
    switch(argc)
    {
    case 2: // Program name, infile
        infile = argv[1];
        outfile[0] = infile + ".M.wav"; // Using these letters sortof mimics the naming
        outfile[1] = infile + ".I.wav"; // as used by the firmware itself
        break;
    case 3: // Program name, offset, infile
        offset = stol(argv[1]);
        infile = argv[2];
        outfile[0] = infile + "+offset.M.wav"; // Using these letters sortof mimics the naming
        outfile[1] = infile + "+offset.I.wav"; // as used by the firmware itself
        break;
    case 4: // Program name, outfile, outfile, infile
        outfile[0] = argv[1];
        outfile[1] = argv[2];
        infile = argv[3];
        break;
    case 5: // Program name, outfile, outfile, offset, infile
        outfile[0] = argv[1];
        outfile[1] = argv[2];
        offset = stol(argv[3]);
        infile = argv[4];
        break;
    default:
        cout << "Usage: " << argv[0] << " [outfile_mics outfile_line] [offset] infile" << endl;
        return 1;
    }

    cout << "Deinterleaving file " << infile << " to files:" << endl << outfile[0] << endl << outfile[1] << endl;

    // Given the Zoom H4n firmware always starts writing at the start of a FAT16/32 cluster, the offset should be
    // a multiplum of the cluster size. The user may however not have used an image of a partition, the user could have
    // made an image of the entire disk. In such case the disk label & MBR skew all cluster positions in the image
    // and hence this assumption not applies. For this reason this program takes an offset in bytes, not in clusters.
    // Do however warn the user in the case of an offset not aligning to cluster size
    if(offset % (ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE))
    {
        cout << "Warning: Supplied offset is not a multiplum of the cluster size used by the Zoom H4n firmware." << endl;
        cout << "Data of an audio recording always starts at the beginning of a cluster!" << endl;
        cout << "If you use this program on an image of a partition, you have made a calculation error." << endl;
        cout << "Only when using this program on a full disk image can a valid offset be anything other than a multiplum of " << (ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE) << endl;
    }

    ifstream interleaved(infile);

    if(!interleaved.is_open())
    {
        cout << "Failed to read input file " << infile << endl;
        return 2;
    }

    ofstream deinterleaved1(outfile[0], ios::out | ios::app | ios::binary); // Input-output stream,
    ofstream deinterleaved2(outfile[1], ios::out | ios::app | ios::binary); // out, append, binary

    if(offset != 0)
    {
        if(!interleaved.seekg(offset, ios_base::beg)) // Set offset relative to begin of stream
        {
            cout << "Failed to seek to byte offset " << offset << ". Offset bigger than file?" << endl;
            return 2;
        }
        cout << "Starting deinterleaving at cluster " << (offset / (ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE)) << endl;
    }

    uint64_t bytecount = 0;

    for(uint32_t i = 0; !interleaved.eof(); i++)
    {
        uint32_t chunksize = ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE * ZOOM_INTERLEAVE_SIZE;
        if((i / 2) % 2 == 1)
            chunksize += ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE * ZOOM_INTERLEAVE_SIZE_DEVIATION;

        char buffer[chunksize];
        memset(buffer, 0, chunksize);

        interleaved.read(buffer, chunksize);

        if(i % 2 == 0)
            deinterleaved1.write(buffer, chunksize);
        else
            deinterleaved2.write(buffer, chunksize);

        bytecount += chunksize;
    }

    cout << "Deinterleaved " << dec << noshowbase << bytecount << " bytes (" << (bytecount / (ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE)) << " clusters)" << endl;

    return 0;
}
