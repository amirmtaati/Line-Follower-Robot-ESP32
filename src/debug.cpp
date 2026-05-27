// debug.cpp
#include <Arduino.h>
#include "types.hh"
#include "debug.hh"

// ── internal snapshot helpers ─────────────────────────────

static void snapshotNV(float* out) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    memcpy(out, nv, sizeof(float) * N_SENSORS);
    xSemaphoreGive(robotMutex);
}

static void snapshotCalibration(float* outWhite, float* outBlack) {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    memcpy(outWhite, WHITE_VALUES, sizeof(float) * N_SENSORS);
    memcpy(outBlack, BLACK_VALUES, sizeof(float) * N_SENSORS);
    xSemaphoreGive(robotMutex);
}

static RobotState snapshotRobot() {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    RobotState copy = robot;
    xSemaphoreGive(robotMutex);
    return copy;
}

static State snapshotState() {
    xSemaphoreTake(robotMutex, portMAX_DELAY);
    State copy = state;
    xSemaphoreGive(robotMutex);
    return copy;
}

// ── public print functions ────────────────────────────────

void printNormalizedSensorValues() {
    float local[N_SENSORS];
    snapshotNV(local);
    Serial.printf("NV:    %.3f  %.3f  %.3f  %.3f\n",
        local[0], local[1], local[2], local[3]);
}

void printCalibrationValues() {
    float white[N_SENSORS], black[N_SENSORS];
    snapshotCalibration(white, black);

    Serial.printf("White: %.1f  %.1f  %.1f  %.1f\n",
        white[0], white[1], white[2], white[3]);
    Serial.printf("Black: %.1f  %.1f  %.1f  %.1f\n",
        black[0], black[1], black[2], black[3]);
}

void printControlParameters() {
    RobotState r = snapshotRobot();
    Serial.printf("err: %6.3f  corr: %7.2f  kP: %.1f  base: %.0f\n",
        r.error, r.correction, r.kP, r.base_speed);
}

void printMotorSpeeds() {
    RobotState r = snapshotRobot();
    int left  = r.base_speed + (int)r.correction;
    int right = r.base_speed - (int)r.correction;
    Serial.printf("L: %5d  R: %5d\n", left, right);
}

void printState() {
    switch (snapshotState()) {
        case State::STOPPED:     Serial.println("STATE: STOPPED");     break;
        case State::CALIBRATING: Serial.println("STATE: CALIBRATING"); break;
        case State::MOVING:      Serial.println("STATE: MOVING");      break;
    }
}

// ── debug task ────────────────────────────────────────────

void vDebugTask(void* parameters) {
    while (!ready) vTaskDelay(pdMS_TO_TICKS(10));

    while (true) {
        printState();
        printNormalizedSensorValues();
        printControlParameters();
        printMotorSpeeds();
        Serial.println("─────────────────────────────");

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
