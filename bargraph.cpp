#include <Application.h>
#include <Looper.h>
#include <SerialPort.h>
#include <OS.h>
#include <fstream>
#include <vector>
#include <string>
#include <stdio.h>
#include <ctype.h>
//TODO Fix default sends only 3 bars @.@
//FIX not reading config file
class BarGraphDaemon : public BApplication {
public:
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
		configureLabels(); //invia sempre la configurazione delle labels
        // Invia configurazione delle etichette ad Arduino se necessario
		if (!config.showLabels) {
			serialPort.Write("2\n", 2);  // Invia "2" per disattivare le etichette
		}
    }

    virtual bool QuitRequested() override {
        // Azioni di chiusura prima dello spegnimento
        printf("Demone in chiusura...\n");
        serialPort.Close();
        return true;
    }
	
	virtual void Pulse() override {
		std::vector<int> values = getSystemData();
		sendData(values);
	}
	
    /*void RunDaemonLoop() {
        while (!QuitRequested()) {
            // Recupera i dati di sistema in base alla configurazione
            std::vector<int> values = getSystemData();
            // Invia i dati via seriale
            sendData(values);
            snooze(250000);  // Pausa di 250 ms
        }
    }*/

private:
    struct Config {
        std::string serialPort;
        bool showLabels;
        int numBars;
        std::vector<std::string> labels;
    };

    Config config;
    BSerialPort serialPort;

    Config loadConfig() {
        Config config;
        std::ifstream configFile("/boot/home/config/settings/tuo_addon.conf");

        if (configFile.is_open()) {
            std::getline(configFile, config.serialPort);
            configFile >> config.showLabels >> config.numBars;
            std::string label;
            while (configFile >> label) {
                config.labels.push_back(label);
            }
        } else {
            // Configurazione di default
            config.serialPort = "/dev/ports/usb0"; //TODO FixMe
            config.showLabels = true;
            config.numBars = 4;
            config.labels = {"1:", "2:", "3:", "4:"}; //implementata M: e F1,F2,Fx
            saveConfig(config);  // Salva la configurazione di default
        }

        return config;
    }

    void saveConfig(const Config& config) {
        std::ofstream configFile("/boot/system/settings/bargraph.conf");
        if (configFile.is_open()) {
            configFile << config.serialPort << "\n";
            configFile << config.showLabels << " " << config.numBars << "\n";
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
        serialPort.SetFlowControl(B_HARDWARE_CONTROL);//B_NOFLOW_CONTROL);

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
		for (const auto& label : config.labels) {
			if (isdigit(label[0])) {
				// Se il primo carattere della label è un numero, è il carico di una CPU specifica
				int cpuIndex = label[0] - '0';  // Converte il carattere in numero
				if (cpuIndex < sysInfo.cpu_count) {
					
					// Calcola il carico della CPU come differenza dei tempi attivi da PicoLCD
					int load = (cpuInfos[cpuIndex].active_time - previousActiveTimes[cpuIndex]) / 2000;
					values.push_back(load);
					previousActiveTimes[cpuIndex] = cpuInfos[cpuIndex].active_time;  // Aggiorna il tempo attivo
				} else {
					values.push_back(0);  // Inserisce 0 se la CPU non esiste
				}
			} else if (label[0] == 'M') {
				// Percentuale di memoria utilizzata
				//int memUsage = static_cast<int>(100 - (sysInfo.used_pages * 100 / sysInfo.max_pages));
				//values.push_back(memUsage);
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
					/*cpu_info cpuInfo;
					get_cpu_info(cpuIndex, 1, &cpuInfo);

					// Calcolo della frequenza come percentuale relativa alla frequenza predefinita
					 cpuInfo.default_frequency;
					uint64 currentFrequency = cpuInfo.current_frequency;

					int frequencyPercent;
					if (currentFrequency <= defaultFrequency) {
						frequencyPercent = (currentFrequency * 50) / defaultFrequency;
					} else if (currentFrequency <= 2 * defaultFrequency) {
						frequencyPercent = 50 + ((currentFrequency - defaultFrequency) * 50) / defaultFrequency;
					} else {
						frequencyPercent = 100;  // Limita al 100% se oltre il doppio della frequenza predefinita
					}
					
					values.push_back(frequencyPercent);*/
				} else {
					values.push_back(0); // CPU non disponibile
				}
				/*// Frequenza della CPU
				int cpuIndex = label[1] - '0';  // Usa il secondo carattere per indicare la CPU specifica
				if (cpuIndex < sysInfo.cpu_count) {
					values.push_back(cpuInfos[cpuIndex].cpu_clock_speed / 1000000);  // Frequenza in MHz
				} else {
					values.push_back(0);  // Inserisce 0 se la CPU non esiste
				}*/
			} 
			// Gestione di altre metriche può essere aggiunta qui, come la temperatura.
		}
		return values;
/*        std::vector<int> values;
        system_info sysInfo;
        get_system_info(&sysInfo);


        int maxMemory = sysInfo.max_pages * B_PAGE_SIZE / (1024 * 1024);  // Memoria totale in MB
        for (const auto& label : config.labels) {
            if (isdigit(label[0])) {
                int cpuIndex = label[0] - '1';  // Indice CPU
                if (cpuIndex >= 0 && cpuIndex < sysInfo.cpu_count) {
                    // Simula recupero carico CPU (qui usiamo un valore fittizio)
                    values.push_back(sysInfo.cpu_infos[cpuIndex].active_time);
                }
            } else if (label == "M:") {
                // Percentuale di memoria usata
                int usedMemory = maxMemory - sysInfo.free_memory / (1024 * 1024);
                int memUsage = static_cast<int>((usedMemory * 100) / maxMemory);
                values.push_back(memUsage);
            } else if (label[0] == 'F') {
                // Simula la frequenza CPU in percentuale rispetto a maxFreq (sostituire con valore reale)
                int maxFreq = 3000;  // Frequenza massima fittizia in MHz
                int currentFreq = 2000;  // Frequenza corrente fittizia in MHz
                int freqUsage = static_cast<int>((currentFreq * 100) / maxFreq);
                values.push_back(freqUsage);
            }
        }

        return values;*/
    }
	

    void sendData(const std::vector<int>& values) {
        std::string data = "0";
        for (size_t i = 0; i < values.size(); i++) {
            //if (config.showLabels) data += config.labels[i] + " ";
            data += " " + std::to_string(values[i]);
        }
        data += "\n";
        serialPort.Write(data.c_str(), data.length());
    }

    void configureLabels() {
        std::string labelConfig = "1";
        for (int i = 0; i < config.numBars && i < config.labels.size(); i++) {
            labelConfig += " " + config.labels[i];
        }
        labelConfig += "\n";
        serialPort.Write(labelConfig.c_str(), labelConfig.length());
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
    app.ReadyToRun();  // Inizializza il demone
    app.Run();  // Esegue il loop del demone
    return 0;
}
