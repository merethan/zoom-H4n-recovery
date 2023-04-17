# zoom-H4n-recovery
Howto guide & tool for recovering a 0-byte .wav file(s) recording from a Zoom H4n handheld recorder corrupted SD-card.

## A guide to recovering a recording for a Zoom H4n handheld recorder
.. and very likely also for other Zoom models of similar era & technology

My Zoom H4n (firmware v1.90) had its power cord pulled while recording internal mics & line inputs (4 channels total), which netted me two .wav files 0 bytes in size. This kind of unfortunate event happens to a lot of people around the globe, hence I decided to write up my endevour in recovering my recording and publish the program I wrote for restoring my data.

The bad news: I can say with certainty that no file recovery program on the internet is going to help you in the event of power loss (net adapter pulled, batteries dead), pulling the card too soon, or any other reason for not having closed down the recording properly (which includes a faulty SD card). Don't spend your time on it, don't risk installing spyware and what not: Recovery is a manual process, of which I have not (yet?) dreamed up an automated "next next finish"-process for. No exceptions, don't waste your time installing spyware that claims to magically recover anything for you.

The good news: The reason I can say the former with certainty is because I spend a good amount of time figuring out how the device works, for the purposes of recovering my own recording and doing a few recoveries for strangers as well.

### Summary (TL;DR):
You probably read this before, but to repeat it once again: If you experience a failure of any kind to gracefully stop the recording, your data is still there, somewhere, on the SD card. DO NOT START ANOTHER RECORDING AFTER YOU HAVE EXPERIENCED YOUR FAILURE! If you're in the situation where your file or files show 0 bytes in size when you put the card in your PC, data is still there, somewhere, provided it was not overwritten by you starting another recording. If you get unfortunate, swap out the SD card for another, go figure out the recovery later. Or make what's called a disk image of the SD card and get into the disk image later. (More on what a disk image is and why further below.)

For the low price of me getting to keep your SD card and you sending two special beers from your country along, I'll do it all for you. Alternatively I can also look into your disk image. Contact me on chef@edgewanderer.com

### TL;DR technical details for the leet geeks:
Zoom H4n firmware pretty much dumps raw data direct from the ADC's to disk, sequentially in one lengthy strip. In what pattern depends on SD card size, number of channels recorded, sample rate and sample resolution: For details on those you need keep reading beyond this TL;DR. Until a proper stop is done, no pointers in the File Allocation Table are made, no RIFF/Wav header prefixed to the data stream; all written on hitting the stop-button. An error of some kind before a proper stop hence leaves you with orphaned data not assigned to the file entry that was made on hitting the record button; hence it shows 0 bytes in length after a failure. No potentially spyware-loaden magic tool on the internet is ever going to help you, as the recording is essentially random data on disk. To find the start of the stream, employ a highly flexible neural network called your brain by loading the entire image into for example Audacity by using the load raw data function. Find the start of the audio you're after, recalculate the start point from seconds or samples into a byte offset in the disk image. All recordings start on a cluster boundary, so pick the nearest start of a cluster when you calculated your byte offset. Extract the data from your now precise on the cluster starting offset to another file (use a hex editor, dd, sleuth kit whatever), and if needed (when more than 2 channels were being recorded) demultiplex that with the C++ program in this repo. You now have your recording back in the form of what essentially are .wav file(s) without a RIFF header (sometimes referred to as raw Wav).

### The story:
There's a very bad design choice in the firmware of the Zoom H4n (other models are probably very similar) in that it writes all metadata the moment you hit stop. There's nothing but raw ADC output written to the SD card, until you hit stop. No magic recovery program on the internet can help you, as there's nothing an automated process can go look for. It will find old recordings you deleted before, but not the one you just lost to your net adapter being pulled, batteries going dead, having pulling the card too soon or the SD card having gone (partially) faulty.

By saying metadata, I refer to any sort of location information in the File Allocation Table (FAT) system or any sort of RIFF/Wav header. There's none. Zero. All there is, is a filename in the index, with no data assigned to it, hence it being 0 bytes in size when viewed on your PC. All the metadata stuff is written when you hit stop, or when the file limit size is reached and a new file is created. Your recording is somewhere on the SD card, but no pointers to it, no header in front of it, no footer. Nothing. It is at this point nothing but pseudo-random data.

// TODO: Section on how recovery programs sometimes recover much older files which by coincidence overlap with data written during the lost recording.

Which means it is very hard to write a computer program to recognize where your data is. Writing such a program would involve it being able to differentiate a voice or music instruments from noise and clicks: All this raw ADC output is essentially pseudo-random data in an ocean of other pseudo-random data. Luckily, locating the data is possible by means of utilizing a super flexible & programmable neural network: The key tool employed in this recovery is you yourself, your brain.

First step in any data recovery process is to make what's called a disk image of the SD-card. Tools for this are Mac's Disk Utility that comes with MacOS, every Linux-nerd of course knows the 'dd' tool, for Windows users there's for example Clonezilla and dd for Windows. The goal here is to make a 1:1 byte-by-byte, copy of the contents of the disk to one single file. Such a copy can be made for the entire disk, or just the first (and only) partition on the disk. I highly recommend making the image not of the entire disk, but only the first partition, as it makes calculation & recovery much, MUCH easier down the line.

How to do it exactly is beyond the scope of this document. Use 'dd' on Linux, Disk Utility on your Mac, or something like Clonezilla on your Windows machine. Read up on how to do it online or ask a savvy friend. It not really matter how, so long you end up with a disk image of the first (and only) partition.
* Of course an image of the entire disk also works, but that means you'll have to factor in the offset of the filesystem in your image during every calculation & operation coming up later, as the filesystem not starts at byte zero. Cutting off the leading part holding the disk label later such the filesystem is at byte 0 again is also an option of course.

If your image does include the disk label & MBR and all that jazz as copied from the SD card,
the offset of the partition from the start of the disk (meaning when it is prefixed with a partition label) is 4194kB, being byte 4194304 when FAT32 is used.
On smaller disks (2GB and under) the H4n firmware uses FAT16 to maximise effective use of the storage medium, with an offset of 127kB, being byte 127488 into the disk.

Now we have an image of the entire partition, we can go look for our lost recording. Turns out it is very hard to write a computer program for reliably doing this, yet your brain can do with relative ease. A polular line editor for working on audio is Audacity. There's plenty file formats it can load & view, the most important feature for us now is its ability to load raw data. In the File menu, go to import, raw data and select your image file holding the entire first partition of the SD-card.

For importing raw data we have to set how Audacity is to interpret the raw data in our file. Once you have selected the file a window pops up:

Encoding is either Signed 16-bit PCM or Signed 24-bit PCM depending on what resolution you had selected on the H4n device during the recording that failed.
Byte order is always Little-endian when having used the Zoom H4n.
Channels is always 2 (stereo), also when having recorded in 4CH mode! This is confusing, I know, but explainable: The Zoom H4n writes two stereo .wav files in 4CH mode. Hence select stereo.
Start offset is to be left at 0 (zero).
Amount to import is to be left at 100%.
Samplerate is either 44100Hz. or 48000Hz. depending on what sample rate you had selected on the H4n device during the recording that failed.

Now hit import. Loading the disk (partition) image into Audacity is going to take some time, depending on the size of the image (which is depending on how big the original SD-card was).

Once it is loaded, it is time to put your highly flexible & programmable neural network to use and find where the start of your recording is. There will be a lot of noise, a lot of other recordings (or pieces/remnants thereof) you already have deleted in the past, recordings that were properly stopped.. And somewhere, your recording that ended up appearing 0 bytes in size is in there. Provided you not started another recording after the unfortunate moment, it should be there in its entirety, most likely in a single stretch. As you listen to it, you will notice it cuts back and forth between the device's internal mics and external inputs, in the case of having done a 4CH recording.

If you did a 2CH recording and you can hear your audio just fine, you're done! Use Audacity to select the audio you want, paste it into another window and save it in the format you please. If you did a 4CH recording (the most likely scenario as that's what you buy a H4n for), there's more work to do which will involve (require) a fair bit more of tech-savvyness.

If you are trying to recover a 4CH recording here, you noticed the audio cuts back and forth between H4n's mics and line-in every 3 seconds or so. To undo this, we need to demuliplex the stream of data into the two .wav files you are familiar with.

Wait, what's demultiplexing?

Demultiplexing is the opposite of multiplexing: When the H4n firmware writes two files, they actually are written to disk much like the two halves of a zipper on your clothes or bag become one. There's two distinct files from the perception of the user (such they are shown when plugging the SD-card into your PC) but actually on the SD-card there's a part of the stereo .wav for internal mics written, then a part for the stereo .wav for the line in, then another part of the .wav file for internal mics etc. So to you, the user, it appears as two distinct files, but on disk they are interleaved, multiplexed, mixed; call it what you will. When loading the disk (partition) image raw into Audacity, this shows as the audio cutting back and forth between internal mics and the line-in signals.

The H4n firmware writes chunks of each file in a continuous strip while recording, almost like the SD-card is a tape-deck. Once you hit stop, it writes to the File Allocation Table (FAT) what chunk belongs to the 1st .wav file, what chunks to the 2nd .wav file. To you they appear as two files, but they are one single strip of data on the SD-card and thus in the image file.

The H4n has two formatting schemes, depending on what size SD-card you feed it. Anything over 2GB is formatted as FAT32. Anything up and including 2GB is formatted as FAT16.
The developers of the firmware did this to maximize the effective use of available storage space, as FAT16 takes less storage space overhead than FAT32 does. As such, FAT16 leaves more room for storing user data (the recording).

Why 2GB as turning point for FAT16 to FAT32? The Zoom firmware insists on a cluster size of 32768 bytes, which is 64 sectors of 512 bytes. All of its writing routines and patterns are based on this cluster size. When you multiply 32KB (cluster size) with 2 (binary) to the power of 16 (the number of bits used for addressing clusters in FAT16) you get 2GB as upper limit. If you want to address more storage space, you need more than 16 bits for addressing them.

How to undo this interleaving by means of demultiplexing?

Luckily, the H4n firmware does this multiplexing in a very predictable pattern, meaning we can reverse it. But before we go into this pattern, a few details on how the FAT32 filesystem works: The Zoom H4n wants SD-cards formatted with a FAT16 or FAT32 filesystem and it very much likes to do the formatting itself. Reason for this, is that FAT16/32 can both be used (formatted) with multiple parameters & settings. But the H4n firmware wants one specific set of them due to how it writes data to SD card. Also it wants a very specific set of directory names to be there before commencing recording, which it also makes during formatting of the SD-card.

The main parameters it cares about are sector size and cluster size: Any storage medium, which includes hard disks and SD-cards, is essentially like a tape deck in that it has a start and an end, with places in between. All any storage media can do is save & read zeroes and ones: Bits. We call a group of 8 bits a byte. A byte is the most elementary unit of addressable data in a computer. All access to data in a computer goes in one or more bytes.

What filesystems like FAT32 do is providing a mechanism to manage this enormous strip of bytes, into what we now know as files and directories. A filesystem functions like an index: For example bytes 956 to byte 3065 are referred to as /groceries/shoppinglist.txt, and bytes 5364 to 5631 is addressible as /school/todo.txt

In FAT16/FAT32, those bytes are not referred to individually, but by what Compaq, Digital Research, IBM, Microsoft and Novell have named sectors and clusters. How big they are is configurable when you format a drive/card/thumbstick with your PC. The Zoom H4n wants to format SD-cards itself because it wants a sector to be precisely 512 bytes and a cluster to be precisely 64 sectors (which makes a cluster 32768 bytes in size).

When the Zoom H4n is recording 4 channels it writes two stereo .wav files interleaved. It writes 16 clusters of data for the first .wav file, 16 clusters of data for the second .wav file, 17 clusters of data for the first file, 17 clusters of data for the second file; then the cycle start over. This is the cutting back and forth you hear when loading the image into Audacity as raw data.

Using Audacity we have found the starting point of our lost recording in the image file. To demultiplex it, we need to cut our image file into two at exactly the start of the recording. By doing the cut & discarding all data that was before the start of our lost recording, the start of our recording is now exactly byte 0. Our new file with the start of our lost recording at byte 0 can now be fed into the demultiplexing program that's in this repo. After demultiplexing, you got yourself essentially two .wav files again.

Unfortunately, we cannot do the cutting with Audacity, as Audacity does all sorts of conversions, prepends headers to the files it saves etc. so all it is to be used for is finding where the start of our lost recording is in the disk image file. Once we found that, we need calculate the start of the recording into a cluster offset, which we will use in another program to cut the file into two (another tool/program which does operations on raw data, no conversions like Audacity does).

Once very handy feature of Audacity for this purpose is its ability to not only show the position of the cursor in hours/minutes/seconds, it can also be set to show the position of the cursor in samples. If this toolbar is not visible on screen, enable it in the menu View > Toolbars > Time Toolbar. Once you know the sample number, it is trivial to calculate that into a byte-offset in the disk (partition) image.

The size of one sample depends on the chosen resolution: A byte is 8 bits, so 16-bit resolution is 2 bytes, 24-bit resolution is 3 bytes. What Audacity refers to as a sample is actually two samples, as our files are stereo. So, in 16-bit mode one sample in Audacity terms is 4 bytes, in 24-bit mode one sample is 6 bytes. So your sample number times 4 or times 6 depending on what sample size you picked during the raw import is the byte offset in the image.

Now putting your cursor precisely on the start of the lost recording in Audacity is a bit tricky. Due to the H4n firmware always starting in a new sector however, we know for sure the offset byte in the image always must be a multiplum of the cluster size: 32768 bytes. With some very basic mathematics we can get ourselves to the exact correct number:

As an example, say you found your recording in the disk image using Audacity, at sample 15926142. Assuming you used a 16-bit resolution for the recording (and thus for importing the raw data into Audacity) each sample will be 4 bytes. So the byte offset for your cursor is sample 15926142 * samplesize 4 = byte 63704568. Now if we divide that by the clustersize, 63704568 / 32768 = ~1944.1091, which can't be because recordings always start precisely at the beginning of a cluster. The number ~1944.1091 is however awfully close to 1944, meaning your recording most likely starts at cluster 1944 and the extra .1091 is your click being slightly off. Cluster 1944 * clustersize 32768 = byte 63700992.

(Another way would be to use a time mark in (milli)seconds, and recalculate that into a byte-offset using both the resolution and samplerate. But I leave figuring that out as an excersise to the reader.)

So, assuming you did your homework, you now have the cluster your recording starts at. How to cut the file?

There's multiple ways to do this of course, so if you are already familiar with this kind of business, use the tool which suits you best: A hex-editor, some binary cut'n'paste tool whatever. As an example I'll propose to you another use of 'dd', which is the same tool you most likely also have used to make the disk image in the first place, if you are on a Linux machine.
This step is actually very simple: dd if=[your file] of=[your file but cut] bs=32768 skip=1944
What the above says is make a carbon copy of the input file, interpret this file as block of 32768 bytes, skip the first 1944 which means block index 1944 is now block 0 in the new file.

* Beware that if your disk image is that of the whole disk and not only the first partition, then you have to subtract the partition offset from your from samples found byte offset, do the cluster calculation, convert that to bytes again and add the prior subtracted offset. And when you got your number in bytes, not specify a block size to dd but use the byte offset for skipping only. These extra steps makes it more likely you screw up the numbers. -> This is why I adviced you to work with an image of the partition, not the whole disk.

// TODO: Modify zoomdeinterleave.cpp such it also takes an offset, which makes the copy operation of data from the disk image file to a new (but cut) disk image file using 'dd' unneeded.

You now have the exact start of the recording at byte 0 of your image file. It is however still cutting back and forth between the internal mics and external inputs of the Zoom H4n audio recorder if you load it as raw data into Adacity again. This is where the zoomdeinterleave tool comes in. All it takes to untangle this (assuming you got all your finding & calculating of the start cluster right): ./zoomdeinterleave mics.wav line.wav your_cut_disk_image.img

And, if you did your homework right: There it is. Your precious lost recording recovered!
