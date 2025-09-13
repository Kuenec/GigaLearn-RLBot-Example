#pragma once

#include <rlbot/bot.h>
#include <RLGymCPP/ObsBuilders/ObsBuilder.h>
#include <RLGymCPP/ActionParsers/ActionParser.h>
#include <GigaLearnCPP/Util/InferUnit.h>
#include <GigaLearnCPP/Util/ModelConfig.h>

#include <RLGymCPP/Framework.h>
#include <memory>
#include <map>

struct RLBotParams {
    int port;
    int tickSkip;
    int actionDelay;
    bool deterministic = false;
    bool useGPU = true;

    RLGC::ObsBuilder* obsBuilder = nullptr;
    RLGC::ActionParser* actionParser = nullptr;
    GGL::InferUnit* inferUnit = nullptr;

    int obsSize;
    GGL::PartialModelConfig policyConfig;
    GGL::PartialModelConfig sharedHeadConfig;
};

class RLBotBot : public rlbot::Bot {
public:
    RLBotParams params;

    RLGC::Action
        action = {},
        controls = {};

    bool updateAction = true;
    float prevTime = 0;
    int ticks = -1;

    RLGC::GameState gs;
    RLGC::GameState prevGs;

    struct PlayerInternalState {
        float jumpTime = 0;
        float flipTime = 0;
        bool isJumping = false;
        bool isFlipping = false;
        uint64_t lastTouchTick = 0;

        float supersonicTime = 0;
        float timeSpentBoosting = 0;
        float handbrakeVal = 0;

        bool hasFlipped = false;
        float airTime = 0;
    };
    std::map<int, PlayerInternalState> internalPlayerStates;
    int lastTeamScores[2] = {0, 0};

    RLBotBot(int _index, int _team, std::string _name, const RLBotParams& params);
    ~RLBotBot();

    rlbot::Controller GetOutput(rlbot::GameTickPacket gameTickPacket) override;

private:
    void UpdateGameState(rlbot::GameTickPacket& packet, float deltaTime, float curTime);
};

namespace RLBotClient {
    void Run(const RLBotParams& params);
}
