#pragma once

#include <rlbot/bot.h>
#include <RLGymCPP/ObsBuilders/ObsBuilder.h>
#include <RLGymCPP/ActionParsers/ActionParser.h>
#include <GigaLearnCPP/Util/InferUnit.h>
#include <GigaLearnCPP/Util/ModelConfig.h>

#include <RLGymCPP/Framework.h>
#include <memory>
#include <map>

namespace RLConstructor {
    constexpr float JUMP_MAX_TIME = 0.2f;
    constexpr float JUMP_MIN_TIME = 0.025f;
    constexpr float FLIP_TORQUE_TIME = 0.65f;
    constexpr float FLIP_TORQUE_MIN_TIME = 0.41f;
    constexpr float FLIP_PITCHLOCK_TIME = 1.0f;
    constexpr float BOOST_MIN_TIME = 0.1f;
    constexpr float POWERSLIDE_RISE_RATE = 5.f;
    constexpr float POWERSLIDE_FALL_RATE = 2.f;
    constexpr float SUPERSONIC_MAINTAIN_MAX_TIME = 1.f;
    constexpr float CAR_AUTOFLIP_TIME = 0.4f;
    constexpr float CAR_AUTOFLIP_ROLL_THRESH = 2.8f;
    constexpr float CAR_AUTOFLIP_NORMZ_THRESH = 0.70710678118f;
    constexpr float BUMP_COOLDOWN_TIME = 0.25f;
}

namespace RLConst {
    constexpr float GRAVITY_Z = -650.f;
    constexpr float AIR_DRAG = 0.97f;
    constexpr float CAR_MAX_SPEED = 2300.f;
    constexpr float CAR_MAX_THROTTLE_SPEED = 1410.f;
    constexpr float BOOST_ACCEL = 991.666f;
    constexpr float BOOST_CONSUMPTION_RATE = 33.3f;
    constexpr float SUPERSONIC_THRESHOLD = 2200.f;
    constexpr float DEMO_SPEED_THRESHOLD = 2300.f;
    constexpr float BALL_RADIUS = 91.25f;
    constexpr float BALL_MAX_SPEED = 6000.f;
}


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

        float autoFlipTimer = 0.f;
        bool autoFlipAttacking = false;
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
