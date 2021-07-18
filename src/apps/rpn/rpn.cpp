#include <cstring>
#include <cstdlib>


// // utilities

// int strcmp(const char *a, const char *b) {
//     int i = 0;
//     int diff = 0;

//     for (; a[i] != 0 && b[i] != 0; ++i) {
//         diff = a[i] - b[i];
//         if (diff) {
//             break;
//         }
//     }

//     return diff;
// }

// // ascii to long
// long atol(const char *s) {
//     long v = 0;
//     for (int i = 0; s[i] != 0; ++i) {
//         v = v*10 + (s[i]-'0');
//     }

//     return v;
// }


// stack

int stack_ptr;
long stack[100];

long Pop() {
    return stack[stack_ptr--];
}

void Push(long value) {
    stack[++stack_ptr] = value;
}


// entrypoint

extern "C"
int main(int argc, char **argv) {
    stack_ptr = -1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "+") == 0) {
            long b = Pop();
            long a = Pop();
            Push(a + b);
        } else if (strcmp(argv[i], "-") == 0) {
            long b = Pop();
            long a = Pop();
            Push(a - b);
        } else {
            long a = atol(argv[i]);
            Push(a);
        }
    }

    if (stack_ptr < 0) {
        return 0;
    }

    return static_cast<int>(Pop());
}
