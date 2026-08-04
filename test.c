#include "includes/codexion.h"

int main()
{
    t_heap hp;
    t_request a;
    t_request b;

    heap_init(&hp, 2, CODEX_EDF);
    a.coder_id = 1;
    a.deadline = 900;
    a.sequence = 10;
    b.coder_id = 2;
    b.deadline = 500;
    b.sequence = 20;
    heap_push(&hp, &a);
    heap_push(&hp, &b);
    printf("%d", heap_peek(&hp)->coder_id);
}