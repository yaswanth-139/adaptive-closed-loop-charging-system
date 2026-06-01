// BMS with Predictive Maintenance & Health Analytics
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin Definitions
#define LED_GREEN 13
#define LED_YELLOW 12
#define LED_RED 14

// Battery Parameters
#define CELL_VOLTAGE_MIN 3.0
#define CELL_VOLTAGE_MAX 4.2
#define CELL_VOLTAGE_NOMINAL 3.7
#define VOLTAGE_IMBALANCE_THRESHOLD 0.1
#define BATTERY_CAPACITY_MAH 5000  // Initial capacity in mAh

// Health Thresholds
#define SOH_WARNING_THRESHOLD 90.0   // Warn below 90%
#define SOH_CRITICAL_THRESHOLD 80.0  // Critical below 80%
#define DEGRADATION_RATE_NORMAL 0.05 // 0.05% per cycle is normal
#define DEGRADATION_RATE_HIGH 0.15   // 0.15% per cycle is concerning

// Cell Health Tracking
struct CellHealth {
  float initialCapacity;
  float currentCapacity;
  float soh;  // State of Health (%)
  float degradationRate;  // % per cycle
  int cycleCount;
  float remainingUsefulLife;  // Estimated days
  bool isHealthy;
  String status;  // "Healthy", "Warning", "Critical"
};

CellHealth cells[3];

// Global Variables
float cellVoltages[3];
float temperature = 25.0;
float packSOC = 0;
float packSOH = 100.0;
int totalCycles = 0;

unsigned long lastUpdate = 0;
unsigned long lastCycleUpdate = 0;
int displayMode = 0;  // 0 = Normal, 1 = Health Analysis
int testScenario = 1;

enum SystemStatus {
  STATUS_NORMAL,
  STATUS_WARNING,
  STATUS_CRITICAL
};

SystemStatus currentStatus = STATUS_NORMAL;

void setup() {
  Serial.begin(115200);
  
  // Initialize LEDs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  // Initialize cell health data
  for(int i = 0; i < 3; i++) {
    cells[i].initialCapacity = BATTERY_CAPACITY_MAH;
    cells[i].currentCapacity = BATTERY_CAPACITY_MAH;
    cells[i].soh = 100.0;
    cells[i].degradationRate = 0.0;
    cells[i].cycleCount = 0;
    cells[i].remainingUsefulLife = 9999;
    cells[i].isHealthy = true;
    cells[i].status = "Healthy";
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  showStartupScreen();
  
  Serial.println("\n=================================");
  Serial.println("  Predictive BMS v2.0");
  Serial.println("  with AI Health Analytics");
  Serial.println("=================================\n");
  
  delay(2000);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Update every 2 seconds
  if (currentTime - lastUpdate >= 2000) {
    lastUpdate = currentTime;
    
    // Set test values (simulates real sensor readings)
    setTestScenario(testScenario);
    
    // Calculate metrics
    calculateSOC();
    
    // Update battery health (simulates aging over time)
    updateBatteryHealth();
    
    // Perform predictive analysis
    performPredictiveAnalysis();
    
    // Check overall status
    checkBatteryStatus();
    
    // Update displays
    if (displayMode == 0) {
      updateOLED_Normal();
    } else {
      updateOLED_HealthAnalysis();
    }
    
    updateStatusLEDs();
    displaySerialData();
    
    // Switch display mode every 10 seconds
    if ((millis() / 10000) % 2 == 0) {
      displayMode = 0;
    } else {
      displayMode = 1;
    }
  }
  
  // Simulate battery cycles (every 30 seconds = 1 cycle for demo purposes)
  if (currentTime - lastCycleUpdate >= 30000) {
    lastCycleUpdate = currentTime;
    totalCycles++;
    
    // Age the battery slightly each cycle
    for(int i = 0; i < 3; i++) {
      cells[i].cycleCount++;
      
      // Simulate different degradation rates for each cell
      float degradation = 0.05 + (i * 0.02);  // Cell 3 degrades faster
      cells[i].currentCapacity -= (cells[i].initialCapacity * degradation / 100.0);
      
      if(cells[i].currentCapacity < 0) cells[i].currentCapacity = 0;
    }
    
    Serial.println(">> Battery aged by 1 cycle <<");
  }
}

void setTestScenario(int scenario) {
  switch(scenario) {
    case 1:  // Normal battery - slight degradation in Cell 2
      cellVoltages[0] = 3.70;
      cellVoltages[1] = 3.65;  // Slightly lower
      cellVoltages[2] = 3.70;
      temperature = 25.0;
      
      // Simulate aging for demo
      cells[0].currentCapacity = 5000;
      cells[1].currentCapacity = 4800;  // 4% degraded
      cells[2].currentCapacity = 5000;
      break;
      
    case 2:  // Cell 2 degrading faster - WARNING
      cellVoltages[0] = 3.70;
      cellVoltages[1] = 3.55;  // Lower voltage
      cellVoltages[2] = 3.70;
      temperature = 28.0;
      
      cells[0].currentCapacity = 4900;
      cells[1].currentCapacity = 4500;  // 10% degraded!
      cells[2].currentCapacity = 4900;
      break;
      
    case 3:  // Cell 3 critical failure - CRITICAL
      cellVoltages[0] = 3.70;
      cellVoltages[1] = 3.65;
      cellVoltages[2] = 3.45;  // Very low!
      temperature = 35.0;  // Elevated temp
      
      cells[0].currentCapacity = 4800;
      cells[1].currentCapacity = 4700;
      cells[2].currentCapacity = 4000;  // 20% degraded - CRITICAL!
      break;
      
    case 4:  // All cells healthy, fully charged
      cellVoltages[0] = 4.2;
      cellVoltages[1] = 4.2;
      cellVoltages[2] = 4.2;
      temperature = 24.0;
      
      cells[0].currentCapacity = 5000;
      cells[1].currentCapacity = 5000;
      cells[2].currentCapacity = 5000;
      break;
      
    case 5:  // Multiple cells degrading - overall WARNING
      cellVoltages[0] = 3.60;
      cellVoltages[1] = 3.55;
      cellVoltages[2] = 3.58;
      temperature = 30.0;
      
      cells[0].currentCapacity = 4600;  // 8% degraded
      cells[1].currentCapacity = 4550;  // 9% degraded
      cells[2].currentCapacity = 4500;  // 10% degraded
      break;
      
    default:
      cellVoltages[0] = 3.7;
      cellVoltages[1] = 3.7;
      cellVoltages[2] = 3.7;
      temperature = 25.0;
  }
}

void calculateSOC() {
  float avgVoltage = (cellVoltages[0] + cellVoltages[1] + cellVoltages[2]) / 3.0;
  packSOC = ((avgVoltage - CELL_VOLTAGE_MIN) / (CELL_VOLTAGE_MAX - CELL_VOLTAGE_MIN)) * 100.0;
  
  if (packSOC > 100) packSOC = 100;
  if (packSOC < 0) packSOC = 0;
}

void updateBatteryHealth() {
  float totalSOH = 0;
  
  for(int i = 0; i < 3; i++) {
    // Calculate State of Health
    cells[i].soh = (cells[i].currentCapacity / cells[i].initialCapacity) * 100.0;
    totalSOH += cells[i].soh;
    
    // Calculate degradation rate (simplified)
    if(cells[i].cycleCount > 0) {
      float totalDegradation = 100.0 - cells[i].soh;
      cells[i].degradationRate = totalDegradation / cells[i].cycleCount;
    }
  }
  
  packSOH = totalSOH / 3.0;
}

void performPredictiveAnalysis() {
  for(int i = 0; i < 3; i++) {
    // Estimate Remaining Useful Life (RUL)
    // Assumes linear degradation and 80% SOH as end-of-life
    if(cells[i].degradationRate > 0.001) {
      float remainingDegradation = cells[i].soh - 80.0;  // Need to reach 80%
      float cyclesRemaining = remainingDegradation / cells[i].degradationRate;
      
      // Convert cycles to days (assume 1 cycle per day for real EV usage)
      cells[i].remainingUsefulLife = cyclesRemaining;
    } else {
      cells[i].remainingUsefulLife = 9999;  // Essentially infinite
    }
    
    // Classify cell health
    if(cells[i].soh >= SOH_WARNING_THRESHOLD) {
      cells[i].status = "Healthy";
      cells[i].isHealthy = true;
    } else if(cells[i].soh >= SOH_CRITICAL_THRESHOLD) {
      cells[i].status = "Warning";
      cells[i].isHealthy = false;
    } else {
      cells[i].status = "Critical";
      cells[i].isHealthy = false;
    }
  }
}

void checkBatteryStatus() {
  currentStatus = STATUS_NORMAL;
  
  // Check voltage limits
  for (int i = 0; i < 3; i++) {
    if (cellVoltages[i] < CELL_VOLTAGE_MIN || cellVoltages[i] > CELL_VOLTAGE_MAX) {
      currentStatus = STATUS_CRITICAL;
      return;
    }
  }
  
  // Check health status
  for(int i = 0; i < 3; i++) {
    if(cells[i].status == "Critical") {
      currentStatus = STATUS_CRITICAL;
      return;
    } else if(cells[i].status == "Warning" && currentStatus != STATUS_CRITICAL) {
      currentStatus = STATUS_WARNING;
    }
  }
  
  // Check imbalance
  float maxV = cellVoltages[0];
  float minV = cellVoltages[0];
  for (int i = 1; i < 3; i++) {
    if (cellVoltages[i] > maxV) maxV = cellVoltages[i];
    if (cellVoltages[i] < minV) minV = cellVoltages[i];
  }
  
  if (maxV - minV > VOLTAGE_IMBALANCE_THRESHOLD) {
    if(currentStatus == STATUS_NORMAL) {
      currentStatus = STATUS_WARNING;
    }
  }
  
  // Check temperature
  if (temperature > 45) {
    currentStatus = STATUS_CRITICAL;
  } else if (temperature > 40 && currentStatus == STATUS_NORMAL) {
    currentStatus = STATUS_WARNING;
  }
}

void updateOLED_Normal() {
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("  EV Battery BMS"));
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // Cell Voltages with health indicators
  for(int i = 0; i < 3; i++) {
    display.setCursor(0, 14 + (i * 10));
    display.print(F("C"));
    display.print(i+1);
    display.print(F(":"));
    display.print(cellVoltages[i], 2);
    display.print(F("V"));
    
    // Health indicator
    display.setCursor(56, 14 + (i * 10));
    if(cells[i].isHealthy) {
      display.print(F("OK"));
    } else if(cells[i].status == "Warning") {
      display.print(F("!"));
    } else {
      display.print(F("X"));
    }
  }
  
  // SOC and SOH
  display.setCursor(72, 14);
  display.print(F("SOC:"));
  display.print((int)packSOC);
  display.print(F("%"));
  
  display.setCursor(72, 24);
  display.print(F("SOH:"));
  display.print((int)packSOH);
  display.print(F("%"));
  
  // Pack voltage and temp
  float packVoltage = cellVoltages[0] + cellVoltages[1] + cellVoltages[2];
  display.setCursor(0, 44);
  display.print(F("Pack:"));
  display.print(packVoltage, 1);
  display.print(F("V"));
  
  display.setCursor(68, 44);
  display.print(F("T:"));
  display.print((int)temperature);
  display.print(F("C"));
  
  // Status bar
  display.drawLine(0, 54, 128, 54, SSD1306_WHITE);
  display.setCursor(0, 56);
  
  switch(currentStatus) {
    case STATUS_NORMAL:
      display.print(F(" Status: NORMAL"));
      display.fillCircle(120, 59, 3, SSD1306_WHITE);
      break;
    case STATUS_WARNING:
      display.print(F(" Status: WARNING"));
      display.fillTriangle(118, 63, 122, 63, 120, 57, SSD1306_WHITE);
      break;
    case STATUS_CRITICAL:
      display.print(F(" Status: CRITICAL"));
      display.drawLine(117, 57, 123, 63, SSD1306_WHITE);
      display.drawLine(123, 57, 117, 63, SSD1306_WHITE);
      break;
  }
  
  display.display();
}

void updateOLED_HealthAnalysis() {
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F(" Health Analysis"));
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // Find worst cell
  int worstCell = 0;
  float lowestSOH = cells[0].soh;
  for(int i = 1; i < 3; i++) {
    if(cells[i].soh < lowestSOH) {
      lowestSOH = cells[i].soh;
      worstCell = i;
    }
  }
  
  // Display worst cell info
  display.setCursor(0, 14);
  display.print(F("Cell "));
  display.print(worstCell + 1);
  display.print(F(": "));
  display.print(cells[worstCell].status);
  
  display.setCursor(0, 24);
  display.print(F("Capacity: "));
  display.print((int)cells[worstCell].soh);
  display.print(F("%"));
  
  display.setCursor(0, 34);
  display.print(F("Degrade: "));
  display.print(cells[worstCell].degradationRate, 2);
  display.print(F("%/cy"));
  
  display.setCursor(0, 44);
  if(cells[worstCell].remainingUsefulLife < 9999) {
    display.print(F("RUL: ~"));
    if(cells[worstCell].remainingUsefulLife > 365) {
      display.print((int)(cells[worstCell].remainingUsefulLife / 365));
      display.print(F(" yrs"));
    } else {
      display.print((int)cells[worstCell].remainingUsefulLife);
      display.print(F(" days"));
    }
  } else {
    display.print(F("RUL: Excellent"));
  }
  
  // Recommendation
  display.drawLine(0, 54, 128, 54, SSD1306_WHITE);
  display.setCursor(0, 56);
  
  if(packSOH >= SOH_WARNING_THRESHOLD) {
    display.print(F(" Recommend: None"));
  } else if(packSOH >= SOH_CRITICAL_THRESHOLD) {
    display.print(F(" Monitor closely"));
  } else {
    display.print(F(" Service soon!"));
  }
  
  display.display();
}

void showStartupScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(5, 10);
  display.println(F("Predictive BMS"));
  display.setCursor(20, 25);
  display.println(F("v2.0"));
  display.setCursor(5, 40);
  display.println(F("AI Health Analytics"));
  display.display();
  delay(2000);
  
  // Progress bar
  for(int i = 0; i < 128; i += 4) {
    display.fillRect(0, 56, i, 8, SSD1306_WHITE);
    display.display();
    delay(15);
  }
  
  delay(500);
}

void updateStatusLEDs() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  
  switch(currentStatus) {
    case STATUS_NORMAL:
      digitalWrite(LED_GREEN, HIGH);
      break;
    case STATUS_WARNING:
      digitalWrite(LED_YELLOW, HIGH);
      break;
    case STATUS_CRITICAL:
      digitalWrite(LED_RED, HIGH);
      break;
  }
}

void displaySerialData() {
  Serial.println("========================================");
  Serial.println("       PREDICTIVE BMS STATUS");
  Serial.println("========================================");
  Serial.print("Scenario: ");
  Serial.println(testScenario);
  Serial.print("Total Cycles: ");
  Serial.println(totalCycles);
  Serial.println();
  
  // Cell details
  for (int i = 0; i < 3; i++) {
    Serial.print("Cell ");
    Serial.print(i + 1);
    Serial.println(":");
    Serial.print("  Voltage: ");
    Serial.print(cellVoltages[i], 3);
    Serial.println(" V");
    Serial.print("  Capacity: ");
    Serial.print((int)cells[i].currentCapacity);
    Serial.print(" / ");
    Serial.print((int)cells[i].initialCapacity);
    Serial.println(" mAh");
    Serial.print("  SOH: ");
    Serial.print(cells[i].soh, 1);
    Serial.println(" %");
    Serial.print("  Degradation Rate: ");
    Serial.print(cells[i].degradationRate, 3);
    Serial.println(" %/cycle");
    Serial.print("  RUL: ");
    if(cells[i].remainingUsefulLife < 9999) {
      Serial.print((int)cells[i].remainingUsefulLife);
      Serial.println(" days");
    } else {
      Serial.println("Excellent");
    }
    Serial.print("  Status: ");
    Serial.println(cells[i].status);
    Serial.println();
  }
  
  // Pack summary
  float packVoltage = cellVoltages[0] + cellVoltages[1] + cellVoltages[2];
  Serial.print("Pack Voltage: ");
  Serial.print(packVoltage, 2);
  Serial.println(" V");
  Serial.print("Pack SOC: ");
  Serial.print(packSOC, 1);
  Serial.println(" %");
  Serial.print("Pack SOH: ");
  Serial.print(packSOH, 1);
  Serial.println(" %");
  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" °C");
  
  Serial.print("\nSystem Status: ");
  switch(currentStatus) {
    case STATUS_NORMAL:
      Serial.println("✓ NORMAL");
      break;
    case STATUS_WARNING:
      Serial.println("⚠ WARNING - Monitor battery health");
      break;
    case STATUS_CRITICAL:
      Serial.println("❌ CRITICAL - Service required!");
      break;
  }
  Serial.println("========================================\n");
}
