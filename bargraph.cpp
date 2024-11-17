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
#include <OS.h>
#include <fstream>
#include <vector>
#include <string>
#include <stdio.h>
#include <ctype.h>
#include <iostream>
//FIX impostare luminosità predefinita
const uint32 SHOW_LABELS = 'SLAB';
const uint32 REMOTE_QUIT_REQUEST = '_RQR';
const uint32 SET_BRIGHTNESS = 'SETB';
const uint32 SET_CONFIG = 'SCFG';

class BarGraphDaemon : public BApplication {
public:
	bool initialized = false;
	int tmp = 1;
	bool switch_labels = false;
	bool change_brightness = false;
	//int8 brightness;
	std::string configuration;
	std::string quit_msg = "3";

    BarGraphDaemon()
        : BApplication("application/x-vnd.BarGraphDaemon") {
			SetPulseRate(150000);
		}

    virtual void ReadyToRun() override {
        // Carica la configurazione
        config = loadConfig();

        // Configura la porta seriale usando BSerialPort
        if (setupSerialPort() != B_OK) {
            fprintf(stderr, "Errore nell'inizializzazione della porta seriale.\n");
            PostMessage(B_QUIT_REQUESTED);  // Termina il demone in caso di errore
            return;
        }
		if (!config.showLabels) {
			serialPort.Write("2\n", 2);  // Invia "2" per disattivare le etichette
			fprintf(stdout, "Etichette disattivate!\n");
		}
		configureLabels(); //invia sempre la configurazione delle labels
		//readSerialData();
    }

    virtual bool QuitRequested() override {
        // Azioni di chiusura prima dello spegnimento
		serialPort.Write("3 Addio e grazie per il pesce\n", 30);
		snooze(500000);
        printf("Demone in chiusura...\n");
        serialPort.Close();
        return true;
    }
	
	virtual void Pulse() override {
		std::vector<int> values = getSystemData();
		if (initialized){
			if (!config.showLabels && tmp > 0) {
				serialPort.Write("2\n", 2);
				tmp--;
			}
			if (switch_labels) {
				serialPort.Write("2\n",2);
				config.showLabels = !config.showLabels;
				switch_labels = false;
			}
			configureLabels();
			sendData(values);
			//readSerialData();
		}
		//uint32 pass = 0;
		//PostMessage(pass);
		initialized=true;
	}
	
	virtual void MessageReceived(BMessage* message) override {
        // Gestione dei messaggi ricevuti
        switch (message->what) {
            case SHOW_LABELS:
                switch_labels = true;
                break;
            case SET_BRIGHTNESS:
				int8_t brightValue;
				if (message->FindInt8("bright", &brightValue) == B_OK) {
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
            default:
                // Passa il messaggio alla classe base per gestione predefinita
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

    Config config;
    BSerialPort serialPort;
	
    Config loadConfig() {
        Config config;
        std::ifstream configFile("/boot/system/settings/bargraph.conf");

        if (configFile.is_open()) {
            std::getline(configFile, config.serialPort);
			//fprintf(stdout, "serial port: %s\n", config.serialPort.c_str());
            configFile >> config.showLabels >> config.numBars >> config.brightness;
            std::string label;
            while (configFile >> label) {
                config.labels.push_back(label);
            }			
        } else {
            // Configurazione di default
            config.serialPort = "/dev/ports/usb0";
            config.showLabels = true;
            config.numBars = 8;
			config.brightness = 80;
            config.labels = {"1:", "2:", "3:", "4:","F1", "F2", "F3", "F4"}; //implementata M: e F1,F2,Fx
            saveConfig(config);  // Salva la configurazione di default
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
        // Apre la porta seriale
        /*if (serialPort.Open(config.serialPort.c_str()) != B_OK) {
            fprintf(stderr, "Errore nell'aprire la porta seriale.\n");
            return B_ERROR;
        }*/
		status_t status = serialPort.Open(config.serialPort.c_str());
		/*if (status != B_OK) {
			fprintf(stderr, "Errore nell'aprire la porta seriale %s: %s\n", config.serialPort.c_str(), strerror(status));
			return B_ERROR;
		}*/

        // Configura le impostazioni della porta seriale
        serialPort.SetDataRate(B_115200_BPS);
        serialPort.SetDataBits(B_DATA_BITS_8);
        serialPort.SetStopBits(B_STOP_BITS_1);
        serialPort.SetParityMode(B_NO_PARITY);
        serialPort.SetFlowControl(B_NOFLOW_CONTROL);//B_HARDWARE_CONTROL);//
		serialPort.SetTimeout(100000);
        return B_OK;
    }
	
	std::vector<int> getSystemData() {
		system_info sysInfo;
		get_system_info(&sysInfo);

		static std::vector<bigtime_t> previousActiveTimes(sysInfo.cpu_count, 0);
		std::vector<cpu_info> cpuInfos(sysInfo.cpu_count);
		std::vector<int> values;

		// Ottiene i dati della CPU
		get_cpu_info(0, sysInfo.cpu_count, cpuInfos.data());
		uint64 defaultFrequency = get_default_cpu_freq();
		
		std::vector<bigtime_t> currentActiveTimes(sysInfo.cpu_count);

		// Passaggio 1: Recupera i tempi attivi senza aggiornare previousActiveTimes
		for (int i = 0; i < sysInfo.cpu_count; ++i) {
			currentActiveTimes[i] = cpuInfos[i].active_time;
		}

		// Passaggio 2: Calcola il carico, memorizza i valori e prepara per aggiornare previousActiveTimes
		for (const auto& label : config.labels) {
			if (isdigit(label[0])) {
				int cpuIndex = label[0] - '1';  // Converte il carattere in numero
				if (cpuIndex < sysInfo.cpu_count) {
					// Calcola il carico come differenza dei tempi attivi
					int load = (currentActiveTimes[cpuIndex] - previousActiveTimes[cpuIndex]) / 1000;
					values.push_back(load);
				} else {
					values.push_back(0);  // Inserisce 0 se la CPU non esiste
				}
			} else if (label[0] == 'M') {
				// Percentuale di memoria utilizzata
				int memoryUsage = ((sysInfo.used_pages * 100) / sysInfo.max_pages);
				values.push_back(memoryUsage);
			} else if (label[0] == 'F') {
				int cpuIndex = label[1] - '1';  // "F1" -> CPU 0, "F2" -> CPU 1, ecc.
				if (cpuIndex < sysInfo.cpu_count) {
					uint64 currentFrequency = cpuInfos[cpuIndex].current_frequency;
					int frequencyPercent;
					if (currentFrequency <= defaultFrequency) {
						frequencyPercent = (currentFrequency * 50) / defaultFrequency;
					} else if (currentFrequency <= 2 * defaultFrequency) {
						frequencyPercent = 50 + ((currentFrequency - defaultFrequency) * 50) / defaultFrequency;
					} else {
						frequencyPercent = 100;  // Limita al 100% se oltre il doppio della frequenza predefinita
					}
					values.push_back(frequencyPercent);
				} else {
					values.push_back(0); // CPU non disponibile
				}
			}
		}
		// Passaggio 3: Aggiorna previousActiveTimes alla fine
		for (int i = 0; i < sysInfo.cpu_count; ++i) {
			previousActiveTimes[i] = currentActiveTimes[i];
		}

		return values;
	}
	
	std::string serialBuffer;
	void readSerialData() {
		char buffer[256];  // Buffer per i dati in arrivo
		ssize_t bytesRead = serialPort.Read(buffer, sizeof(buffer) - 1);

		if (bytesRead > 0) {
			buffer[bytesRead] = '\0';  // Aggiungi terminatore di stringa
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
			/*
			std::string data(buffer);

			// Analizza o stampa i dati ricevuti
			fprintf(stdout, "Ricevuto dalla seriale: %s\n", data.c_str());

			// Puoi gestire risposte specifiche qui, ad esempio:
			if (data == "REBOOT\n") {
				fprintf(stdout, "Arduino ha segnalato un reboot.\n");
				// Esegui eventuali azioni necessarie
			}*/
		} else if (bytesRead < 0) {
			fprintf(stderr, "Errore nella lettura dalla seriale.\n");
		}
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
		
		fprintf(stdout, "Output su seriale: %s\n", data.c_str());
        serialPort.Write(data.c_str(), data.length());
		//snooze(100000);
		//readSerialData();
    }

    void configureLabels() {
        std::string labelConfig = "1";
        for (int i = 0; i < config.numBars && i < config.labels.size(); i++) {
            labelConfig += " " + config.labels[i];
        }
        labelConfig += "\n";
		//fprintf(stdout, "Configurazione su seriale: %s\n", labelConfig.c_str());
        serialPort.Write(labelConfig.c_str(), labelConfig.length());
		//readSerialData();
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
	/*
	  int target, frac, delta;
	  int freqs[] = { 100, 50, 25, 75, 33, 67, 20, 40, 60, 80, 10, 30, 70, 90 };
	  uint x;
	  target = cpuFrequency / 1000000;
	  frac = target % 100;
	  delta = -frac;

	  for (x = 0; x < sizeof(freqs) / sizeof(freqs[0]); x++) {
		int ndelta = freqs[x] - frac;
		if (abs(ndelta) < abs(delta))
		  delta = ndelta;
	  }
	  return target + delta;*/
	  return cpuFrequency;
	}

};

int main() {
    BarGraphDaemon app;
    app.Run();  // Esegue il loop del demone
    return 0;
}
