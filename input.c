static void input();

void input() {
    int d, i;
    struct termios old, new;

    tcgetattr(0, &old);
    new = old;
    new.c_lflag &= ~ICANON;
    new.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &new);

    while (running) {
        d = getchar(); 

        for (i = 0; keys[i].key; i++) {
            if (keys[i].key == d)
                keys[i].function();
        }
    }

    tcsetattr(0, TCSANOW, &old);
}
