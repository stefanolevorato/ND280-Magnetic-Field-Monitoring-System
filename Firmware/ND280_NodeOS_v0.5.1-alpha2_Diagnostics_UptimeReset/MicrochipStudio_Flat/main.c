#include "node_app.h"

int main(void)
{
    node_app_init();

    while (1) {
        node_app_process();
    }
}
