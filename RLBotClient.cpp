#include "RLBotClient.h"
#include <rlbot/platform.h>
#include <rlbot/botmanager.h>
#include <cmath>

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

void RLBotBot::UpdateBallHitInfo(Player& player, PlayerInternalState& internalState, 
                                  float curTime, const rlbot::flat::Touch* latestTouch) {
    // Update ball hit info if this player touched the ball
    if (latestTouch && latestTouch->playerIndex() == player.index) {
        float timeSinceTouch = curTime - latestTouch->gameSeconds();
        
        // Only update if this is a recent touch
        if (timeSinceTouch < 0.1f && gs.lastTickCount > internalState.ballHitInfo.tickCountWhenHit) {
            internalState.ballHitInfo.isValid = true;
            internalState.ballHitInfo.tickCountWhenHit = gs.lastTickCount;
            
            // Calculate relative position on ball
            Vec ballPos = gs.ball.pos;
            Vec touchLocation = ToVec(latestTouch->location());
            internalState.ballHitInfo.ballPos = ballPos;
            internalState.ballHitInfo.relativePosOnBall = touchLocation - ballPos;
            
            // Extra hit velocity (approximated - RLBot doesn't provide this directly)
            internalState.ballHitInfo.extraHitVel = Vec(0, 0, 0);
        }
    } else {
        // Invalidate old hit info after some time
        if (gs.lastTickCount > internalState.ballHitInfo.tickCountWhenHit + 120) {
            internalState.ballHitInfo.isValid = false;
        }
    }
    
    // Note: We don't assign to player.ballHitInfo since Player doesn't have this field
    // The internal state tracking is sufficient for observation builders that need it
}

void RLBotBot::UpdatePlayerState(Player& player, Player* prevPlayer, 
                                 PlayerInternalState& internalState, 
                                 float deltaTime, bool isLocalPlayer) {
    
    if (player.isSupersonic) {
        internalState.supersonicTime += deltaTime;
        if (internalState.supersonicTime > RLBotConst::SUPERSONIC_MAINTAIN_MAX_TIME) {
            internalState.supersonicTime = RLBotConst::SUPERSONIC_MAINTAIN_MAX_TIME;
        }
    } else {
        internalState.supersonicTime = 0;
    }
    player.supersonicTime = internalState.supersonicTime;
    
    bool currentlyBoosting = (isLocalPlayer && controls.boost) || 
                            (prevPlayer && prevPlayer->prevAction.boost != 0);
    
    if (internalState.timeSpentBoosting > 0) {
        if (!currentlyBoosting && internalState.timeSpentBoosting >= RLBotConst::BOOST_MIN_TIME) {
            internalState.timeSpentBoosting = 0;
        } else {
            internalState.timeSpentBoosting += deltaTime;
        }
    } else {
        if (currentlyBoosting) {
            internalState.timeSpentBoosting = deltaTime;
        }
    }
    player.timeSpentBoosting = internalState.timeSpentBoosting;
    bool currentlyHandbraking = (isLocalPlayer && controls.handbrake) || 
                               (prevPlayer && prevPlayer->prevAction.handbrake != 0);
    
    if (currentlyHandbraking) {
        internalState.handbrakeVal += RLBotConst::POWERSLIDE_RISE_RATE * deltaTime;
    } else {
        internalState.handbrakeVal -= RLBotConst::POWERSLIDE_FALL_RATE * deltaTime;
    }
    internalState.handbrakeVal = RS_CLAMP(internalState.handbrakeVal, 0.f, 1.f);
    player.handbrakeVal = internalState.handbrakeVal;
    
    // We estimate by setting all 4 wheels to the same state (API limitation)
    int numWheelsInContact = player.isOnGround ? 4 : 0;
    
    for (int i = 0; i < 4; i++) {
        internalState.wheelsWithContact[i] = (numWheelsInContact > 0);
        player.wheelsWithContact[i] = internalState.wheelsWithContact[i];
    }
    internalState.numWheelsInContact = numWheelsInContact;
    
    if (player.isDemoed) {
        if (!internalState.wasDemoedLastFrame) {
            // Just got demoed this frame
            internalState.demoRespawnTimer = RLBotConst::DEMO_RESPAWN_TIME;
            internalState.demoTick = gs.lastTickCount;
        } else {
            // Continue counting down
            internalState.demoRespawnTimer -= deltaTime;
            if (internalState.demoRespawnTimer < 0) {
                internalState.demoRespawnTimer = 0;
            }
        }
    } else {
        if (internalState.wasDemoedLastFrame) {
            // Just respawned
            internalState.demoRespawnTimer = 0;
        }
    }
    player.demoRespawnTimer = internalState.demoRespawnTimer;
    internalState.wasDemoedLastFrame = player.isDemoed;
    
    if (internalState.carContact.cooldownTimer > 0) {
        internalState.carContact.cooldownTimer -= deltaTime;
        if (internalState.carContact.cooldownTimer < 0) {
            internalState.carContact.cooldownTimer = 0;
            internalState.carContact.otherCarID = 0;
        }
    }
    
    player.carContact.otherCarID = internalState.carContact.otherCarID;
    player.carContact.cooldownTimer = internalState.carContact.cooldownTimer;
    
    if (player.isOnGround) {
        internalState.isJumping = false;
        internalState.isFlipping = false;
        internalState.flipTime = 0;
        internalState.hasFlipped = false;
        internalState.flipRelTorque = Vec(0, 0, 0);
        internalState.airTime = 0;
        internalState.airTimeSinceJump = 0;
        internalState.autoFlipTimer = 0;
        internalState.isAutoFlipping = false;
        internalState.autoFlipTorqueScale = 0;
        
        // Reset hasJumped when landing (with time padding for minimum jumps)
        if (prevPlayer && prevPlayer->hasJumped) {
            if (internalState.jumpTime < RLBotConst::JUMP_MIN_TIME + RLBotConst::JUMP_RESET_TIME_PAD) {
                // Don't reset jump yet - might still be leaving ground after min-time jump
            } else {
                internalState.jumpTime = 0;
            }
        }
    } else {

        internalState.airTime += deltaTime;
        player.airTime = internalState.airTime;
        
        if (internalState.isJumping) {
            internalState.jumpTime += deltaTime;
            // Jump ends after JUMP_MAX_TIME
            if (internalState.jumpTime >= RLBotConst::JUMP_MAX_TIME) {
                internalState.isJumping = false;
            }
        }
        player.jumpTime = internalState.jumpTime;
        
        if (internalState.isFlipping) {
            internalState.flipTime += deltaTime;
            // Flip torque ends after FLIP_TORQUE_TIME
            if (internalState.flipTime >= RLBotConst::FLIP_TORQUE_TIME) {
                internalState.isFlipping = false;
            }
        }
        player.flipTime = internalState.flipTime;
        
        if (player.hasJumped && !internalState.isJumping) {
            internalState.airTimeSinceJump += deltaTime;
        } else {
            internalState.airTimeSinceJump = 0;
        }
        player.airTimeSinceJump = internalState.airTimeSinceJump;

        // Car auto-flips when upside down in the air for too long
        bool shouldAutoFlip = (player.rotMat.up.z < RLBotConst::CAR_AUTOFLIP_NORMZ_THRESH) && 
                             (std::abs(player.rotMat.forward.z) < 0.9f);
        
        if (shouldAutoFlip) {
            internalState.autoFlipTimer += deltaTime;
            if (internalState.autoFlipTimer >= RLBotConst::CAR_AUTOFLIP_TIME && !internalState.isAutoFlipping) {
                internalState.isAutoFlipping = true;
                // Calculate auto-flip direction based on roll angle
                Angle angles = Angle::FromRotMat(player.rotMat);
                float absRoll = std::abs(angles.roll);
                if (absRoll > RLBotConst::CAR_AUTOFLIP_ROLL_THRESH) {
                    internalState.autoFlipTorqueScale = (angles.roll > 0) ? 1.f : -1.f;
                }
            }
        } else {
            internalState.autoFlipTimer = 0;
            internalState.isAutoFlipping = false;
            internalState.autoFlipTorqueScale = 0;
        }
        
        // Flip reset occurs when all wheels touch a surface while in the air
        internalState.gotFlipResetThisFrame = false;
        
        // Detect flip reset: hasJumped or hasDoubleJumped goes false while in air
        if (prevPlayer) {
            bool hadJumpedBefore = internalState.hadJumpedLastFrame;
            bool hadDoubleJumpedBefore = internalState.hadDoubleJumpedLastFrame;
            
            // If hasJumped was true and is now false while in air to flip reset
            if (hadJumpedBefore && !player.hasJumped && !player.isOnGround) {
                internalState.gotFlipResetThisFrame = true;
                internalState.lastFlipResetTick = gs.lastTickCount;
                
                // Reset flip related states
                internalState.hasFlipped = false;
                internalState.isFlipping = false;
                internalState.flipTime = 0;
                internalState.flipRelTorque = Vec(0, 0, 0);
                internalState.airTimeSinceJump = 0;
            }
            
            // Alternative detection: doubleJumped goes false while in air
            if (hadDoubleJumpedBefore && !player.hasDoubleJumped && !player.isOnGround && player.hasJumped) {
                internalState.gotFlipResetThisFrame = true;
                internalState.lastFlipResetTick = gs.lastTickCount;
            }
        }
    }
    
    if (prevPlayer) {
        // Detect first jump
        if (player.hasJumped && !prevPlayer->hasJumped) {
            internalState.isJumping = true;
            internalState.jumpTime = 0;
            internalState.jumpReleased = false;
            // Starting a new jump cancels any flip state
            internalState.isFlipping = false;
            internalState.flipTime = 0;
        }
        
        // Detect second jump/flip (double jump changes from false to true)
        if (player.hasDoubleJumped && !prevPlayer->hasDoubleJumped && !player.isOnGround) {
            internalState.isFlipping = true;
            internalState.flipTime = 0;
            internalState.hasFlipped = true;
            internalState.isJumping = false; // Flip consumes second jump, ends jumping state
            
            // Calculate flip direction from controls
            // Get the current action being applied
            Action currentAction = isLocalPlayer ? controls : prevPlayer->prevAction;
            
            float pitch = currentAction.pitch;
            float yaw = currentAction.yaw;
            
            // Normalize the dodge direction
            Vec dodgeDir = Vec(pitch, yaw, 0);
            float dodgeMag = dodgeDir.Length();
            
            if (dodgeMag > 0.1f) {
                dodgeDir = dodgeDir.Normalized();
                
                // Apply deadzones (< 0.1 becomes 0)
                if (std::abs(dodgeDir.x) < 0.1f) dodgeDir.x = 0;
                if (std::abs(dodgeDir.y) < 0.1f) dodgeDir.y = 0;
                
                // Calculate relative flip torque: flipRelTorque = (-yaw, pitch, 0)
                // This matches RocketSim's calculation
                internalState.flipRelTorque = Vec(-dodgeDir.y, dodgeDir.x, 0);
            } else {
                // Neutral flip (straight up double jump) - no flip torque
                internalState.flipRelTorque = Vec(0, 0, 0);
            }
        }
    }
    
    player.isJumping = internalState.isJumping;
    player.isFlipping = internalState.isFlipping;
    player.hasFlipped = internalState.hasFlipped;
    player.flipRelTorque = internalState.flipRelTorque;
    player.isAutoFlipping = internalState.isAutoFlipping;
    player.autoFlipTimer = internalState.autoFlipTimer;
    player.autoFlipTorqueScale = internalState.autoFlipTorqueScale;
    
    // World contact (estimated based on wheel contact)
    player.worldContact.hasContact = internalState.worldContact.hasContact;
    player.worldContact.contactNormal = internalState.worldContact.contactNormal;
    
    // Store previous frame states for next update
    internalState.wasOnGroundLastFrame = player.isOnGround;
    internalState.wasInAirLastFrame = !player.isOnGround;
    internalState.hadJumpedLastFrame = player.hasJumped;
    internalState.hadDoubleJumpedLastFrame = player.hasDoubleJumped;
    // Note: Don't track hadWheelContactLastFrame since Player doesn't have hasWheelContact field
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
        Player* prevPlayer = (prevGs.players.size() > i && prevGs.players[i].carId == playerInfo->spawnId()) 
                            ? &prevGs.players[i] : nullptr;
        PlayerInternalState& internalState = internalPlayerStates[i];

        // Basic physics state
        static_cast<PhysState&>(player) = ToPhysState(playerInfo->physics());
        
        // Basic player info from RLBot
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

        // Update comprehensive state tracking (1:1 with RocketSim)
        bool isLocalPlayer = (i == index);
        UpdatePlayerState(player, prevPlayer, internalState, deltaTime, isLocalPlayer);
        
        // Update ball hit info
        UpdateBallHitInfo(player, internalState, curTime, latestTouch);
        
        player.ballTouchedStep = false;
        player.ballTouchedTick = false;
        
        if (latestTouch && latestTouch->playerIndex() == i) {
            float timeSinceTouch = curTime - latestTouch->gameSeconds();
            
            // Step touch: within the current step's time window
            if (timeSinceTouch < (params.tickSkip * CommonValues::TICK_TIME) + 0.01f) {
                player.ballTouchedStep = true;
                gs.lastTouchCarID = player.carId;
            }
            
            // Tick touch: within this specific tick
            if (timeSinceTouch < deltaTime + 0.01f) {
                player.ballTouchedTick = true;
            }
            
            internalState.lastTouchTick = gs.lastTickCount;
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

    // If no time has passed, return previous controls
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

    last_ticks = ticks;
    ticks += ticksElapsed;

    // Update game state with comprehensive 1:1 RocketSim tracking
    UpdateGameState(gameTickPacket, deltaTime, curTime);
    auto& localPlayer = gs.players[index];
    localPlayer.prevAction = controls;

    // Determine if we need new action from policy
    if (ticks >= params.tickSkip || ticks == -1) {
        ticks %= params.tickSkip;
        updateAction = true;
    }

    // Get new action from policy if needed
    if (updateAction) {
        updateAction = false;
        action = params.inferUnit->InferAction(localPlayer, gs, params.deterministic);
    }

    // Apply action delay
    if (last_ticks < params.actionDelay && ticks >= params.actionDelay) {
        controls = action;
    }

    // Convert to RLBot controller format
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
