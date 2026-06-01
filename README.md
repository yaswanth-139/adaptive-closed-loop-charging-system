# Adaptive Closed-Loop Charging System

A real-time embedded system built on ESP32 that monitors battery state and 
adaptively controls charging using closed-loop PWM feedback.

## Overview

Traditional chargers apply fixed charging without considering actual battery 
condition. This system continuously reads battery voltage and current, 
estimates the State of Charge (SOC), and adjusts PWM duty cycle in real time 
to charge safely and efficiently.

## Features

- Real-time SOC estimation using ADC feedback
- Adaptive PWM duty cycle control based on battery state
- SOH monitoring with predictive degradation analysis
- Charge cycle history tracking
- FreeRTOS task management for concurrent monitoring

## Tech Stack

- Platform: ESP32
- Language: Embedded C
- RTOS: FreeRTOS
- Protocols: I2C, ADC, PWM
- Simulation: Wokwi

## Simulation

🔗 https://wokwi.com/projects/455124494333228033

## How It Works

1. ADC reads battery voltage and current continuously
2. SOC is calculated from sensor feedback in real time
3. PWM duty cycle is adjusted based on SOC threshold
4. FreeRTOS manages sensor, control and monitoring tasks concurrently
5. Degradation prediction alerts when cycle count exceeds threshold

## Author

Yaswanth R — ECE Undergraduate, GKM College of Engineering and Technology  
[LinkedIn](https://linkedin.com/in/yaswanth-r-965b12297)
