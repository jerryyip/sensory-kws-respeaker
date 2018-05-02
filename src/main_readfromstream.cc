/**
 * Copyright (c) 2017 Seeed Technology Co., Ltd.
 *
 * @author Jack Shao <jacky.shaoxg@gmail.com>
 *
 */


#include <cstring>
#include <memory>
#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>

#include <respeaker.h>
#include <chain_nodes/pulse_collector_node.h>
#include <chain_nodes/selector_node.h>
#include "include/sensory.h"

extern "C"
{
#include <sndfile.h>
#include <unistd.h>
#include <getopt.h>
}


using namespace std;
using namespace respeaker;

static bool stop = false;


void SignalHandler(int signal){
  cerr << "Caught signal " << signal << ", terminating..." << endl;
  stop = true;
  //maintain the main thread untill the worker thread released its resource
  //std::this_thread::sleep_for(std::chrono::seconds(1));
}

static void help(const char *argv0) {
    cout << "main_base_test [options]" << endl;
    cout << "A demo application for librespeaker." << endl << endl;
    cout << "  -h, --help                               Show this help" << endl;
    cout << "  -s, --source=SOURCE_NAME                 The source (microphone) to connect to" << endl;
}


int main(int argc, char *argv[]) {

    // Configures signal handling.
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = SignalHandler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);
    sigaction(SIGTERM, &sig_int_handler, NULL);

    // start snsr
    snsr = SensoryDetect("models/spot-alexa-rpi-31000.snsr");
    snsr.init();


    // parse opts
    int c;
    string source = "default";

    static const struct option long_options[] = {
        {"help",         0, NULL, 'h'},
        {"source",       1, NULL, 's'},
        {NULL,           0, NULL,  0}
    };

    while ((c = getopt_long(argc, argv, "s:h", long_options, NULL)) != -1) {

        switch (c) {
        case 'h' :
            help(argv[0]);
            return 0;
        case 's':
            source = string(optarg);
            break;
        default:
            return 0;
        }
    }


    unique_ptr<PulseCollectorNode> collector;
    unique_ptr<SelectorNode> selector;
    unique_ptr<ReSpeaker> respeaker;

    collector.reset(PulseCollectorNode::Create(source, 16000));
    //std::vector<int> selected_channels {1, 0, 3};
    std::vector<int> selected_channels {0};
    selector.reset(SelectorNode::Create(selected_channels));

    selector->Uplink(collector.get());

    respeaker.reset(ReSpeaker::Create());
    respeaker->RegisterChainByHead(collector.get());
    respeaker->RegisterOutputNode(selector.get());

    if (!respeaker->Start(&stop)) {
        cout << "Can not start the respeaker node chain." << endl;
        return -1;
    }

    string data;
    int frames;
    size_t num_channels = respeaker->GetNumOutputChannels();
    int rate = respeaker->GetNumOutputRate();

    cout << "num channels: " << num_channels << ", rate: " << rate << endl;

    // init libsndfile
    SNDFILE	*file ;
    SF_INFO	sfinfo ;

    memset (&sfinfo, 0, sizeof (sfinfo)) ;
    sfinfo.samplerate	= rate ;
	sfinfo.channels		= num_channels ;
	sfinfo.format		= (SF_FORMAT_WAV | SF_FORMAT_PCM_24) ;
    if (! (file = sf_open ("record_base_test.wav", SFM_WRITE, &sfinfo)))
	{
        cout << "Error : Not able to open output file." << endl;
		return -1 ;
	}

    while (!stop)
    {
        data = respeaker->Listen();
        frames = data.length() / (sizeof(int16_t) * num_channels);
        sf_writef_short(file, (const int16_t *)(data.data()), frames);
        cout << "." << flush;
        //
        snsr.RunDetection((const int16_t *)data.data(), data.length());
    }

    cout << "stopping the respeaker worker thread..." << endl;

    respeaker->Stop();

    cout << "cleanup done." << endl;

    sf_close (file);

    return 0;
}


