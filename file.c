#include <stdio.h>
#include <stdint.h>

#define KEY_LEFT_CTRL     "\x80"
#define KEY_LEFT_SHIFT    "\x81"
#define KEY_LEFT_ALT      "\x82"
#define KEY_LEFT_GUI      "\x83"
#define KEY_RIGHT_CTRL    "\x84"
#define KEY_RIGHT_SHIFT   "\x85"
#define KEY_RIGHT_ALT     "\x86"
#define KEY_RIGHT_GUI     "\x87"

#define KEY_RETURN        "\xB0"
#define KEY_ESC           "\xB1"
#define KEY_BACKSPACE     "\xB2"
#define KEY_TAB           "\xB3"

#define KEY_CAPS_LOCK     "\xC1"

#define KEY_F1            "\xC2"
#define KEY_F2            "\xC3"
#define KEY_F3            "\xC4"
#define KEY_F4            "\xC5"
#define KEY_F5            "\xC6"
#define KEY_F6            "\xC7"
#define KEY_F7            "\xC8"
#define KEY_F8            "\xC9"
#define KEY_F9            "\xCA"
#define KEY_F10           "\xCB"
#define KEY_F11           "\xCC"
#define KEY_F12           "\xCD"
#define KEY_PRTSC         "\xCE"

#define KEY_INSERT        "\xD1"
#define KEY_HOME          "\xD2"
#define KEY_PAGE_UP       "\xD3"
#define KEY_DELETE        "\xD4"
#define KEY_END           "\xD5"
#define KEY_PAGE_DOWN     "\xD6"
#define KEY_ARROW_RIGHT   "\xD7"
#define KEY_ARROW_LEFT    "\xD8"
#define KEY_ARROW_DOWN    "\xD9"
#define KEY_ARROW_UP      "\xDA"

#define KEY_NUM_LOCK      "\xDB"
#define KEY_NUM_SLASH     "\xDC"
#define KEY_NUM_ASTERISK  "\xDD"
#define KEY_NUM_MINUS     "\xDE"
#define KEY_NUM_PLUS      "\xDF"
#define KEY_NUM_ENTER     "\xE0"
#define KEY_NUM_1         "\xE1"
#define KEY_NUM_2         "\xE2"
#define KEY_NUM_3         "\xE3"
#define KEY_NUM_4         "\xE4"
#define KEY_NUM_5         "\xE5"
#define KEY_NUM_6         "\xE6"
#define KEY_NUM_7         "\xE7"
#define KEY_NUM_8         "\xE8"
#define KEY_NUM_9         "\xE9"
#define KEY_NUM_0         "\xEA"
#define KEY_NUM_PERIOD    "\xEB"

#define KEY_F13           "\xF0"
#define KEY_F14           "\xF1"
#define KEY_F15           "\xF2"
#define KEY_F16           "\xF3"
#define KEY_F17           "\xF4"
#define KEY_F18           "\xF5"
#define KEY_F19           "\xF6"
#define KEY_F20           "\xF7"
#define KEY_F21           "\xF8"
#define KEY_F22           "\xF9"
#define KEY_F23           "\xFA"
#define KEY_F24           "\xFB"

int main() {
    FILE* file = fopen("file1", "wb");
    const uint8_t data[] = 
        KEY_F5 KEY_F6 KEY_F7 KEY_F8 KEY_F9 KEY_F10 KEY_F12
        "helloladies worshipme naturaltalent cvwkxam kvgyzqk vkypqcf hesoyam professionalkiller aezakmi fullclip pleasantlywarm everyoneisrich speedfreak ripazha bubblecars flyingfish" KEY_ESC;
    for (int i = 0; data[i] != 0; i++) {
        fputc(data[i], file);
    }
    fclose(file);
}