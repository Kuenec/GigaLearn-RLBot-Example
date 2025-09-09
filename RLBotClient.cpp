#include "RLBotClient.h"
#include <rlbot/platform.h>
#include <rlbot/botmanager.h>

using namespace RLGC;
using namespace GGL;

RLBotParams g_RLBotParams = {};

rlbot::Bot* BotFactory(int index, int team, std::string name) {
    return new RLBotBot(index, team, name, g_RLBotParams);
}

Vec ToVec(const rlbot::flat::Vector3* rlbotVec) {
    if (!rlbotVec) return Vec();
    return Vec(rlbotVec->x(), rlbotVec->y(), rlbotVec->z());
}

PhysState ToPhysState(const rlbot::flat::Physics* phys) {
    PhysState obj = {};
    if (phys) {
        obj.pos = ToVec(phys->location());
        if (phys->rotation()) {
            Angle ang = Angle(phys->rotation()->yaw(), phys->rotation()->pitch(), phys->rotation()->roll());
            obj.rotMat = ang.ToRotMat();
        }
        obj.vel = ToVec(phys->velocity());
        obj.angVel = ToVec(phys->angularVelocity());
    }
    return obj;
}

RLBotBot::RLBotBot(int _index, int _team, std::string _name, const RLBotParams& params)
    : rlbot::Bot(_index, _team, _name), params(params) {
    RG_LOG("Created RLBot bot: index " << _index << ", name: " << name << "...");
}

RLBotBot::~RLBotBot() {
}

void RLBotBot::UpdateGameState(rlbot::GameTickPacket& packet, float deltaTime, float curTime) {

    prevGs = gs;
    gs = {};
    gs.lastTickCount = packet->gameInfo()->frameNum();
    gs.deltaTime = deltaTime;

    PhysState ballPhys = ToPhysState(packet->ball()->physics());

    static_cast<PhysState&>(gs.ball) = ballPhys;
    auto latestTouch = packet->ball()->latestTouch();

    auto boostPadStates = packet->boostPadStates();
    gs.boostPads.resize(CommonValues::BOOST_LOCATIONS_AMOUNT, true);
    gs.boostPadsInv.resize(CommonValues::BOOST_LOCATIONS_AMOUNT, true);
    gs.boostPadTimers.resize(CommonValues::BOOST_LOCATIONS_AMOUNT, 0);
    gs.boostPadTimersInv.resize(CommonValues::BOOST_LOCATIONS_AMOUNT, 0);

    if (boostPadStates && boostPadStates->size() == CommonValues::BOOST_LOCATIONS_AMOUNT) {
        for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
            gs.boostPads[i] = boostPadStates->Get(i)->isActive();
            gs.boostPadsInv[CommonValues::BOOST_LOCATIONS_AMOUNT - i - 1] = gs.boostPads[i];
            gs.boostPadTimers[i] = boostPadStates->Get(i)->timer();
            gs.boostPadTimersInv[CommonValues::BOOST_LOCATIONS_AMOUNT - i - 1] = gs.boostPadTimers[i];
        }
    }

    auto players = packet->players();
    gs.players.resize(players->size());
    for (int i = 0; i < players->size(); i++) {
        auto playerInfo = players->Get(i);
        Player& player = gs.players[i];
        Player* prevPlayer = (prevGs.players.size() > i && prevGs.players[i].carId == playerInfo->spawnId()) ? &prevGs.players[i] : nullptr;
        PlayerInternalState& internalState = internalPlayerStates[i];

        static_cast<PhysState&>(player) = ToPhysState(playerInfo->physics());
        player.carId = playerInfo->spawnId();
        player.team = (Team)playerInfo->team();
        player.boost = playerInfo->boost();
        player.isDemoed = playerInfo->isDemolished();
        player.isOnGround = playerInfo->hasWheelContact();
        player.hasJumped = playerInfo->jumped();
        player.hasDoubleJumped = playerInfo->doubleJumped();
        player.isSupersonic = playerInfo->isSupersonic();
        player.index = i;
        player.prev = prevPlayer;

        if (player.isOnGround) {
            internalState.isJumping = false;
            internalState.isFlipping = false;
            internalState.jumpTime = 0;
            internalState.flipTime = 0;
        } else {
            if (internalState.isJumping) {
                internalState.jumpTime += deltaTime;
                if (internalState.jumpTime >= RLConst::JUMP_MAX_TIME) {
                    internalState.isJumping = false;
                }
            }
            if (internalState.isFlipping) {
                internalState.flipTime += deltaTime;
                if (internalState.flipTime >= RLConst::FLIP_TORQUE_TIME) {
                    internalState.isFlipping = false;
                }
            }
        }

        if (prevPlayer) {
            if (player.hasJumped && !prevPlayer->hasJumped && prevPlayer->isOnGround) {
                internalState.isJumping = true;
                internalState.jumpTime = 0;
            }
            if (player.hasDoubleJumped && !prevPlayer->hasDoubleJumped && !prevPlayer->isOnGround) {
                internalState.isFlipping = true;
                internalState.flipTime = 0;
            }
        }
        
        player.isJumping = internalState.isJumping;
        player.isFlipping = internalState.isFlipping;
        player.jumpTime = internalState.jumpTime;
        player.flipTime = internalState.flipTime;
        player.airTimeSinceJump = player.hasJumped && !player.isJumping ? (prevPlayer ? prevPlayer->airTimeSinceJump + deltaTime : deltaTime) : 0;
        
        player.ballTouchedStep = false;
        player.ballTouchedTick = false;
        if (latestTouch && latestTouch->playerIndex() == i) {
            float timeSinceTouch = curTime - latestTouch->gameSeconds();

            if (timeSinceTouch < (params.tickSkip * CommonValues::TICK_TIME) + 0.01f) {
                player.ballTouchedStep = true;
                gs.lastTouchCarID = player.carId;
            }

            if (timeSinceTouch < deltaTime + 0.01f) {
                 player.ballTouchedTick = true;
            }
        }
    }
    
    gs.goalScored = false;
    for (int i = 0; i < 2; i++) {
        int currentScore = packet->teams()->Get(i)->score();
        if (currentScore > lastTeamScores[i]) {
            gs.goalScored = true;
        }
        lastTeamScores[i] = currentScore;
    }
}

rlbot::Controller RLBotBot::GetOutput(rlbot::GameTickPacket gameTickPacket) {

    float curTime = gameTickPacket->gameInfo()->secondsElapsed();
    if (prevTime == 0) prevTime = curTime;
    float deltaTime = curTime - prevTime;
    prevTime = curTime;

    int ticksElapsed = (ticks == -1) ? params.tickSkip : roundf(deltaTime * 120);

    if (ticksElapsed == 0 && ticks != -1) {
        rlbot::Controller output_controller = {};
        output_controller.throttle = controls.throttle;
        output_controller.steer = controls.steer;
        output_controller.pitch = controls.pitch;
        output_controller.yaw = controls.yaw;
        output_controller.roll = controls.roll;
        output_controller.jump = controls.jump != 0;
        output_controller.boost = controls.boost != 0;
        output_controller.handbrake = controls.handbrake != 0;
        return output_controller;
    }
    ticks += ticksElapsed;

    UpdateGameState(gameTickPacket, deltaTime, curTime);
    
    auto& localPlayer = gs.players[index];
    localPlayer.prevAction = controls;

    if (updateAction) {
        updateAction = false;
        action = params.inferUnit->InferAction(localPlayer, gs, params.deterministic);
    }

    if (ticks >= (params.actionDelay - 1) || ticks == -1) {
        controls = action;
    }

    if (ticks >= params.tickSkip || ticks == -1) {
        ticks = 0;
        updateAction = true;
    }

    rlbot::Controller output_controller = {};
    output_controller.throttle = controls.throttle;
    output_controller.steer = controls.steer;
    output_controller.pitch = controls.pitch;
    output_controller.yaw = controls.yaw;
    output_controller.roll = controls.roll;
    output_controller.jump = controls.jump != 0;
    output_controller.boost = controls.boost != 0;
    output_controller.handbrake = controls.handbrake != 0; 
    output_controller.useItem = false;

    return output_controller;
}

void RLBotClient::Run(const RLBotParams& params) {
    g_RLBotParams = params;
    rlbot::platform::SetWorkingDirectory(rlbot::platform::GetExecutableDirectory());
    rlbot::BotManager botManager(BotFactory);
    botManager.StartBotServer(params.port);
}
