#include "../include/sensory.h"
#include <iostream>
#include <cstdint>
#include <cstddef>

namespace sensory {

/**
 * Returns information about the ongoing sensory session. Primarily used to populate error messages.
 *
 * @param session The Sensory session handle.
 * @param result The Sensory return code.
 *
 * @return The pertinent message about the sensory session in string format.
 */
static std::string getSensoryDetails(SnsrSession session, SnsrRC result) {
    std::string message;
    // It is recommended by Sensory to prefer snsrErrorDetail() over snsrRCMessage() as it provides more details.
    if (session) {
        message = snsrErrorDetail(session);
    } else {
        message = snsrRCMessage(result);
    }
    if (message.empty()) {
        message = "Unrecognized error";
    }
    return message;
}


// Callback for when the wakeword is detected
SnsrRC SensoryDetect::wakeWordDetectedSensoryCallback(SnsrSession s, const char* key, void* userData) {
    SensoryDetect* engine = static_cast<SensoryDetect*>(userData);
    SnsrRC result;
    const char* keyword;
    double begin;
    double end;
    result = snsrGetDouble(s, SNSR_RES_BEGIN_SAMPLE, &begin);
    if (result != SNSR_RC_OK) {
        std::cout << "keyWordDetectedCallbackFailed: " << getSensoryDetails(s, result) << std::endl;
        return result;
    }

    result = snsrGetDouble(s, SNSR_RES_END_SAMPLE, &end);
    if (result != SNSR_RC_OK) {
        std::cout << "keyWordDetectedCallbackFailed: " << getSensoryDetails(s, result) << std::endl;
        return result;
    }

    result = snsrGetString(s, SNSR_RES_TEXT, &keyword);
    if (result != SNSR_RC_OK) {
        std::cout << "keyWordDetectedCallbackFailed: " << getSensoryDetails(s, result) << std::endl;
        return result;
    }

    // engine->notifyKeyWordObservers(
    //     engine->m_stream,
    //     keyword,
    //     engine->m_beginIndexOfStreamReader + begin,
    //     engine->m_beginIndexOfStreamReader + end);
    std::cout << "Wake word Detected: " << keyword << "  [begin: " << begin << "]  [end: " << end << "]\n";

    // static int number = 0;
    // std::cout << " *** Wakeword Detected ***" << ++number << std::endl;
    std::system("aplay ding.wav");
    engine->_detection_status = 1;
    return SNSR_RC_OK;
}

// init
// 
SensoryDetect::SensoryDetect(const std::string& model_name)
{
    _alexa_task_version = std::string("~0.7.0");
    _detection_status = 0;
}


bool SensoryDetect::init()
{
    if (_m_session)
    {
        std::cout << "m_session has been initialized!" << std::endl;
        return false;
    }

    std::cout << "Initializing Sensory\n";

    // Allocate the Sensory library handle
    SnsrRC result = snsrNew(&_m_session);
    if(result != SNSR_RC_OK){
        std::cout << "Could not allocation a new Sensory session: " 
                  << getSensoryDetails(_m_session, result) << std::endl;
        return false;
    }

    // Get the expiration date of the library
    const char* info = nullptr;
    result = snsrGetString(_m_session, SNSR_LICENSE_EXPIRES, &info);
    if(result == SNSR_RC_OK && info) {
        std::cout << "Library expires on: " << std::string(info) << std::endl;
    } else {
        std::cout << "Library does not expire" << std::endl;
        return false;
    }

    // Check if the expiration date is near, then we should display a warning
    result = snsrGetString(_m_session, SNSR_LICENSE_WARNING, &info);
    if(result == SNSR_RC_OK && info) {
        std::cout << "Library expires in: " << std::string(info) << std::endl;
    } else {
        std::cout << "Library expiration is valid for at least 60 days." << std::endl;
    }

    if( snsrLoad(_m_session, snsrStreamFromFileName(_model_file_path.c_str(), "r")) != SNSR_RC_OK ||
        snsrRequire(_m_session, SNSR_TASK_TYPE, SNSR_PHRASESPOT) != SNSR_RC_OK ||
        snsrRequire(_m_session, SNSR_TASK_VERSION, _alexa_task_version.c_str()) != SNSR_RC_OK) {
            std::cout << "Could not load and configure Sensory model: " <<
                         getSensoryDetails(_m_session, result) << std::endl;
            return false;
    }

    // // do not need this in avs_sdk
    // result = snsrSetHandler(_m_session, SNSR_SAMPLES_EVENT,
    //                         snsrCallback(samplesReadySensoryCallback, nullptr, nullptr));
    // if(result != SNSR_RC_OK) {
    //     std::cout << "Could not set audio samples callback: " 
    //             << getSensoryDetails(_m_session, result) << std::endl;
    // }

    // do callback after detecting key word
    result = snsrSetHandler(_m_session, SNSR_RESULT_EVENT,
                            snsrCallback(wakeWordDetectedSensoryCallback, nullptr, reinterpret_cast<void*>(this)));
    if(result != SNSR_RC_OK) {
        std::cout << "Could not set audio samples callback: " 
                << getSensoryDetails(_m_session, result) << std::endl;
    }
    return true;
}


// input 40ms data
int SensoryDetect::RunDetection(const int16_t* const data, const int array_length, bool is_end)
{
    _detection_status = 0;
    int err;
    snsrSetStream(
        _m_session,
        SNSR_SOURCE_AUDIO_PCM,
        snsrStreamFromMemory(
            data, array_length * sizeof(int16_t), SNSR_ST_MODE_READ));
    SnsrRC result = snsrRun(_m_session);
    switch (result) {
        case SNSR_RC_STREAM_END:
            // Reached end of buffer without any keyword detections
            err = 0;
            break;
        case SNSR_RC_OK:
            err = 0;
            break;
        default:
            // A different return from the callback function that indicates some sort of error
            // ACSDK_ERROR(LX("detectionLoopFailed")
            //                 .d("reason", "unexpectedReturn")
            //                 .d("error", getSensoryDetails(m_session, result)));
            std::cout << "RunDetection Error: " << getSensoryDetails(_m_session, result) << std::endl;
            err = 1;
            break;
    }
    // Reset return code for next round
    snsrClearRC(_m_session);
    return err;
}


int SensoryDetect::GetDetectionStatus()
{
    return _detection_status;
}


SensoryDetect::~SensoryDetect()
{
    snsrRelease(_m_session);
}

} // namespace
