#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>

#define TOUCHFILE0 "/dev/input/event0\0"
#define TOUCHFILE1 "/dev/input/event1\0"

#define TOUCHSCREENS_COUNT 2

#define TOUCHES_COUNT 10

struct TouchEvent
{
    int32_t trackingID;
    int32_t x;
    int32_t y;
    char state;
};

//
int main()
{
    struct input_event ie;
    int slot=0;

    struct TouchEvent touchEvent[TOUCHES_COUNT];

    memset(touchEvent, 0, sizeof(touchEvent));

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
                                        //printf("Touchscreen %d: EV_KEY BTN_TOUCH Value=%X\n", j, ie.value);
                                        break;
                                    default:
                                        //printf("Touchscreen %d: EV_KEY ! Uknown Code=%X Value=%X\n", j, ie.code, ie.value);
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
                                        if (touchEvent[slot].x != ie.value)
                                        {
                                            touchEvent[slot].state |= 0x10; //changed
                                            touchEvent[slot].x = ie.value;
                                        }
                                        touchEvent[slot].state |= 0x2; //has X
                                        //printf("Touchscreen %d: EV_ABS ABS_MT_POSITION_X Slot=%d Value=%X\n", j, slot, ie.value);
                                        break;
                                    case ABS_MT_POSITION_Y:
                                        if (touchEvent[slot].y != ie.value)
                                        {
                                            touchEvent[slot].state |= 0x10; //changed
                                            touchEvent[slot].y = ie.value;
                                        }
                                        touchEvent[slot].state |= 0x4; //has Y
                                        //printf("Touchscreen %d: EV_ABS ABS_MT_POSITION_Y Slot=%d Value=%X\n", j, slot, ie.value);
                                        break;
                                    case ABS_MT_TRACKING_ID:
                                        touchEvent[slot].trackingID = ie.value;
                                        touchEvent[slot].state |= 0x1; //has Tracking ID
                                        touchEvent[slot].state |= 0x10; //changed
                                        //printf("Touchscreen %d: EV_ABS ABS_MT_TRACKING_ID Slot=%d Value=%X\n", j, slot, ie.value);
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

                        switch(touchEvent[slot].state)
                        {
                            case 0x17:
                                printf("Touchscreen %d: Slot=%d Touch   at [%dx%d]\n", j, slot, touchEvent[slot].x, touchEvent[slot].y);
                                touchEvent[slot].state |= 0x8;
                                touchEvent[slot].state &= ~0x10;
                                break;
                            case 0x1F:
                                if (touchEvent[slot].trackingID == -1)
                                {
                                    printf("Touchscreen %d: Slot=%d Release at [%dx%d]\n", j, slot, touchEvent[slot].x, touchEvent[slot].y);
                                    touchEvent[slot].state = 0x0;
                                }
                                else
                                {
                                    touchEvent[slot].state &= ~0x10;
                                    printf("Touchscreen %d: Slot=%d Move    to [%dx%d]\n", j, slot, touchEvent[slot].x, touchEvent[slot].y);
                                }
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
