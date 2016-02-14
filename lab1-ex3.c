#include"kern/printf.c"
#include"lib/printfmt.c"

extern void putchar(int c);

void cputchar(int c) { 
    putchar(c);
}

int main() {
    unsigned int i = 0x00646c72;
    cprintf("H%x Wo%s", 57616, &i);
}
