#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/time.h>

#include <signal.h>

volatile sig_atomic_t stop = 0;
void handle_sigint(int sig) {
    if (stop) {
        fprintf(stderr, "Exiting\n");
        exit(1);
    }
    fprintf(stderr, "Interrupted\n");
    stop = 1;
}

void emit_event(int fd, unsigned short type, unsigned short code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

void emit_sync(int fd) {
    emit_event(fd, EV_SYN, SYN_REPORT, 0);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s /dev/input/eventX\n", argv[0]);
        return 1;
    }
    const char *device = argv[1];
    struct input_event ev;
    int uinput_fd;

    signal(SIGINT, handle_sigint);

    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open touchpad device");
        return 2;
    }

    {
        struct uinput_setup usetup;
        struct uinput_abs_setup abs_setup;

        uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (uinput_fd < 0) {
            perror("Failed to open /dev/uinput");
            close(fd);
            return 2;
        }

        struct pair { unsigned int x, y; } abs_inputs[] = {
            {ABS_X, EVIOCGABS(ABS_X)},
            {ABS_Y, EVIOCGABS(ABS_Y)},
            {ABS_MT_SLOT, EVIOCGABS(ABS_MT_SLOT)},
            {ABS_MT_POSITION_X, EVIOCGABS(ABS_MT_POSITION_X)},
            {ABS_MT_POSITION_Y, EVIOCGABS(ABS_MT_POSITION_Y)},
            {ABS_MT_TOOL_TYPE, EVIOCGABS(ABS_MT_TOOL_TYPE)},
            {ABS_MT_TRACKING_ID, EVIOCGABS(ABS_MT_TRACKING_ID)}
        };
        ioctl(uinput_fd, UI_SET_EVBIT, EV_ABS);
        ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
        for (int i=0;i<sizeof(abs_inputs)/sizeof(*abs_inputs);i++) {
            ioctl(uinput_fd, UI_SET_ABSBIT, abs_inputs[i].x);
        
            abs_setup.code = abs_inputs[i].x;
            ioctl(fd, abs_inputs[i].y, &abs_setup.absinfo);
            ioctl(uinput_fd, UI_ABS_SETUP, &abs_setup);
        }
        ioctl(uinput_fd, UI_SET_KEYBIT, BTN_TOUCH);
        ioctl(uinput_fd, UI_SET_KEYBIT, BTN_TOOL_FINGER);
        ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);

        ioctl(uinput_fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
        // ioctl(uinput_fd, UI_SET_PROPBIT, INPUT_PROP_POINTER);

        memset(&usetup, 0, sizeof(usetup));
        snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "Virtual Touchscreen (%s)", device);
        usetup.id.bustype = BUS_USB;
        usetup.id.vendor = 0x1234;
        usetup.id.product = 0x5678;
        usetup.id.version = 1;

        ioctl(uinput_fd, UI_DEV_SETUP, &usetup);
        ioctl(uinput_fd, UI_DEV_CREATE);
    }

    if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        perror("Failed to grab device");
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
        close(fd);
        return 2;
    }

    printf("Listening to events from %s ... (interrupt to exit)\n", device);

    while (!stop) {
        ssize_t bytes = read(fd, &ev, sizeof(ev));
        if (bytes < (ssize_t) sizeof(ev)) {
            perror("Failed to read event");
            break;
        }

        if (ev.type == EV_ABS) {
            switch (ev.code) {
                case ABS_X:
                case ABS_Y:
                case ABS_MT_SLOT:
                case ABS_MT_POSITION_X:
                case ABS_MT_POSITION_Y:
                case ABS_MT_TOOL_TYPE:
                case ABS_MT_TRACKING_ID:
                    write(uinput_fd, &ev, sizeof(ev));
                    break;
                default:
                    break;
            }
        } else if (ev.type == EV_KEY) {
            switch (ev.code) {
                case BTN_TOUCH:
                case BTN_TOOL_FINGER:
                case BTN_LEFT:
                    write(uinput_fd, &ev, sizeof(ev));
                    break;
                default:
                    break;
            }
        } else if (ev.type == EV_SYN) {
            write(uinput_fd, &ev, sizeof(ev));
        }
    }

    ioctl(fd, EVIOCGRAB, 0);
    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
    close(fd);

    return 0;
}
