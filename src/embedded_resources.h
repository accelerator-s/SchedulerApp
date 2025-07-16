#ifndef EMBEDDED_RESOURCES_H
#define EMBEDDED_RESOURCES_H

#include <cstddef>
#include <string>
#include <memory>

namespace EmbeddedResources {

    // GTK UI Design XML
    extern const unsigned char gui_design_xml[];
    extern const size_t gui_design_xml_size;

    // Notification sound MP3
    extern const unsigned char notification_mp3[];
    extern const size_t notification_mp3_size;

    // Helper functions
    std::string getXMLContent();
    const unsigned char* getAudioData(size_t& size);
    bool writeAudioToTempFile(const std::string& tempPath);

}

#endif // EMBEDDED_RESOURCES_H
