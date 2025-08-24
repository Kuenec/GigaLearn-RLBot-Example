# GigaLearn for RLBot Integration

This repository provides the necessary files to run a GigaLearnCPP agent within the RLBot framework for Rocket League.

The main purpose is to bridge the GigaLearn inference model with the RLBot client, allowing your trained bot to play in standard RLBot matches and events.

## Prerequisites

Before you begin, you should have:
* A working, compiled [GigaLearnCPP](https://github.com/ZealanL/GigaLearnCPP-Leak) project.
* RLBot installed and running on your machine.

## Setup Instructions

### Step 1: Replace Core Project Files

You will need to replace files in your `GigaLearnCPP` directory to integrate the RLBot logic.

* **Copy `CMakeLists.txt`:**
    * **Source:** `CMakeLists.txt` from this repository.
    * **Destination:** replace `GigaLearnCPP\CMakeLists.txt`.

* **Copy Source Files:**
    * **Source:** `RLBotClient.h`, `RLBotClient.cpp`, and `rlbotmain.cpp` from this repository.
    * **Destination:** Place these in `GigaLearnCPP\src\`, replacing any existing files.

### Step 2: Configure the RLBot Agent

* **Add the Agent Config:**
    * **Source:** `CppPythonAgent.cfg` from this repository.
    * **Destination:** `GigaLearnCPP\rlbot\CppPythonAgent.cfg`

* **Set Your Executable Path:**
    You **must** edit `CppPythonAgent.cfg` to point to your compiled bot's executable.
    1.  Open `GigaLearnCPP\rlbot\CppPythonAgent.cfg`.
    2.  Find the line `cpp_executable_path = `.
    3.  Add the full path to your compiled `.exe` file.

    **Example:**
    ```ini
    [Locations]
    # Path to the C++ executable
    cpp_executable_path = C:\path\to\your\GigaLearnCPP\build\Release\GigaLearnCPP.exe
    ```

## How to Run

1.  After replacing the files, **re-compile your GigaLearnCPP project** and use regular build tool (ex: Visual Studio).
2.  Ensure you have a `checkpoints` folder with your trained model inside the same directory as your final executable (e.g., `GigaLearnCPP\build\Release\`). The bot will automatically load the latest model.
3.  Add the `GigaLearnCPP\rlbot\CppPythonAgent.cfg` to rlbot.
4.  Launch a match from the RLBot GUI, and your bot should appear.

## Optional: Using a Specific Checkpoint

By default, the bot loads the latest checkpoint. To force it to use a specific model, you can hardcode the path.

1.  Open `GigaLearnCPP\src\rlbotmain.cpp`.
2.  Find the commented-out line that begins with `// checkpointPath =`.
3.  Uncomment it and replace the path with the full path to your `POLICY.lt` file.

**Example:**
```cpp
// On Windows, you must use double backslashes (\\) or forward slashes (/) in the path.
checkpointPath = "C:\\Users\\YourUser\\GigaLearnCPP\\build\\Release\\checkpoints\\14594451456\\POLICY.lt";
```

## Troubleshooting

* **Bot doesn't appear in RLBot?**
    * Double-check that the `cpp_executable_path` in `CppPythonAgent.cfg` is absolutely correct and points to the `.exe` file you compiled.

Here’s a more concise and well-formatted version for your GitHub README:

### Notes

* **Ball prediction:** Not supported. If your observation uses ball prediction, modify `rlbotmain.cpp` and pass a RocketSimArena to it. This is straightforward.

* **Padded observations:** Likely supported. To use, change:

```cpp
auto obsBuilder = std::make_unique<AdvancedObs>();
```

to

```cpp
auto obsBuilder = std::make_unique<YourObsNamePadded>(3);
```

Replace `3` with the value used during training.


