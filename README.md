# CS 377 Final Project Report
## Project Idea: Packr - A Custom, Multithreaded File Archiver For File Packaging & Compression
### Introduction
This project was made by Shreyas Donti and Stella Dey for CS 377. Our project can be broken down into two applications:
1. A folder compression library similar to a tool like 7-Zip or WinRAR, but one that implements encryption/decryption, multithreading for large directories, and exposes a C++ API for usage in other software programs.
2. A simple CLI tool for using the library directly to compress/decompress files and folders.
This project demonstrates the following concepts from CS 377:
1. Concurrency/multithreading
2. Understanding of filesystems
3. Serialization/deserialization
4. Working with locks/semaphores
5. Producer and consumer threads
### Motivation
File & folder compression tools are incredibly useful for packaging large files and directories to prepare them for transfer between computers via downloads/uploads/sends. By reducing the size of a file or folder of files, you can send data faster between computers, use less bandwidth, and send data under storage limits. However, file packaging like this has other incredibly useful use cases.

(Shreyas) In high school during Covid, I had installed a bunch of games since I had nothing better to do with my time, and since I was also becoming interested in game development around that time, I would try and poke around in the files of the video games I had installed. Now while some games had their assets obscured or impossible to find, or others had them out in the open, I came across a game with
### Methodology
### Instructions
You can run the program named `exec` inside the `build/` folder. This program has been tested in the Edlab machine, and if you need to rebuild it, we have generated Makefiles using Premake 5 so you can simply call the `make` command to rebuild it.
### Results
### Reflection
