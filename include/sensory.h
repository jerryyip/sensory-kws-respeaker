
#ifndef __SENSORY_H__
#define __SENSORY_H__

#include <string>
#include "snsr.h"


namespace sensory
{


class SensoryApi
{
public:
    SensoryDetect(const std::string& model_file_path);

    bool init();
    // blocking
    // accept 
    // int RunDetection(const std::string& data, bool is_end = false);
    int RunDetection(const int16_t* const data, const int array_length, bool is_end = false);
    int GetDetectionStatus();
    ~SensoryDetect();

private:
    SnsrRC wakeWordDetectedSensoryCallback(SnsrSession s, const char* key, void* userData);
    int _detection_status;

    std::string _model_file_path;
    std::string _alexa_task_version;
    SnsrSession _m_session;

};

} // namespace sensory

#endif