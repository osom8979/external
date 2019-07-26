#include "scy/archo/zipfile.h"
#include "scy/test.h"
// #include "scy/base.h"
#include "scy/filesystem.h"
// #include "scy/logger.h"
#include "scy/platform.h"
// #include "scy/util.h"

// #include <algorithm>
// #include <assert.h>
// #include <stdexcept>


// using namespace std;
using namespace scy;
using namespace scy::test;


// namespace scy {
// namespace archo {


//
// class Tests
// {
// public:
//     Tests() { testUnzip(); }
//
//     void testUnzip()
//     {
//     }
//
//     // void testGZStream()
//     // {
//     //
//     //     std::string path("gzstream.tgz");
//     //     std::string dest("gzstream");
//     //
//     //     assert(fs::exists("gzstream.tgz"));
//     //
//     //     // check alternate way of opening file
//     //     igzstream in2;
//     //     in2.open(path.c_str());
//     //     if (!in2.good()) {
//     //         std::cerr << "ERROR: Opening file `" << path << "' failed.\n";
//     //         assert(0);
//     //     }
//     //     in2.close();
//     //     if (!in2.good()) {
//     //         std::cerr << "ERROR: Closing file `" << path << "' failed.\n";
//     //         assert(0);
//     //     }
//     //
//     //     // now use the shorter way with the constructor to open the same file
//     //     igzstream in(path.c_str());
//     //     if (!in.good()) {
//     //         std::cerr << "ERROR: Opening file `" << path << "' failed.\n";
//     //         assert(0);
//     //     }
//     //     std::ofstream out(dest.c_str());
//     //     if (!out.good()) {
//     //         std::cerr << "ERROR: Opening file `" << dest << "' failed.\n";
//     //         assert(0);
//     //     }
//     //
//     //     char c;
//     //     while (in.get(c))
//     //         out << c;
//     //     in.close();
//     //     out.close();
//     //     if (!in.eof()) {
//     //         std::cerr << "ERROR: Reading file `" << path << "' failed.\n";
//     //         assert(0);
//     //     }
//     //     if (!out.good()) {
//     //         std::cerr << "ERROR: Writing file `" << dest << "' failed.\n";
//     //         assert(0);
//     //     }
//     // }
// };
//
//
//
// } // namespace scy


std::string testDataDir(const std::string& file)
{
    std::string dir;
    fs::addnode(dir, SCY_SOURCE_DIR);
    fs::addnode(dir, "archo");
    fs::addnode(dir, "tests");
    fs::addnode(dir, "data");
    if (!file.empty())
        fs::addnode(dir, file);
    return dir;
}


int main(int argc, char** argv)
{
    // Logger::instance().add(new ConsoleChannel("debug", Level::Trace));
    test::init();

    describe("extract zip archive", []() {
        std::string path(testDataDir("test.zip"));
        archo::ZipFile zip(path);

        std::string output(scy::getCwd());
        fs::addnode(output, "archo");
        zip.extract(output);

        fs::addnode(output, "HELLO");
        expect(fs::exists(output));
        fs::unlink(output);
    });

    test::runAll();
    return test::finalize();
}
