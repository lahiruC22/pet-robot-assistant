# ESP32 Pet Robot Assistant

[Introduction to be written here]

## Project Structure
The project is organized into the following top-level directories to keep hardware, software, and documentation distinctly seperated but versioned together.

``` bash
pet-robot-assistant/
├── .gitignore
├── README.md
├── docs/
├── firmware/
├── hardware/
└── learning/
```

- **firmware/** : A-self contained PlatformIO project for the ESP32-S3. This is the main application code. See the`firmware/README.md` for details on building and flashing.

- **hardware/** : Contains all physical design files, including 3D models for enclosures (`cad/`) and schematics for component connections (`wiring-diagrams/`)

- **docs/** : High-level project documentation including the project roadmap and overview.

## Features

[features to be included in here]

## Installation
To get started with this project, you will need to setup the development environment.

1. Clone the repository:  

``` bash
git clone https://github.com/lahiruC22/pet-robot-assistant.git

cd pet-robot-assistant
```

2. Install Visual Studio Code:  
Download and install VS Code from the [official website.]('https://code.visualstudio.com/downloadhttps://code.visualstudio.com/download')

3. Install PlatformIO IDE Extension:  
Open VS Code, go to the Extensions view (Ctrl+Shift+X), and search for and install `PlatformIO IDE.` Please refer to the [PlatformIO Installation Documentation]('https://docs.platformio.org/en/latest/')

4. Install the corresponding drivers:  
To flash the firmware to the ESP32 corresponding USB drivers needed. This is depend on the development board.
[Documentation enhancement needed in here.]

5. Open the Firmware Project:  
In VS Code, go to `File > Open Folder ...` and select the **firmware/** directory inside the cloned repository.**Do not open the top-level folder.** VS Code will automatically recognize it as a PlatformIO project.

6. Configure Credentials:  
- Inside the `firmware/src/` directory, make a `config.h` file.
- Configure that file to take your Wi-Fi credentials and API keys. [Documentation enhancement to be done in here]

7. Install Dependencies:  
PlatformIO will prompt you to install any required libraries when you first open the project or attempt to build it. Click "Yes" to allow it to install them automatically.

## Usage
You can run the project on either a physical device or a simulator.

### 1. Running on WokWi Simulator
[Proper Instructions to be written]

### 2. Running on Physical Hardware
[Proper Instructions to be written]

## Contributing
[Contributing guidelines.md]

## License
This project is licensed under the GPL-3.0 license. See the `LICENSE` file for more details.