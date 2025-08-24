#include "RLBotClient.h"

#include <rlbot/platform.h>
#include <rlbot/botmanager.h>

using namespace RLGC;
using namespace GGL;

// Global variable so that we can pass params to the bot factory
// TODO: This is a lame solution
RLBotParams g_RLBotParams = {};

rlbot::Bot* BotFactory(int index, int team, std::string name) {
	return new RLBotBot(index, team, name, g_RLBotParams);
}

RLBotBot::RLBotBot(int _index, int _team, std::string _name, const RLBotParams& params) 
	: rlbot::Bot(_index, _team, _name), params(params) {

	RG_LOG("Created RLBot bot: index " << _index << ", name: " << name << "...");
}

RLBotBot::~RLBotBot() {
	delete g_RLBotParams.inferUnit;
}

Vec ToVec(const rlbot::flat::Vector3* rlbotVec) {
	return Vec(rlbotVec->x(), rlbotVec->y(), rlbotVec->z());
}

PhysState ToPhysObj(const rlbot::flat::Physics* phys) {
	PhysState obj = {};
	obj.pos = ToVec(phys->location());

	Angle ang = Angle(phys->rotation()->yaw(), phys->rotation()->pitch(), phys->rotation()->roll());
	obj.rotMat = ang.ToRotMat();

	obj.vel = ToVec(phys->velocity());
	obj.angVel = ToVec(phys->angularVelocity());

	return obj;
}

Player ToPlayer(const rlbot::flat::PlayerInfo* playerInfo) {
	Player pd = {};
	
	static_cast<PhysState&>(pd) = ToPhysObj(playerInfo->physics());

	pd.carId = playerInfo->spawnId();

	pd.team = (Team)playerInfo->team();

	pd.boost = playerInfo->boost();
	pd.isOnGround = playerInfo->hasWheelContact();
	pd.hasJumped = playerInfo->jumped();
	pd.hasDoubleJumped = playerInfo->doubleJumped();
	pd.isDemoed = playerInfo->isDemolished();

	return pd;
}

GameState ToGameState(rlbot::GameTickPacket& gameTickPacket) {
	GameState gs = {};

	auto players = gameTickPacket->players();
	for (int i = 0; i < players->size(); i++)
		gs.players.push_back(ToPlayer(players->Get(i)));

	static_cast<PhysState&>(gs.ball) = ToPhysObj(gameTickPacket->ball()->physics());

	auto boostPadStates = gameTickPacket->boostPadStates();
	if (boostPadStates->size() != CommonValues::BOOST_LOCATIONS_AMOUNT) {
		if (rand() % 20 == 0) {
			RG_LOG(
				"RLBotClient ToGameState(): Bad boost pad amount, expected " << CommonValues::BOOST_LOCATIONS_AMOUNT << " but got " << boostPadStates->size()
			);
		}

		std::fill(gs.boostPads.begin(), gs.boostPads.end(), 1);
	} else {
		for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			gs.boostPads[i] = boostPadStates->Get(i)->isActive();
			gs.boostPadsInv[CommonValues::BOOST_LOCATIONS_AMOUNT - i - 1] = gs.boostPads[i];

			gs.boostPadTimers[i] = boostPadStates->Get(i)->timer();
			gs.boostPadTimersInv[CommonValues::BOOST_LOCATIONS_AMOUNT - i - 1] = gs.boostPadTimers[i];
		}
	}

	return gs;
}

rlbot::Controller RLBotBot::GetOutput(rlbot::GameTickPacket gameTickPacket) {

	float curTime = gameTickPacket->gameInfo()->secondsElapsed();
	float deltaTime = curTime - prevTime;
	prevTime = curTime;

	int ticksElapsed = roundf(deltaTime * 120);
	ticks += ticksElapsed;

	GameState gs = ToGameState(gameTickPacket);
	auto& localPlayer = gs.players[index];
	localPlayer.prevAction = controls;

	if (updateAction) {
		updateAction = false;
		action = params.inferUnit->InferAction(localPlayer, gs, params.deterministic); //deterministic is last bool originally true
	}

	if (ticks >= (params.actionDelay - 1) || ticks == -1) {
		controls = action;
	}

	if (ticks >= params.tickSkip || ticks == -1) {
		ticks = 0;
		updateAction = true;
	}

	auto rc = rlbot::Controller();
	{
		rc.throttle = controls.throttle;
		rc.steer = controls.steer;

		rc.pitch = controls.pitch;
		rc.yaw = controls.yaw;
		rc.roll = controls.roll;

		rc.boost = controls.boost;
		rc.jump = controls.jump;
		rc.handbrake = controls.handbrake;
	}

	return rc;
}

void RLBotClient::Run(const RLBotParams& params) {
	g_RLBotParams = params;

	rlbot::platform::SetWorkingDirectory(
		rlbot::platform::GetExecutableDirectory()
	);

	rlbot::BotManager botManager(BotFactory);
	botManager.StartBotServer(params.port);
}