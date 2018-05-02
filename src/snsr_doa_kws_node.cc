
#include <string>
#include "log.h"
#include "components/sensory.h"
#include "components/audio_processing_module.h"
#include "chain_nodes/vep_aec_bf_node.h"

namespace respeaker
{

class SnsrDoaKwsNodeImpl : public SnsrDoaKwsNode
{
public:
    SnsrDoaKwsNodeImpl(std::string snsr_model_path,
                      int underclocking_count,
                      bool enable_agc, 
                      bool output_interleaved);
    virtual ~SnsrDoaKwsNodeImpl() = default;

    virtual void SetTriggerPostConfirmThresholdTime(int ms);
    virtual void SetAngleForMic0(int angle);
    virtual void SetDoAecWhenListen(bool do_aec_when_listen);
    virtual void DisableAutoStateTransfer();
    virtual void SetAgcTargetLevelDbfs(int dbfs);

    virtual bool OnStartThread();
    virtual std::string ProcessBlock(std::string block, bool &exit);
    virtual bool OnJoinThread();

    virtual int GetDirection();
    virtual bool HotwordDetected();

private:
    int _underclocking_count, _underclocking_count_cur;
    bool _enable_agc;

    std::unique_ptr<AudioProcessingModule> _apm;
    int _agc_level;
    int _num_frames_apm;
    int _apm_allowed_block_size_ms;

    std::string _buffered_data;
    std::string _buffer_before_process[NUM_OF_BEAMS + 1];
    std::string _buffer_after_process[NUM_OF_BEAMS + 1];
    std::string _buffer_before_apm;

    //short bli_sco[NUM_OF_BEAMS + 1];

    bool _do_aec_when_listen;
    bool _disable_auto_state_transfer;

    int _num_frames;
    int _input_channels, _output_channels;
    int _block_len_ms;

    // std::unique_ptr<snowboy::SnowboyApi> _snowboy[NUM_OF_BEAMS];
    // int _snowboy_detected[NUM_OF_BEAMS];

    bool _triggering, _fake_trigger;
    int _cnt_non_trigger;
    int _trigger_post_confirm_time;
    bool _hotworddetected_flag;
    TimePoint _last_fake_trigger_time;

    std::multimap<int, int, std::greater<int>> _map_score_beam;
    int _calculated_dir;
    int _angle_offset;
    int _last_fixed_beam, _last_beam_picked_for_asr;
    std::mutex _mutex_dir_hotworddetect;

    log4cplus::Logger _logger;
};
} // namespace