#  CS 377 Final Project Report

## Project Idea: Packr - A Lightweight Program For File Archiving and Single & Multi-Threaded Compression

###  Introduction

This project was made by **Shreyas Donti** and **Stella Dey** for CS 377. Our project essentially provides a lightweight C++ program that can archive directories, placing the contents of a directory in a single file, and can employ single-threaded and multi-threaded compression to reduce archive sizes.

This project demonstrates the following concepts from CS 377:

1. Concurrency/multithreading
2. Understanding of filesystems
3. Serialization/deserialization
4. Working with locks/semaphores
5. Producer and consumer threads

###  Motivation

File & folder compression tools are incredibly useful for packaging large files and directories to prepare them for transfer between computers via downloads/uploads/sends. By packaging the contents of a directory in a single file, and compressing their contents, you can quickly and easily send files between computers. File compression systems are used all the time with tools such as Git, creating JAR files in Java, and in video games where game assets can be packaged in a singular file and loaded at once to avoid performance spikes during a game caused by lazy loading of assets.

(Shreyas) I personally became interested in file packaging & compression when I began searching through the files of video games I played in high school, in order to mess with the files and mod the game. By going through the files of several video games, I came across a lot of video games that would package their assets (audio, images, shaders) in a single file for obfuscation and so they can be loaded at bootup instead of being loaded lazily.

From this experience, we decided to try and implement our own file archiver program named Packr, that could automatically package a directory into a single file for re-extraction later, and we also investigated how effective our compression algorithm was at reducing file sizes during packaging, and how effective a multi-threaded approach to compression/decompression could be for ensuring faster compression/decompression times.

###  Methodology
In this project, our goal is to implement five main algorithms:
-  `archive()`: Package a directory into a single .packr file (no compression)
- `unarchive()`: Extract all files and folders from a .packr file (no compression)
- `compress()`: Compress & package directory into a single .packr file
- `decompress()`: Decompress & extract all files and folders from a .packr file
- `parallel_compress()`: Like `compress` but using multi-threading for speed.
- `parallel_decompress()`: Like `decompress` but using multi-threading for speed.

We can divide these algorithms into four categories: single threaded packaging, multi-threaded packaging, single threaded unpackaging, multi-threaded unpackaging.

#### Single Threaded Packaging
1. Get a list of file paths in the directory to compress.
2. Create an instance of the `Packr` class. This class object is used to manage each file's data, and store headers for each file chunk and for the .packr file.
3. Read each file's contents and store to a chunk.
4. Add a header to each chunk that stores metadata for loading it.
5. (COMPRESS) Compress the chunk using the open source DEFLATE algorithm.
6. Add the chunk to the `Packr` object.
7. Flush the `Packr` object's contents to disk.

#### Multi-Threaded Packaging
1. Get a list of file paths in the directory to compress, and store in a queue.
2. Create an instance of the `Packr` class. This class object is used to manage each file's data, and store headers for each file chunk and for the .packr file.
3. Create a worker function that executes on the following steps:
- (CRITICAL) Pop file path from the queue (lock when accessing)
- Load the file data to memory
- Create a chunk and add its header
- Compress the file data
- (CRITICAL) Push the chunk back into the `Packr` object.
- Loop until there are no more files to compress.
4. Create worker threads to execute the worker function we set up.
5. Join each thread.
6. Add the chunk to the `Packr` object.
7. Flush the `Packr` object's contents to disk.

#### Single Threaded Unpackaging
1.

###  Results
###  Instructions
You can run the program named `exec` inside the `build/` folder. If it doesn't work, you can call:
- `make clean`
- `make config=debug`

This should work for MacOS/Linux. If running on a different OS, you can install Premake 5 and use it to generate builds for different targets, though note we haven't tested this project on Windows so there might be issues because of the different file system.

We tried building this on the Edlab machine, but for some reason we got an error saying the `<string>` header wasn't installed, so we weren't able to build it there unfortunately.

###  Reflection
From this project, we learnt how compression tools like 7zip and WinRar work, and we learnt to apply multithreading for practical purpose and witnessed firsthand the benefits of a multithreaded approach.