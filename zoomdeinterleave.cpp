#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cstring> // memset()

// The following are insisted upon by the Zoom H4n & H6 firmware, which is why they like doing the formatting of SD-cards themselves.
// The device manuals even state you must have the device do the formatting for you in order to use the card. Now you know why
#define ZOOM_FAT_SECTOR_SIZE 512 // In bytes
#define ZOOM_FAT_CLUSTER_SIZE 64 // In sectors

// Zoom H4n v1.90 starts by writing a cluster for each of 4CHxxxM.wav & 4CHxxxI.wav,
// then starts alternating between writing 16 clusters of each file, writing 17 clusters of each file etc.
// Which I think is a bug in the firmware: I think it was meant to do 1 cluster of each, then 16 of each until end of file.
// To me it looks like 1 + 16 becoming 17 on every uneven iteration apart from the first is a programming error
#define ZOOM_H4n_START_SIZE 1 // In clusters
#define ZOOM_H4n_INTERLEAVE_SIZE 16 // In clusters

// The Zoom H6 seems to have a different scheme, where it reserves 26 clusters for the start of each stereo file,
// then it alternates between the two files with just 2 clusters. And unlike the H4n there's no further alteration there
#define ZOOM_H6_START_SIZE 26 // In clusters
#define ZOOM_H6_INTERLEAVE_SIZE 2 // In clusters

enum DeviceModel
{
    ZOOM_H4n = 0,   // Normal & Pro model; same writing routines
    ZOOM_H6,        // Like H4n & Pro, but different cluster counts
    ZOOM_BROKEN,    // Sometimes buggy firmware does really weird shit, one needs a single-use hack for
};

using namespace std;

int main(int argc, char *argv[])
{
    string infile;
    string outfile[2];
    uint64_t offset = 0; // Maximum filesize 2^64 = 16777216TB (yes terabytes)
    uint64_t limit = 0; // The amount (count) of bytes we'll do
    DeviceModel devicemodel = ZOOM_H4n;

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
        offset = stoll(argv[1]);
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
        offset = stoll(argv[3]);
        infile = argv[4];
        break;
    case 6: // Program name, outfile, outfile, offset, limit, infile
        outfile[0] = argv[1];
        outfile[1] = argv[2];
        offset = stoll(argv[3]);
        limit = stoll(argv[4]);
        infile = argv[5];
        break;
    case 7: // Program name, pattern, outfile, outfile, offset, limit, infile
        devicemodel = (DeviceModel) stoll(argv[1]);
        outfile[0] = argv[2];
        outfile[1] = argv[3];
        offset = stoll(argv[4]);
        limit = stoll(argv[5]);
        infile = argv[6];
        break;
    default:
        cout << "Usage: " << argv[0] << " [pattern] [outfile_mics outfile_line] [offset in bytes] [limit in bytes] infile" << endl;
        return 1;
    }

    cout << "Deinterleaving file " << infile << " to files:" << endl << outfile[0] << endl << outfile[1] << endl;

    // Given Zoom H4n & H6 firmware always starts writing at the start of a FAT16/32 cluster, the offset should be
    // a multiplum of the cluster size. The user may however not have used an image of a partition, the user could have
    // made an image of the entire disk. In such case the disk label & MBR skew all cluster positions in the image
    // and hence this assumption not applies. For this reason this program takes an offset in bytes, not in clusters.
    // Do however warn the user in the case of an offset not aligning to cluster size
    if((offset % (ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE)) && (devicemodel != ZOOM_BROKEN)) // When doing random hacky shit, no need warn anyone
    {
        cout << "Warning: Supplied offset is not a multiplum of the cluster size used by the Zoom H4n & H6 firmware." << endl;
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
        cout << "Starting deinterleave at byte " << offset << endl;
        interleaved.seekg(offset, ios_base::beg); // Set offset relative to begin of stream
    }

    uint64_t bytecount = 0;

    for(uint32_t i = 0; !interleaved.eof(); i++)
    {
        uint32_t chunksize = 0; // In bytes

        switch(devicemodel)
        {
        case ZOOM_H4n:
        {
            // Net effect of the following is: 1,1,16,16,17,17,16,16,17,17 etc. (in clusters)
            // I suspect this was intended to be 1,1,16,16,16,16,16,16 etc. but somebody screwed up.
            // Sometimes, for reasons I cannot phantom, it even does: 1,1, wasting 14 clusters (448K) of space, then continue 16,16,17,17,16,16,17,17 etc.
            // Weird shit. It worked though, so nobody noticed, so nobody bothered fixing it. Newer devices may have it fixed.
            // (Zoom being from Japan though, I suspect it never gonna be fixed for some weird cultural reason with respect or something)
            if((i / 2) % 2 == 0)
                chunksize += ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE * ZOOM_H4n_START_SIZE;
            if((i / 2) > 0)
                chunksize += ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE * ZOOM_H4n_INTERLEAVE_SIZE;
        }
        break;

        case ZOOM_H6:
        {
            // Net effect of the following is: 26,26,2,2,2,2,2,2 etc. (in clusters)
            // Which is a lot more sane than the H4n does. So whatever weird shit that thing does is fixed in the H6.
            // That is, in the case of 4CH recordings because I not yet attempted recovering anything else of H6
            chunksize = ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE * ZOOM_H6_INTERLEAVE_SIZE;

            if((i / 2) % 2 == 0)
                chunksize = ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE * ZOOM_H6_START_SIZE;
        }
        break;

        case ZOOM_BROKEN:
        {
            // Net effect of the following is: 481680,481680,511800,511800 etc. (in bytes!)
            // I had a weird case drop into my inbox, where the only explanation I could come up with
            // is the device firmware having colossally screwed up: After 30 minutes of recording,
            // a freshly formatted 32GB card was designated to be "full" by a H4n Pro running v1.01 firmware..?
            // Which in itself is a total fail obviously, but the thing also not written any metadata, leaving 0-byte files.
            // Upon inspection, turned out the device not done the usual interleaving pattern, but 481680,481680,511800,511800 bytes..?
            // Which is a (somewhat) similar pattern to 16,16,17,17 in clusters, but it not adheres to cluster boundaries or even sector boundaries.
            // Sharp minds immediately see how this can never work, as the smallest fraction one can allocate to an (interleaved) file is a sector:
            // When making files you not need use all of the last sector, but a sector can never realistically be used for two files.
            // So if the interleaved datastream not has the interleaving boundaries on sector boundaries, something has gone very very haywire.
            // Meaning, the data of this gentleman in my inbox would have never read correctly even if the device had properly written metadata.
            // But to recover at least something, I use this before mentioned pattern, which I found using Audacity, my ears, and a spreadsheet
            if((i / 2) % 2 == 0)
                chunksize = 481680;
            else
                chunksize = 511800;
        }
        break;

        default:
            cout << "Unknown pattern enumeration" << endl;
            return 3;
        }

        char buffer[chunksize];
        memset(buffer, 0, chunksize);

        interleaved.read(buffer, chunksize);

        if(i % 2 == 0)
            deinterleaved1.write(buffer, chunksize);
        else
            deinterleaved2.write(buffer, chunksize);

        bytecount += chunksize;

        if(limit != 0 && bytecount > limit)
            break;
    }

    cout << "Deinterleaved " << dec << noshowbase << bytecount << " bytes (" << (bytecount / (ZOOM_FAT_SECTOR_SIZE * ZOOM_FAT_CLUSTER_SIZE)) << " clusters)" << endl;

    return 0;
}
