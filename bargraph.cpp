/*
 * Copyright 2024, Fabio Tomat <f.t.public@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
*/
 
/*
MIT License

Copyright (c) 2024 Fabio Tomat (TmTFx)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Application.h>
#include <Looper.h>
#include <SerialPort.h>
#include <String.h>
#include <StringList.h>
#include <OS.h>
#include <fstream>
#include <vector>
#include <string>
#include <stdio.h>
#include <ctype.h>
#include <iostream>
static const uint32 SHOW_LABELS = 'SLAB';
static const uint32 REMOTE_QUIT_REQUEST = '_RQR';
static const uint32 SET_BRIGHTNESS = 'SETB';
static const uint32 SET_CONFIG = 'SCFG';
static const uint32 SERIAL_PATH = 'SPTH';
static const uint32 DAEMON_PING = 'PING';
static const uint32 SPECIAL_GRAPH = 'GRPH';

class BarGraphDaemon : public BApplication {
public:
	bool initialized = false;
	bool tmp = true;
	bool switch_labels = false;
	bool initial_backlight = true;
	bool change_brightness = false;
	std::string configuration;
	std::string quit_msg = "3";
	int counterSpecGraph = 2;

    BarGraphDaemon()
        : BApplication("application/x-vnd.BarGraphDaemon") {
			SetPulseRate(200000);
		}

    virtual void ReadyToRun() override {
		
        config = loadConfig();

        if (setupSerialPort() != B_OK) {
            fprintf(stderr, "Errore nell'inizializzazione della porta seriale.\n");
            PostMessage(B_QUIT_REQUESTED);
            return;
        }
		if (!config.showLabels) {
			serialPort.Write("2\n", 2);
			//fprintf(stdout, "Etichette disattivate!\n");
		}
		configureLabels(); //invia sempre la configurazione delle labels
		//readSerialData();
    }

    virtual bool QuitRequested() override {
		snooze(500000);
        printf("Demone in chiusura...\n");
        serialPort.Close();
        return true;
    }
	
	virtual void Pulse() override {
		std::vector<int> values = getSystemData();
		if (initialized){
			if (!config.showLabels && tmp ) {
				serialPort.Write("2\n", 2);
				tmp=false;
			}
			tmp=false;
			if (switch_labels) {
				serialPort.Write("2\n",2);
				config.showLabels = !config.showLabels;
				saveConfig(config);
				switch_labels = false;
			}
			configureLabels();
			if (initial_backlight){
				set_initial_backlight();
				initial_backlight=false;
			}
			if (specialgraph && config.numBars == 1){
				if (counterSpecGraph > 0) {
					counterSpecGraph-=1;
				} else {
					sendGraph(values[0]);
					counterSpecGraph=2;
				}
			} else {
				sendData(values);
				//readSerialData();
			}
		}
		initialized=true;
	}
	
	virtual void MessageReceived(BMessage* message) override {
        switch (message->what) {
            case SHOW_LABELS:
                switch_labels = true;
                break;
            case SET_BRIGHTNESS:
				int brightValue;
				if (message->FindInt32("bright", &brightValue) == B_OK) {
					if (brightValue >= 0 && brightValue <= 100) {
						config.brightness = brightValue;
						change_brightness = true;
						saveConfig(config);
					} else {
						fprintf(stderr, "Valore di brightness non valido: %d\n", brightValue);
					}
				} else {
					fprintf(stderr, "Impossibile trovare il valore 'bright' nel messaggio.\n");
				}
				break;
			case REMOTE_QUIT_REQUEST:
				{
				std::string command = "3";
				const char* foundString = nullptr;
				status_t status = message->FindString("text", &foundString);

				if (status == B_OK && foundString != nullptr) {
					fGoodByeMsg = std::string(foundString);
					command += " " + fGoodByeMsg;
				}
				command += "\n";
				serialPort.Write(command.c_str(), command.length());
				PostMessage(B_QUIT_REQUESTED);
				}
				break;
			case SET_CONFIG:
				{
					int numRecv;
					if (message->FindInt32("numBars", &numRecv) ==B_OK) {
						config.numBars= numRecv;
						BStringList etichette(numRecv);
						if (message->FindStrings("labels",&etichette) == B_OK){
							config.labels.clear();
							for (int i = 0; i < etichette.CountStrings(); i++) {
								config.labels.push_back(etichette.StringAt(i).String());
							}
							saveConfig(config);
						} else {
							return;
						}
						BString configString = "1 ";
						for (int i = 0; i < etichette.CountStrings(); i++) {
							configString << etichette.StringAt(i);
							if (i < etichette.CountStrings() -1) {
								configString << " ";
							}
						}
						serialPort.Write(configString.String(),configString.Length());
						snooze(200000);
						serialPort.Write("4\n",2);
						serialPort.Write("4\n",2);
						snooze(200000);
					}
				}
				break;
			case SERIAL_PATH:
				{
					const char* foundString = nullptr;
					status_t status = message->FindString("path", &foundString);
					if (status == B_OK && foundString != nullptr) {
						config.serialPort = std::string(foundString);
						saveConfig(config);
					}
				}
				break;
			case DAEMON_PING:
				{
					BMessenger messenger("application/x-vnd.BarGraph-Preflet");
					messenger.SendMessage(message);
				}
				break;
			case SPECIAL_GRAPH:
				{
					bool act_state;
					status_t status = message->FindBool("graphicMode", &act_state);
					if (status == B_OK) specialgraph = act_state;
					/*int index;
					status = message->FindInt32("index", &index);
					if (status == B_OK) specialgraph.index = index;*/
				}
				break;
            default:
                BApplication::MessageReceived(message);
                break;
        }
    }

private:
    struct Config {
        std::string serialPort;
        bool showLabels;
        int numBars;
		int brightness;
        std::vector<std::string> labels;
    };

	bool specialgraph;
    Config config;
    BSerialPort serialPort;
	std::string fGoodByeMsg = "Shutting Down";
	
    Config loadConfig() {
        Config config;
        std::ifstream configFile("/boot/system/settings/bargraph.conf");

        if (configFile.is_open()) {
            std::getline(configFile, config.serialPort);
            configFile >> config.showLabels >> config.numBars >> config.brightness;
            std::string label;
            while (configFile >> label) {
                config.labels.push_back(label);
            }			
        } else {
            // default configuration
            config.serialPort = "/dev/ports/usb0";
            config.showLabels = true;
            config.numBars = 8;
			config.brightness = 80;
            config.labels = {"1:", "2:", "3:", "4:","F1", "F2", "F3", "F4"};
            saveConfig(config);
        }

        return config;
    }

    void saveConfig(const Config& config) {
        std::ofstream configFile("/boot/system/settings/bargraph.conf");
        if (configFile.is_open()) {
            configFile << config.serialPort << "\n";
            configFile << config.showLabels << " " << config.numBars << " " << config.brightness << "\n";
            for (const auto& label : config.labels) {
                configFile << label << " ";
            }
        }
    }

    status_t setupSerialPort() {
		status_t status = serialPort.Open(config.serialPort.c_str());
		/*if (status != B_OK) {
			fprintf(stderr, "Errore nell'aprire la porta seriale %s: %s\n", config.serialPort.c_str(), strerror(status));
			return B_ERROR;
		}*/

        // Serial port settings as set on Arduino
        serialPort.SetDataRate(B_115200_BPS);
        serialPort.SetDataBits(B_DATA_BITS_8);
        serialPort.SetStopBits(B_STOP_BITS_1);
        serialPort.SetParityMode(B_NO_PARITY);
        serialPort.SetFlowControl(B_NOFLOW_CONTROL);//B_HARDWARE_CONTROL);
		serialPort.SetTimeout(100000);
        return B_OK;
    }
	
	std::vector<int> getSystemData() {
		system_info sysInfo;
		get_system_info(&sysInfo);

		static std::vector<bigtime_t> previousActiveTimes(sysInfo.cpu_count, 0);
		std::vector<cpu_info> cpuInfos(sysInfo.cpu_count);
		std::vector<int> values;

		get_cpu_info(0, sysInfo.cpu_count, cpuInfos.data());
		uint64 defaultFrequency = get_default_cpu_freq();
		
		std::vector<bigtime_t> currentActiveTimes(sysInfo.cpu_count);

		for (int i = 0; i < sysInfo.cpu_count; ++i) {
			currentActiveTimes[i] = cpuInfos[i].active_time;
		}

		for (const auto& label : config.labels) {
			if (isdigit(label[0])) {
				int cpuIndex = label[0] - '1';
				if (cpuIndex < sysInfo.cpu_count) {
					int load = (currentActiveTimes[cpuIndex] - previousActiveTimes[cpuIndex]) / 2000;
					values.push_back(load);
				} else {
					values.push_back(0);
				}
			} else if (label[0] == 'M') {
				int memoryUsage = ((sysInfo.used_pages * 100) / sysInfo.max_pages);
				values.push_back(memoryUsage);
			} else if (label[0] == 'F') {
				int cpuIndex = label[1] - '1';
				if (cpuIndex < sysInfo.cpu_count) {
					uint64 currentFrequency = cpuInfos[cpuIndex].current_frequency;
					int frequencyPercent;
					if (currentFrequency <= defaultFrequency) {
						frequencyPercent = (currentFrequency * 50) / defaultFrequency;
					} else if (currentFrequency <= 2 * defaultFrequency) {
						frequencyPercent = 50 + ((currentFrequency - defaultFrequency) * 50) / defaultFrequency;
					} else {
						frequencyPercent = 100;
					}
					values.push_back(frequencyPercent);
				} else {
					values.push_back(0);
				}
			}
		}
		
		for (int i = 0; i < sysInfo.cpu_count; ++i) {
			previousActiveTimes[i] = currentActiveTimes[i];
		}

		return values;
	}
	
	std::string serialBuffer;
	void readSerialData() {
		char buffer[256];
		ssize_t bytesRead = serialPort.Read(buffer, sizeof(buffer) - 1);

		if (bytesRead > 0) {
			buffer[bytesRead] = '\0'; //Add Line-End
			serialBuffer += buffer;
			size_t pos;
			while ((pos = serialBuffer.find('\n')) != std::string::npos) {
				std::string message = serialBuffer.substr(0, pos);
				serialBuffer.erase(0,pos+1);
				fprintf(stdout, "Messaggio completo: %s\n", message.c_str());
				if (message == "REBOOT") {
					fprintf(stdout, "Arduino ha segnalato un reboot.\n");
				}
			}
		} else if (bytesRead < 0) {
			fprintf(stderr, "Errore nella lettura dalla seriale.\n");
		}
	}
	void sendGraph(int& value) {
		std::string data = "5 "+ std::to_string(value);
		if (change_brightness){
			data += " " + std::to_string(config.brightness);
			change_brightness = false;
		}
        data += "\n";
		serialPort.Write(data.c_str(), data.length());
	}

    void sendData(const std::vector<int>& values) {
        std::string data = "0";
        for (size_t i = 0; i < values.size(); i++) {
            data += " " + std::to_string(values[i]);
        }
		if (change_brightness){
			data += " " + std::to_string(config.brightness);
			change_brightness = false;
		}
        data += "\n";
		
        serialPort.Write(data.c_str(), data.length());
		//readSerialData();
    }

    void configureLabels() {
        std::string labelConfig = "1";
        for (int i = 0; i < config.numBars && i < config.labels.size(); i++) {
            labelConfig += " " + config.labels[i];
        }
        labelConfig += "\n";
        serialPort.Write(labelConfig.c_str(), labelConfig.length());
		//readSerialData();
		snooze(150000);
    }
	void set_initial_backlight(){
		std::string labelConfig = "0";
        for (int i = 0; i < config.numBars; i++) {
            labelConfig += " 0";
        }
		labelConfig += " " + std::to_string(config.brightness);
		labelConfig += "\n";
        serialPort.Write(labelConfig.c_str(), labelConfig.length());
		snooze(150000);
	}
	

	uint64 get_default_cpu_freq(void) {
	  uint32 topologyNodeCount = 0;
	  cpu_topology_node_info* topology = NULL;
	  get_cpu_topology_info(NULL, &topologyNodeCount);
	  if (topologyNodeCount != 0)
		topology = (cpu_topology_node_info*)calloc(topologyNodeCount, sizeof(cpu_topology_node_info));
	  get_cpu_topology_info(topology, &topologyNodeCount);

	  uint64 cpuFrequency = 0;
	  for (uint32 i = 0; i < topologyNodeCount; i++) {
		if (topology[i].type == B_TOPOLOGY_CORE) {
			cpuFrequency = topology[i].data.core.default_frequency;
			break;
		}
	  }
	  free(topology);
	  return cpuFrequency;
	}

};

int main() {
    BarGraphDaemon app;
    app.Run();
    return 0;
}
