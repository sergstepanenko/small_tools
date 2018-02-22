#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/mman.h>

#define MEM_FILL_VALUE 0x5A
#define PART_COUNT 40

int main(void)
{
    int command;
    char *ptr[PART_COUNT];
    char a = 0;
    unsigned long i, n, mmapSize, errCount, errBeginOffset, errEndOffset;
    char* mmapBegin = NULL;
    char* errBegin = NULL;
    char* errEnd = NULL;
    
    n = 1024*1024*50; //50 MB
    
    int count = -1;
    int j;
    
    printf("Usage:\n");
    printf("a - allocate %lu MB and fill with 0x5A\n", n/1024/1024);
    printf("w - fill all with 0x5A\n");
    printf("t - test memory content\n");
    printf("f - free %lu MB\n", n/1024/1024);

    printf("\nm - map %lu MB and fill with 0x5A\n", n/1024/1024);
    printf("j - fill mapping with 0x5A\n");
    printf("z - test mapping content\n");
    printf("x - unmap all except bad blocks\n");
    printf("u - unmap %lu MB\n", n/1024/1024);

    printf("q - exit\n");
    
    while ((command = getchar()) != EOF)
    {
        switch(command)
        {
        case 'm':
            if(!mmapBegin)
            {
                printf("Mapping %lu MB and fill it with 0x5A\n", n/1024/1024);
                mmapSize = n;
                mmapBegin = (char *)mmap(NULL, mmapSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if(mmapBegin == MAP_FAILED)
                {
                    mmapBegin = NULL;
                    printf("ERROR! Memory not mapped!\n");
                }
            }
        case 'j':
            printf("Filling memory content... ");
            for (i = 0; i < mmapSize; i++)
            {
                mmapBegin[i] = MEM_FILL_VALUE;
            }
            printf("Finished\n");
            break;
            
        case 'z':
            printf("Testing memory content\n");
            a = 0;
            errCount = 0;
            for (i = 0; i < mmapSize; i++)
            {
                if (mmapBegin[i] != MEM_FILL_VALUE)
                {
                    errCount++;
                    if (a != mmapBegin[i])
                    {
                        a = mmapBegin[i];
                        printf("ERROR! Mapped memory corrupted from [%lu] (has 0x%X)\n", i, a);
                    }
                }
                else
                {
                    if (a != mmapBegin[i])
                    {
                        if (errCount != 0)
                            printf("ERROR count = %lu\n", errCount);
                        
                        a = mmapBegin[i];
                        printf("Memory ok from [%lu] (has 0x%X)\n", i, a);
                    }
                }
            }
            break;
            
        case 'x':
            printf("Testing memory content\n");
            a = 0;
            errCount = 0;
            for (i = 0; i < mmapSize; i++)
            {
                if (mmapBegin[i] != MEM_FILL_VALUE)
                {
                    errCount++;
                    if (errBegin == NULL)
                    {
                        errBeginOffset = i;
                        errBegin = mmapBegin + errBeginOffset;
                        printf("ERROR! Mapped memory corrupted from 0x%p [%lu] (has 0x%X)\n", errBegin, errBeginOffset, a);
                    }
                }
                else
                {
                    if (errBegin != NULL)
                    {
                        if (errCount != 0)
                            printf("ERROR count = %lu\n", errCount);
                        
                        errEndOffset = i;
                        errEnd = mmapBegin + errEndOffset;
                        printf("ERROR! Mapped memory corrupted until 0x%p [%lu]\n", errEnd, errEndOffset);
                        break;
                    }
                }
            }
            if (errBegin != NULL)
            {
                printf("Unmap: %p-%p (%lu bytes) and %p-%p (%lu bytes)\n", mmapBegin, errBegin - 1, errBeginOffset, errEnd, mmapBegin + mmapSize - 1, mmapSize - errEndOffset);
                
                munmap(mmapBegin, errBeginOffset);
                munmap(errEnd, mmapSize - errEndOffset);
                mmapSize = errCount;
                mmapBegin = errBegin;
            }
            
            
            break;
            
        case 'u':
            if(mmapBegin)
            {
                printf("Unmap\n");
                munmap(mmapBegin, mmapSize);
                mmapSize = 0;
                mmapBegin = NULL;
            }
            break;

            
            
            
            
            
            
            
        case 'a':
            count++;
            printf("Allocating %lu MB [%d] and fill it with 0x5A\n", n/1024/1024, count);
            ptr[count]=(char*)malloc(n);
            if (ptr[count]==NULL)
            {
                printf("ERROR! Memory [%d] not allocated!\n", count);
                break;
            }
            for (i = 0; i < n; i++)
            {
                ptr[count][i] = MEM_FILL_VALUE;
            }
            printf("Finished\n");
            break;

        case 'w':
            printf("Filling memory content... ");
            for (j = 0; j <= count; j++)
            {
                for (i = 0; i < n; i++)
                {
                    ptr[count][i] = MEM_FILL_VALUE;
                }
            }
            printf("Finished\n");
            break;

        case 't':
            printf("Testing memory content\n");
            for (j = 0; j <= count; j++)
            {
                a = 0;
                errCount = 0;
                for (i = 0; i < n; i++)
                {
                    if (ptr[j][i] != MEM_FILL_VALUE)
                    {
                        errCount++;
                        if (a != ptr[j][i])
                        {
                            a = ptr[j][i];
                            printf("ERROR! Memory corrupted from [%d][%lu] (has 0x%X)\n", j, i, a);
                        }
                    }
                    else
                    {
                        if (a != ptr[j][i])
                        {
                            if (errCount != 0)
                                printf("ERROR count [%d] = %lu\n", j, errCount);
                            
                            a = ptr[j][i];
                            printf("Memory ok from [%d][%lu] (has 0x%X)\n", j, i, a);
                        }
                    }
                }
            }
            break;

        case 'f':
            if (count >=0)
            {
                printf("Free %lu MB [%d]\n", n/1024/1024, count);
                free(ptr[count]);
                count--;
            }
            else
                printf("All memory is free\n");
            break;
            
        case 'q':
            while (count >=0)
            {
                printf("Free %lu MB [%d]\n", n/1024/1024, count);
                free(ptr[count]);
                count--;
            }
            if(mmapBegin)
            {
                printf("Unmap\n");
                munmap(mmapBegin, mmapSize);
                mmapSize = 0;
                mmapBegin = NULL;
            }
            printf("Quit\n");
            return 0;

        case 0x0a:
            break;

        default:
            printf("Unexpected input %d (0x%.2X) ('%c')\n",
                   command, command, isgraph(command) ? command : '.');
            break;
        }
    }
    return 0;
}
