#pragma once

class UI
{
public:
    int ReadMode();
    void ReadFilePaths(char inputFilePath[], char outputFilePath[], int maxSize);
    void PrintSilentStart();
    void PrintSilentEnd();

private:
    void CleanFilePath(char path[]);
};
