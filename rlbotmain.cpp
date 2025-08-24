#include "RLBotClient.h"
#include "RLGymCPP/ActionParsers/DefaultAction.h"
#include "RLGymCPP/ObsBuilders/AdvancedObs.h"
#include "GigaLearnCPP/Util/InferUnit.h"
#include "GigaLearnCPP/Util/ModelConfig.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

using namespace GGL;
using namespace RLGC;

// Match params with what you trained with
void rlbotparameters(RLBotParams& params) {
    params.port = 32257;
    params.tickSkip = 8;
    params.actionDelay = 7;
    params.deterministic = true;
    params.obsSize = 109;
    params.useGPU = true;

    params.sharedHeadConfig.layerSizes = {};
    params.sharedHeadConfig.activationType = ModelActivationType::RELU;
    params.sharedHeadConfig.addOutputLayer = false;

    params.policyConfig.layerSizes = {1024, 1024, 1024, 1024};
    params.policyConfig.activationType = ModelActivationType::RELU;
    params.policyConfig.addLayerNorm = true;
    params.policyConfig.addOutputLayer = true;
}

// Finds latest checkpoint in exe folder
std::filesystem::path find_latest_checkpoint_path(const std::filesystem::path& checkpointsDir) {
    std::filesystem::path latestCheckpointPath;
    long long latestTimestamp = -1;

    if (!std::filesystem::exists(checkpointsDir) || !std::filesystem::is_directory(checkpointsDir)) {
        std::cerr << "Error: 'checkpoints' directory not found at: " << checkpointsDir << std::endl;
        return {};
    }

    for (const auto& entry : std::filesystem::directory_iterator(checkpointsDir)) {
        if (entry.is_directory()) {
            try {
                long long ts = std::stoll(entry.path().filename().string());
                if (ts > latestTimestamp) {
                    latestTimestamp = ts;
                    latestCheckpointPath = entry.path();
                }
            } catch (...) {
                continue;
            }
        }
    }

    return latestCheckpointPath;
}

int main(int argc, char* argv[]) {
    if (argc == 0) {
        std::cerr << "Error: Could not determine executable path." << std::endl;
        return 1;
    }

    RLBotParams params;
    rlbotparameters(params);

    std::filesystem::path checkpointPath;
    
    //To use a specific checkpoint uncomment the line below and set the path to POLICY.lt
    // checkpointPath = "C:/Users/FurryLover69/Downloads/GigaLearnCPP/GigaLearnCPP/build/Release/checkpoints/14594451456/POLICY.lt";

    if (!checkpointPath.empty()) {
        std::cout << "Loading policy from hardcoded path: " << checkpointPath << std::endl;
    } else {
        std::filesystem::path exeParentPath = std::filesystem::path(argv[0]).parent_path();
        std::filesystem::path checkpointsDir = exeParentPath / "checkpoints";
        checkpointPath = find_latest_checkpoint_path(checkpointsDir);
        if (!checkpointPath.empty()) {
            std::cout << "Automatically loading LATEST policy from: " << checkpointPath << std::endl;
        }
    }

    if (checkpointPath.empty() || !std::filesystem::exists(checkpointPath)) {
        std::cerr << "Error: No valid checkpoint path found or provided." << std::endl;
        return 1;
    }
    // Replace with your obs and parser names
    auto obsBuilder = std::make_unique<AdvancedObs>();
    auto actionParser = std::make_unique<DefaultAction>();
    auto inferUnit = std::make_unique<GGL::InferUnit>(
        obsBuilder.get(),
        params.obsSize,
        actionParser.get(),
        params.sharedHeadConfig,
        params.policyConfig,
        checkpointPath,
        params.useGPU
    );

    std::cout << "Starting in RLBot Mode...\n";
    params.obsBuilder = obsBuilder.get();
    params.actionParser = actionParser.get();
    params.inferUnit = inferUnit.get();
    RLBotClient::Run(params);

    return 0;
}
