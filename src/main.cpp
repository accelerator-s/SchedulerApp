#include "SchedulerApp.h"

int main(int argc, char* argv[]) {
    auto app = SchedulerApp::create();
    return app->run(argc, argv);
}