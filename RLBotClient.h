#pragma once

#include <rlbot/bot.h>
#include <RLGymCPP/ObsBuilders/ObsBuilder.h>
#include <RLGymCPP/ActionParsers/ActionParser.h>
#include <GigaLearnCPP/Util/InferUnit.h>
#include <GigaLearnCPP/Util/ModelConfig.h>

#include <RLGymCPP/Framework.h>
#include <memory>
#include <map>

namespace RLBotConst {
    // Physics constants
    constexpr float GRAVITY_Z = -650.f;
    
    // Arena dimensions
    constexpr float ARENA_EXTENT_X = 4096.f;
    constexpr float ARENA_EXTENT_Y = 5120.f;
    constexpr float ARENA_HEIGHT = 2048.f;
    
    // Car physics
    constexpr float CAR_MASS_BT = 180.f;
    constexpr float CAR_MAX_SPEED = 2300.f;
    constexpr float CAR_MAX_ANG_SPEED = 5.5f;
    
    // Ball physics
    constexpr float BALL_MASS_BT = CAR_MASS_BT / 6.f;
    constexpr float BALL_RADIUS = 91.25f;
    constexpr float BALL_MAX_SPEED = 6000.f;
    constexpr float BALL_REST_Z = 93.15f;
    constexpr float BALL_MAX_ANG_SPEED = 6.f;
    constexpr float BALL_DRAG = 0.03f;
    constexpr float BALL_FRICTION = 0.35f;
    constexpr float BALL_RESTITUTION = 0.6f;
    
    // Boost constants
    constexpr float BOOST_MAX = 100.f;
    constexpr float BOOST_USED_PER_SECOND = BOOST_MAX / 3.f;
    constexpr float BOOST_MIN_TIME = 0.1f;
    constexpr float BOOST_ACCEL_GROUND = 2975.f / 3.f;
    constexpr float BOOST_ACCEL_AIR = 3175.f / 3.f;
    constexpr float BOOST_SPAWN_AMOUNT = BOOST_MAX / 3.f;
    
    // Supersonic constants
    constexpr float SUPERSONIC_START_SPEED = 2200.f;
    constexpr float SUPERSONIC_MAINTAIN_MIN_SPEED = 2100.f;
    constexpr float SUPERSONIC_MAINTAIN_MAX_TIME = 1.f;
    
    // Powerslide/Handbrake constants
    constexpr float POWERSLIDE_RISE_RATE = 5.f;
    constexpr float POWERSLIDE_FALL_RATE = 2.f;
    
    // Throttle constants
    constexpr float THROTTLE_AIR_ACCEL = 200.f / 3.f;
    constexpr float THROTTLE_TORQUE_AMOUNT = CAR_MASS_BT * 400.f;
    constexpr float BRAKE_TORQUE_AMOUNT = CAR_MASS_BT * (14.25f + (1.f / 3.f));
    constexpr float STOPPING_FORWARD_VEL = 25.f;
    constexpr float COASTING_BRAKE_FACTOR = 0.15f;
    
    // Jump constants
    constexpr float JUMP_ACCEL = 4375.f / 3.f;
    constexpr float JUMP_IMMEDIATE_FORCE = 875.f / 3.f;
    constexpr float JUMP_MIN_TIME = 0.025f;
    constexpr float JUMP_MAX_TIME = 0.2f;
    constexpr float JUMP_RESET_TIME_PAD = 1.f / 40.f;
    constexpr float DOUBLEJUMP_MAX_DELAY = 1.25f;
    
    // Flip/Dodge constants
    constexpr float FLIP_Z_DAMP_120 = 0.35f;
    constexpr float FLIP_Z_DAMP_START = 0.15f;
    constexpr float FLIP_Z_DAMP_END = 0.21f;
    constexpr float FLIP_TORQUE_TIME = 0.65f;
    constexpr float FLIP_TORQUE_MIN_TIME = 0.41f;
    constexpr float FLIP_PITCHLOCK_TIME = 1.f;
    constexpr float FLIP_PITCHLOCK_EXTRA_TIME = 0.3f;
    constexpr float FLIP_INITIAL_VEL_SCALE = 500.f;
    constexpr float FLIP_TORQUE_X = 260.f;
    constexpr float FLIP_TORQUE_Y = 224.f;
    constexpr float FLIP_FORWARD_IMPULSE_MAX_SPEED_SCALE = 1.f;
    constexpr float FLIP_SIDE_IMPULSE_MAX_SPEED_SCALE = 1.9f;
    constexpr float FLIP_BACKWARD_IMPULSE_MAX_SPEED_SCALE = 2.5f;
    constexpr float FLIP_BACKWARD_IMPULSE_SCALE_X = 16.f / 15.f;
    
    // Auto-flip constants
    constexpr float CAR_AUTOFLIP_IMPULSE = 200.f;
    constexpr float CAR_AUTOFLIP_TORQUE = 50.f;
    constexpr float CAR_AUTOFLIP_TIME = 0.4f;
    constexpr float CAR_AUTOFLIP_NORMZ_THRESH = 0.70710678118f; // sqrt(0.5)
    constexpr float CAR_AUTOFLIP_ROLL_THRESH = 2.8f;
    
    // Auto-roll constants
    constexpr float CAR_AUTOROLL_FORCE = 100.f;
    constexpr float CAR_AUTOROLL_TORQUE = 80.f;
    
    // Air control constants (Pitch, Yaw, Roll)
    constexpr float CAR_AIR_CONTROL_TORQUE_PITCH = 130.f;
    constexpr float CAR_AIR_CONTROL_TORQUE_YAW = 95.f;
    constexpr float CAR_AIR_CONTROL_TORQUE_ROLL = 400.f;
    constexpr float CAR_AIR_CONTROL_DAMPING_PITCH = 30.f;
    constexpr float CAR_AIR_CONTROL_DAMPING_YAW = 20.f;
    constexpr float CAR_AIR_CONTROL_DAMPING_ROLL = 50.f;
    
    // Demo/Bump constants
    constexpr float DEMO_RESPAWN_TIME = 3.f;
    constexpr float BUMP_COOLDOWN_TIME = 0.25f;
    constexpr float BUMP_MIN_FORWARD_DIST = 64.5f;
    
    // Spawn constants
    constexpr float CAR_SPAWN_REST_Z = 17.f;
    constexpr float CAR_RESPAWN_Z = 36.f;
    
    // Ball-car collision constants
    constexpr float BALL_CAR_EXTRA_IMPULSE_Z_SCALE = 0.35f;
    constexpr float BALL_CAR_EXTRA_IMPULSE_FORWARD_SCALE = 0.65f;
    constexpr float BALL_CAR_EXTRA_IMPULSE_MAXDELTAVEL_UU = 4600.f;
    
    // Collision physics
    constexpr float CAR_COLLISION_FRICTION = 0.3f;
    constexpr float CAR_COLLISION_RESTITUTION = 0.1f;
    constexpr float CARBALL_COLLISION_FRICTION = 2.0f;
    constexpr float CARBALL_COLLISION_RESTITUTION = 0.0f;
    constexpr float CARWORLD_COLLISION_FRICTION = 0.3f;
    constexpr float CARWORLD_COLLISION_RESTITUTION = 0.3f;
    constexpr float CARCAR_COLLISION_FRICTION = 0.09f;
    constexpr float CARCAR_COLLISION_RESTITUTION = 0.1f;
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
    int last_ticks = -1;

    RLGC::GameState gs;
    RLGC::GameState prevGs;

    struct PlayerInternalState {
        // Jump state tracking
        float jumpTime = 0;
        bool isJumping = false;
        bool jumpReleased = false;
        
        // Flip state tracking
        float flipTime = 0;
        bool isFlipping = false;
        bool hasFlipped = false;
        Vec flipRelTorque = Vec(0, 0, 0);
        
        // Air time tracking
        float airTime = 0;
        float airTimeSinceJump = 0;
        
        // Auto flip state tracking
        float autoFlipTimer = 0.f;
        float autoFlipTorqueScale = 0.f;
        bool isAutoFlipping = false;
        
        // Boost tracking
        float supersonicTime = 0;
        float timeSpentBoosting = 0;
        
        // Handbrake tracking (gradual analog value 0-1)
        float handbrakeVal = 0;
        
        // Wheel contact tracking
        // NOTE: RLBot only provides hasWheelContact boolean, not per wheel
        // We estimate all 4 wheels based on the single boolean (API limitation)
        bool wheelsWithContact[4] = {false, false, false, false};
        int numWheelsInContact = 0;
        
        // World contact tracking
        struct {
            bool hasContact = false;
            Vec contactNormal = Vec(0, 0, 1);
        } worldContact;
        
        // Car-to-car contact tracking
        struct {
            uint32_t otherCarID = 0;
            float cooldownTimer = 0;
        } carContact;
        
        // Ball hit info tracking
        struct {
            bool isValid = false;
            Vec relativePosOnBall = Vec(0, 0, 0);
            Vec ballPos = Vec(0, 0, 0);
            Vec extraHitVel = Vec(0, 0, 0);
            uint64_t tickCountWhenHit = 0;
            uint64_t tickCountWhenExtraImpulseApplied = 0;
        } ballHitInfo;
        
        // Demo tracking
        float demoRespawnTimer = 0;
        bool wasDemoedLastFrame = false;
        uint64_t demoTick = 0;
        
        // Ball touch tracking
        uint64_t lastTouchTick = 0;
        
        // Previous frame state for transition detection
        bool wasOnGroundLastFrame = false;
        bool wasInAirLastFrame = false;
        bool hadJumpedLastFrame = false;
        bool hadDoubleJumpedLastFrame = false;
        bool hadWheelContactLastFrame = false;
        
        // Flip reset tracking
        uint64_t lastFlipResetTick = 0;
        bool gotFlipResetThisFrame = false;
        
        inline bool HasFlipOrJump() const {
            return (!hasFlipped && airTimeSinceJump < RLBotConst::DOUBLEJUMP_MAX_DELAY);
        }
        
        inline bool HasFlipReset(bool isOnGround, bool hasJumped) const {
            return !isOnGround && HasFlipOrJump() && !hasJumped;
        }
        
        inline bool GotFlipReset(bool isOnGround, bool hasJumped) const {
            return !isOnGround && !hasJumped;
        }
    };
    
    std::map<int, PlayerInternalState> internalPlayerStates;
    int lastTeamScores[2] = {0, 0};

    RLBotBot(int _index, int _team, std::string _name, const RLBotParams& params);
    ~RLBotBot();

    rlbot::Controller GetOutput(rlbot::GameTickPacket gameTickPacket) override;

private:
    void UpdateGameState(rlbot::GameTickPacket& packet, float deltaTime, float curTime);
    void UpdatePlayerState(RLGC::Player& player, RLGC::Player* prevPlayer, 
                          PlayerInternalState& internalState, 
                          float deltaTime, bool isLocalPlayer);
    void UpdateBallHitInfo(RLGC::Player& player, PlayerInternalState& internalState, 
                          float curTime, const rlbot::flat::Touch* latestTouch);
};

namespace RLBotClient {
    void Run(const RLBotParams& params);
}
