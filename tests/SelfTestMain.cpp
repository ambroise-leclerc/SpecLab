/**
 * @brief Entry point of SpecLab's self-tests: every scenario is registered by the other files of
 * this directory through speclab::Register, and run through speclab::runMain.
 */
import std;
import speclab;

int main(int argc, char** argv) {
    return speclab::runMain(argc, argv, "SpecLab self-tests");
}
