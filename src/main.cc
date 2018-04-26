#include "snsr.h"
#include <thread>
#include <csignal>
#include <mutex>
#include <cstddef>
#include <cstdint>
#include <iostream>

//using namespace std;

const std::string ALEXA_TASK_VERSION = "~0.7.0";
// const std::string MODEL_FILE = "../ext/resources/spot-alexa-rpi.snsr";
const std::string MODEL_FILE = "models/spot-alexa-rpi-20500.snsr";

bool init();
bool start();
void main_loop();
void stop();


// audio is acquired and processed in this thread
std::unique_ptr<std::thread> m_thread;
bool is_running = false;
std::mutex mutex_is_running;
bool is_stop = false;

SnsrSession m_session;

void int_handler(int signal)
{
    printf("Caught signal %d, quit...\n", signal);
    
    stop();
    is_stop = true;
}

int main(int argc, char *argv[])
{
    // Configures signal handling.
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = int_handler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    if (init())
    {
        std::cout << "******* Init success *******" << std::endl;
    }
    else
    {
        stop();
    }
    if (start())
    {
        std::cout << "******* Start success *******" << std::endl;
    }
    else
    {
        stop();
    }

    while (!is_stop)
    {
	std::cout << "running..." << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;

}

// Callback for whenever audio samples have been read from the input stream
// and are about to be processed.
// This callback exists to allow sensory to gracefully exit when we wish
// to end the thread.
SnsrRC samplesReadySensoryCallback(SnsrSession s, const char* key, void* userData) {
    bool _is_running;
    {
        std::lock_guard<std::mutex> _lock(mutex_is_running);
        _is_running = is_running;
    }

    if(!_is_running) {
        return SNSR_RC_INTERRUPTED;
    }
    return SNSR_RC_OK;
}

// Callback for when the wakeword is detected
SnsrRC wakeWordDetectedSensoryCallback(SnsrSession s, const char* key, void* userData) {
    static int number = 0;
    std::cout << " *** Wakeword Detected ***" << ++number << std::endl;
    // SensoryWakeWordEngine* engine =
    //         static_cast<SensoryWakeWordEngine*>(userData);
    // engine->callWakeWordDetected();
    std::system("aplay ding.wav");
    return SNSR_RC_OK;
}

// Returns information about the ongoing sensory session.
// Primarily used to populate error messages.
std::string getSensoryDetails(SnsrSession session, SnsrRC result) {

   	std::string message;
	if(session) {
    	message = snsrErrorDetail(session);
    } else {
        message = snsrRCMessage(result);
    }
    if("" == message) {
        message = "Unrecognized error";
	}                     
    return message;
}


bool init()
{
    if (m_session)
    {
        // fprintf(stderr, "m_session has been initialized!\n");
        std::cout << "m_session has been initialized!" << std::endl;
        return false;
    }

    fprintf(stdout, "Initializing Sensory\n");

    // Allocate the Sensory library handle
    SnsrRC result = snsrNew(&m_session);
    if(result != SNSR_RC_OK){
        std::cout << "Could not allocation a new Sensory session: " 
                  << getSensoryDetails(m_session, result) << std::endl;
        return false;
    }

    // Get the expiration date of the library
    const char* info = nullptr;
    result = snsrGetString(m_session, SNSR_LICENSE_EXPIRES, &info);
    if(result == SNSR_RC_OK && info) {
        std::cout << "Library expires on: " << std::string(info) << std::endl;
        // fprintf(stdout, "Library expires on: " + std::string(info));
    } else {
        std::cout << "Library does not expire" << std::endl;
        // fprintf(stderr, "Library does not expire\n");
        return false;
    }

    // Check if the expiration date is near, then we should display a warning
    result = snsrGetString(m_session, SNSR_LICENSE_WARNING, &info);
    if(result == SNSR_RC_OK && info) {
        std::cout << "Library expires in: " << std::string(info) << std::endl;
        // fprintf(stdour, "Library expires in: " + std::string(info));
    } else {
        std::cout << "Library expiration is valid for at least 60 days." << std::endl;
        // fprintf(stderr, "Library expiration is valid for at least 60 days.");
    }

    if( snsrLoad(m_session, snsrStreamFromFileName(MODEL_FILE.c_str(), "r")) != SNSR_RC_OK ||
        snsrRequire(m_session, SNSR_TASK_TYPE, SNSR_PHRASESPOT) != SNSR_RC_OK ||
        snsrRequire(m_session, SNSR_TASK_VERSION, ALEXA_TASK_VERSION.c_str()) != SNSR_RC_OK) {
            std::cout << "Could not load and configure Sensory model: " <<
                         getSensoryDetails(m_session, result) << std::endl;
            return false;
    }

    // like a switch
    result = snsrSetHandler(m_session, SNSR_SAMPLES_EVENT,
                            snsrCallback(samplesReadySensoryCallback, nullptr, nullptr));
    if(result != SNSR_RC_OK) {
        std::cout << "Could not set audio samples callback: " 
                << getSensoryDetails(m_session, result) << std::endl;
        // throw WakeWordException("Could not set audio samples callback: " +
        //                                 getSensoryDetails(m_session, result));
    }

    // do callback after detecting key word
    result = snsrSetHandler(m_session, SNSR_RESULT_EVENT,
                            snsrCallback(wakeWordDetectedSensoryCallback, nullptr, nullptr));
    if(result != SNSR_RC_OK) {
        std::cout << "Could not set audio samples callback: " 
                << getSensoryDetails(m_session, result) << std::endl;
        // throw WakeWordException("Could not set wake word detected callback: " +
        //                                 getSensoryDetails(m_session, result));
    }
    return true;

}

bool start()
{
    if (is_running)
    {
        std::cout << "Sensory KWS is already running" << std::endl;
        return false;
    }

    std::cout << "Starting Sensory KWS " << std::endl;
    // Associate the microphone input with Sensory session.
    SnsrRC result = snsrSetStream(m_session,
                                    SNSR_SOURCE_AUDIO_PCM,
                                    snsrStreamFromAudioDevice(SNSR_ST_AF_DEFAULT));

    if(result != SNSR_RC_OK) {
        std::cout << "Can not set audio stream from default mic: "
                  << getSensoryDetails(m_session, result) << std::endl;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_is_running);
        is_running = true;
    }
    
    m_thread.reset(new std::thread(&main_loop));
    // m_thread = make_unique<std::thread>(&SensoryWakeWordEngine::mainLoop, this);
    return true;
} 

void main_loop()
{
    std::cout << "Sensory KWS mainloop thread starting" << std::endl;

    SnsrRC result = snsrRun(m_session);
    if (result != SNSR_RC_OK &&
        result != SNSR_RC_INTERRUPTED) {
        std::cout << "An error happened in the mainLoop of SensoryWakeWord " 
                << getSensoryDetails(m_session, result) << std::endl;
    }

    std::cout << "SensoryWakeWordEngine: mainLoop thread ended" << std::endl;
}

void stop()
{
	{
	    std::lock_guard<std::mutex> lock(mutex_is_running);
		is_running = false;
	}
	m_thread->join();
	snsrClearRC(m_session);
	snsrSetStream(m_session,SNSR_SOURCE_AUDIO_PCM, nullptr);
	if (m_session)
    {
        snsrRelease(m_session);
        m_session = nullptr;
    }
    std::cout << "********** Sensory KWS stop! **********" << std::endl;
}
