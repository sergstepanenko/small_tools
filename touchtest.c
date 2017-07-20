#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>

#define TOUCHFILE0 "/dev/input/event0\0"
#define TOUCHFILE1 "/dev/input/event1\0"

#define TOUCHSCREENS_COUNT 2
//
int main()
{
    struct input_event ie;
    int slot;

    int fds[TOUCHSCREENS_COUNT], maxfd, j, result;
    fd_set readset;

    if((fds[0] = open(TOUCHFILE0, O_RDONLY)) == -1)
    {
        printf("Blocking %s open ERROR\n",TOUCHFILE0);
        exit(EXIT_FAILURE);
    }
    else
        printf("Blocking %s open OK\n",TOUCHFILE0);

    if((fds[1] = open(TOUCHFILE1, O_RDONLY)) == -1)
    {
        printf("Blocking %s open ERROR\n",TOUCHFILE1);
        exit(EXIT_FAILURE);
    }
    else
        printf("Blocking %s open OK\n",TOUCHFILE1);

            // Initialize the set
    FD_ZERO(&readset);
    maxfd = 0;
    for (j=0; j<TOUCHSCREENS_COUNT; j++)
    {
        FD_SET(fds[j], &readset);
        maxfd = (maxfd>fds[j])?maxfd:fds[j];
    }


    printf("maxfd=%d\n", maxfd);

    while(1)
    {



        // Now, check for readability
        result = select(maxfd+1, &readset, NULL, NULL, NULL);
        if (result == -1)
        {
            printf("Select error...\n");
        }
        else
        {
            for (j=0; j<TOUCHSCREENS_COUNT; j++)
            {
                if (FD_ISSET(fds[j], &readset))
                {
                    // fds[j] is readable
                    if(read(fds[j], &ie, sizeof(struct input_event))!=-1)
                    {
                        switch (ie.type)
                        {
                            case EV_SYN:
                                //printf("Touchscreen %d: EV_SYN\n", j);
                                break;
                            case EV_KEY:
                                switch (ie.code)
                                {
                                    case BTN_TOUCH:
                                        printf("Touchscreen %d: EV_KEY BTN_TOUCH Value=%X\n", j, ie.value);
                                        break;
                                    default:
                                        printf("Touchscreen %d: EV_KEY ! Uknown Code=%X Value=%X\n", j, ie.code, ie.value);
                                        break;
                                }
                                break;
                            case EV_ABS:
                                switch (ie.code)
                                {
                                    case ABS_X:
                                        //printf("Touchscreen %d: EV_ABS ABS_X Slot=%d Value=%X\n", j, slot, ie.value);
                                        break;
                                    case ABS_Y:
                                        //printf("Touchscreen %d: EV_ABS ABS_Y Slot=%d Value=%X\n", j, slot, ie.value);
                                        break;
                                    case ABS_MT_SLOT:
                                        slot=ie.value;
                                        //printf("Touchscreen %d: EV_ABS ABS_MT_SLOT Value=%X\n", j, ie.value);
                                        break;
                                    case ABS_MT_POSITION_X:
                                        printf("Touchscreen %d: EV_ABS ABS_MT_POSITION_X Slot=%d Value=%X\n", j, slot, ie.value);
                                        break;
                                    case ABS_MT_POSITION_Y:
                                        printf("Touchscreen %d: EV_ABS ABS_MT_POSITION_Y Slot=%d Value=%X\n", j, slot, ie.value);
                                        break;
                                    case ABS_MT_TRACKING_ID:
                                        printf("Touchscreen %d: EV_ABS ABS_MT_TRACKING_ID Slot=%d Value=%X\n", j, slot, ie.value);
                                        break;
                                    default:
                                        printf("Touchscreen %d: EV_ABS ! Uknown Code=%X Value=%X\n", j, ie.code, ie.value);
                                        break;
                                }
                                break;
                            default:
                                printf("! Touchscreen %d: Unknown Type=%X Code=%X Value=%X\n", j, ie.type, ie.code, ie.value);
                                break;
                        }

                    }

                    //fflush(stdout);
                }

                FD_SET(fds[j], &readset);
            }
        }
    }
    //
    for (j=0; j<TOUCHSCREENS_COUNT; j++)
        close(fds[j]);

    printf("End\n");
    return 0;
}
