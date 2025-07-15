#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

struct ResourceInfo
{
    string filename;
    string varName;
    string description;
};

// 将二进制文件转换为C++数组
vector<unsigned char> readBinaryFile(const string &filename)
{
    ifstream file(filename, ios::binary);
    if (!file)
    {
        cerr << "Error: Cannot open file " << filename << endl;
        return {};
    }

    vector<unsigned char> buffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    return buffer;
}

void generateResourceFiles(const vector<ResourceInfo> &resources)
{
    // 生成头文件
    ofstream header("src/embedded_resources.h");
    header << "#ifndef EMBEDDED_RESOURCES_H\n";
    header << "#define EMBEDDED_RESOURCES_H\n\n";
    header << "#include <cstddef>\n";
    header << "#include <string>\n";
    header << "#include <memory>\n\n";
    header << "namespace EmbeddedResources {\n\n";

    for (const auto &res : resources)
    {
        header << "    // " << res.description << "\n";
        header << "    extern const unsigned char " << res.varName << "[];\n";
        header << "    extern const size_t " << res.varName << "_size;\n\n";
    }

    header << "    // Helper functions\n";
    header << "    std::string getXMLContent();\n";
    header << "    const unsigned char* getAudioData(size_t& size);\n";
    header << "    bool writeAudioToTempFile(const std::string& tempPath);\n\n";
    header << "}\n\n";
    header << "#endif // EMBEDDED_RESOURCES_H\n";
    header.close();

    // 生成实现文件
    ofstream cpp("src/embedded_resources.cpp");
    cpp << "#include \"embedded_resources.h\"\n";
    cpp << "#include <fstream>\n";
    cpp << "#include <iostream>\n\n";

    for (const auto &res : resources)
    {
        vector<unsigned char> data = readBinaryFile(res.filename);
        if (data.empty())
            continue;

        cpp << "// " << res.description << "\n";
        cpp << "const unsigned char EmbeddedResources::" << res.varName << "[] = {\n";
        for (size_t i = 0; i < data.size(); ++i)
        {
            if (i % 16 == 0)
                cpp << "    ";
            cpp << "0x" << hex << setfill('0') << setw(2) << (int)data[i];
            if (i < data.size() - 1)
                cpp << ",";
            if (i % 16 == 15)
                cpp << "\n";
        }
        if (data.size() % 16 != 0)
            cpp << "\n";
        cpp << "};\n\n";
        cpp << "const size_t EmbeddedResources::" << res.varName << "_size = " << dec << data.size() << ";\n\n";
    }

    // 生成辅助函数
    cpp << "namespace EmbeddedResources {\n\n";
    cpp << "std::string getXMLContent() {\n";
    cpp << "    return std::string(reinterpret_cast<const char*>(gui_design_xml), gui_design_xml_size);\n";
    cpp << "}\n\n";

    cpp << "const unsigned char* getAudioData(size_t& size) {\n";
    cpp << "    size = notification_mp3_size;\n";
    cpp << "    return notification_mp3;\n";
    cpp << "}\n\n";

    cpp << "bool writeAudioToTempFile(const std::string& tempPath) {\n";
    cpp << "    std::ofstream file(tempPath, std::ios::binary);\n";
    cpp << "    if (!file) return false;\n";
    cpp << "    file.write(reinterpret_cast<const char*>(notification_mp3), notification_mp3_size);\n";
    cpp << "    return file.good();\n";
    cpp << "}\n\n";
    cpp << "}\n";

    cpp.close();

    cout << "Generated embedded resources files successfully!" << endl;
}

int main()
{
    vector<ResourceInfo> resources = {
        {"src/GUIDesign.xml", "gui_design_xml", "GTK UI Design XML"},
        {"asserts/notification.mp3", "notification_mp3", "Notification sound MP3"}};

    generateResourceFiles(resources);
    return 0;
}
