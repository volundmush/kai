#include "kai/scripting.h"

int main(int argc, char **argv) {

    script::ScriptManager manager;

    auto t = manager.createThread();

    auto code = manager.compile("print('Hello World!')");

    auto task = script::Task(t, code);

    task.load();

    task.run();

}