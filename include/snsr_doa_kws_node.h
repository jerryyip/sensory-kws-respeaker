#ifndef __SNSR_DOA_KWS_NODE__
#define __SNSR_DOA_KWS_NODE__

#include "chain_nodes/base_node.h"
#include "chain_nodes/direction_reporter_node.h"
#include "chain_nodes/hotword_detection_node.h"

namespace respeaker
{

/**
 * Please note that, this node can only uplink to VepAecBeamformingNode.
 */
class SnsrDoaKwsNode : public BaseNode, public DirectionReporterNode, public HotwordDetectionNode
{
public:
    /**
     * @param snsr_model_path
     * @param output_interleaved - Set it true to output interleaved data, false to output deinterleaved data.
     *                          At most time, we don't have to set it true in this node. Because the output data
     *                          has only one channel(beam). 
     *
     * @return SnsrDoaKwsNode*
     */
    static SnsrDoaKwsNode* Create(std::string snsr_model_path,
                                 bool output_interleaved=false);

    /**
     * @param underclocking_count - the count of input block this node will wait before
     *                              processing and passing down.
     */
    static SnsrDoaKwsNode* Create(std::string snsr_model_path,
                                 int underclocking_count,
                                 bool output_interleaved=false);

    /**
     * @param enable_agc - enable automatic gain control
     *
     * @return SnsrDoaKwsNode*
     */
    static SnsrDoaKwsNode* Create(std::string snsr_model_path,
                                 int underclocking_count,
                                 bool enable_agc, 
                                 bool output_interleaved=false);

    virtual ~SnsrDoaKwsNode() = default;

    /**
     * Time length over which we can confirm that trigger has been post for all beams,
     * then we begin to scoring the triggerd beams, finally calculated the target beam
     *
     * @param ms - milliseconds
     */
    virtual void SetTriggerPostConfirmThresholdTime(int ms) = 0;

    /**
     * Set the angle for microphone 0, if it's not `0` for the board. For ReSpeaker v2, this should be `30`.
     *
     * @param angle - degree
     */
    virtual void SetAngleForMic0(int angle) = 0;

    /**
     * Whether or not do AEC when the state machine is in LISTEN* state.
     *
     * @param do_aec_when_listen - The default is true.
     */
    virtual void SetDoAecWhenListen(bool do_aec_when_listen) = 0;

    /**
     * @see ReSpeaker::SetChainState
     *
     */
    virtual void DisableAutoStateTransfer() = 0;

    /**
     * @param dbfs - [0, 31], Sets the target peak |level| (or envelope) of the AGC in dBFs (decibels
     *               from digital full-scale). The convention is to use positive values. For
     *               instance, passing in a value of 3 corresponds to -3 dBFs, or a target
     *               level 3 dB below full-scale. The default is 3.
     */
    virtual void SetAgcTargetLevelDbfs(int dbfs) = 0;

    virtual bool OnStartThread() = 0;
    virtual std::string ProcessBlock(std::string block, bool &exit) = 0;
    virtual bool OnJoinThread() = 0;

    virtual int GetDirection() = 0;
    virtual bool HotwordDetected() = 0;

};

}  //namespace

#endif // __SNSR_DOA_KWS_NODE__