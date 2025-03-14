# AbsoluteTouch4L
AbsoluteTouchEx lets you use your touchpad like a touchscreen, giving you absolute cursor movement just like you would get on a tablet. It is the Linux version of [AbsoluteTouchEx](https://github.com/apsun/AbsoluteTouchEx) by [Andrew Sun](https://github.com/apsun).

## Running the project

Requirements:

- Linux
- root privileges

Ensure that your computer has touchpad.
```
sudo ./touchpad_to_touchscreen /dev/input/eventX
```
This program works by redirecting all events from touchpad and emits them to virtual touchscreen.

If you don't know touchpad device path. Try testing every input device.
```
ls /dev/input/event*
sudo evtest /dev/input/eventX
```

## Building the project

Requirements:

- c compiler (gcc)

```
gcc main.c -o touchpad_to_touchscreen
```