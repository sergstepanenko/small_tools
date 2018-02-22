#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#define MEM_FILL_VALUE 0x5A
#define MAX_ERR_COUNT 4

int main(void)
{
    char errCount = 0;
    //char *errArr[MAX_ERR_COUNT];
    //unsigned long errSizeArr[MAX_ERR_COUNT];
    
    unsigned long i, mmapSize, errSize, errBeginOffset, errEndOffset;
    char* mmapBegin = NULL;
    char* errBegin = NULL;
    char* errEnd = NULL;

    do
    {
        mmapBegin = NULL;
        errBegin = NULL;
        errEnd = NULL;

        mmapSize = 1024*1024*50; //50 MB
        //printf("Mapping %lu MB and fill it with 0x5A\n", mmapSize/1024/1024);
        mmapBegin = (char *)mmap(NULL, mmapSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(mmapBegin == MAP_FAILED)
        {
            mmapBegin = NULL;
            printf("Error! Memory not mapped!\n");
        }
        
        //printf("Filling memory content... ");
        for (i = 0; i < mmapSize; i++)
        {
            mmapBegin[i] = MEM_FILL_VALUE;
        }
        //printf("Finished\n");
                
        //printf("Testing memory content\n");
        errSize = 0;
        for (i = 0; i < mmapSize; i++)
        {
            if (mmapBegin[i] != MEM_FILL_VALUE)
            {
                errSize++;
                if (errBegin == NULL)
                {
                    errBeginOffset = i;
                    errBegin = mmapBegin + errBeginOffset;
                    //printf("Error! Mapped memory corrupted from 0x%p [%lu] (has 0x%X)\n", errBegin, errBeginOffset, mmapBegin[i]);
                }
            }
            else
            {
                if (errBegin != NULL)
                {
                    //if (errSize != 0)
                    //    printf("Error count = %lu\n", errSize);
                    
                    errEndOffset = i;
                    errEnd = mmapBegin + errEndOffset;
                    //printf("Error! Mapped memory corrupted until 0x%p [%lu]\n", errEnd, errEndOffset);
                    break;
                }
            }
        }
        if (errBegin != NULL)
        {
            //printf("Unmap: %p-%p (%lu bytes) and %p-%p (%lu bytes)\n", mmapBegin, errBegin - 1, errBeginOffset, errEnd, mmapBegin + mmapSize - 1, mmapSize - errEndOffset);
            
            munmap(mmapBegin, errBeginOffset);
            munmap(errEnd, mmapSize - errEndOffset);
            
            //errSizeArr[errCount] = errSize;
            //errArr[errCount] = errBegin;
            errCount++;
            
            printf("\033[1m\033[31m *** Error #%d: Memory corruption at %p, corrupted size is %lu bytes *** \033[0m\n", errCount, errBegin, errSize);
            
            mmapBegin = NULL;
        }
    }
    while(errBegin);
            
    if(mmapBegin)
    {
        //printf("Unmap test memory. End of all tests.\n");
        munmap(mmapBegin, mmapSize);
        mmapSize = 0;
        mmapBegin = NULL;
    }
    
    if(errCount > 0)
        printf("\033[1m\033[37m *** Snow White has eaten her poisoned apple and is falling asleep now. *** \033[0m\n");
    
    while(errCount > 0)
        sleep(0xffffffff);   
    
    printf("\033[1m\033[32m *** Everything is Ok! Exit *** \033[0m\n");

    return 0;
}
