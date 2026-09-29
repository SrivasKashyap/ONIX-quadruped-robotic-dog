/*
  ROBOTIC DOG — Sit / Stand / Walk
  
  CONTROLS:
    Button press (SW)     → Toggle Sit / Stand
    Joystick UP           → Walk forward
    Joystick neutral      → Stop walking

  CHANNEL MAP:
  CH0=FL Shoulder  CH1=FL Knee
  CH2=FR Shoulder  CH3=FR Knee
  CH4=BL Shoulder  CH5=BL Knee
  CH6=BR Shoulder  CH7=BR Knee

  WIRING:
    Joystick VRY → A1
    Joystick SW  → D7
    GND          → GND
*/

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

#define SERVOMIN  102
#define SERVOMAX  553
#define JOY_Y     A1
#define JOY_SW    7
#define CENTER    512
#define DEADZONE  150

// ── YOUR CALIBRATED ANGLES — DO NOT CHANGE ────────────
//                  CH0   CH1   CH2   CH3   CH4   CH5   CH6   CH7
//                  FL    FL    FR    FR    BL    BL    BR    BR
//                  SHO   KNE   SHO   KNE   SHO   KNE   SHO   KNE
const int SIT[8]   = { 180,    0,    0,  150,  150,    0,    0,  150 };
const int STAND[8] = { 100,  100,   50,   50,  100,  100,   50,   50 };
// ──────────────────────────────────────────────────────

// ── WALK PARAMETERS — tune these ──────────────────────
// How much to lift a leg (reduce shoulder angle = leg goes up)
const int LIFT_AMOUNT  = 25;  // degrees to lift leg off ground
// How much knee curls when leg is lifted
const int KNEE_CURL    = 20;  // degrees knee pulls up when lifted
// Walk step delay (lower = faster walk)
const unsigned long WALK_STEP_MS = 8;
// Pause between diagonal steps
const unsigned long STEP_PAUSE   = 300;
// ──────────────────────────────────────────────────────

// Diagonal pairs:
// PAIR A = FL(0,1) + BR(6,7)
// PAIR B = FR(2,3) + BL(4,5)

const int STEP_DEG           = 1;
const unsigned long STEP_MS  = 10;
const unsigned long PAUSE_2S = 2000;
const unsigned long PAUSE_1S = 1000;

enum State {
  IDLE,
  ST1_FRONT_UP, ST2_PAUSE, ST3_BACK_KNEES_DOWN,
  ST4_PAUSE_KNEES, ST5_BACK_SHO_UP,
  SI1_BACK_SHO_DOWN, SI2_PAUSE_SHO, SI3_BACK_KNEES_UP,
  SI4_PAUSE, SI5_FRONT_DOWN,
  WALKING
};

State state      = IDLE;
bool isStanding  = false;
int curPos[8];
int target[8];
unsigned long lastStep   = 0;
unsigned long pauseStart = 0;

// Walk phase tracking
int walkPhase = 0;  // 0=lift pair A, 1=set down A, 2=lift pair B, 3=set down B
unsigned long walkTimer = 0;

// ── HELPERS ───────────────────────────────────────────
void setServo(int ch, int deg) {
  deg = constrain(deg, 0, 180);
  pca.setPWM(ch, 0, map(deg, 0, 180, SERVOMIN, SERVOMAX));
}

void setTargetChs(int fromCh, int toCh, const int* pose) {
  for (int ch = fromCh; ch <= toCh; ch++) target[ch] = pose[ch];
}

void holdChs(int fromCh, int toCh) {
  for (int ch = fromCh; ch <= toCh; ch++) target[ch] = curPos[ch];
}

bool chsDone(int fromCh, int toCh) {
  for (int ch = fromCh; ch <= toCh; ch++)
    if (curPos[ch] != target[ch]) return false;
  return true;
}

bool allDone() { return chsDone(0, 7); }

void startPause() { pauseStart = millis(); }
bool pauseDone(unsigned long dur) { return millis() - pauseStart >= dur; }

bool buttonPressed() {
  static bool lastState = HIGH, stableState = HIGH;
  static unsigned long lastDeb = 0;
  bool r = digitalRead(JOY_SW);
  if (r != lastState) { lastDeb = millis(); lastState = r; }
  if (millis() - lastDeb > 120 && r != stableState) {
    stableState = r;
    if (stableState == LOW) return true;
  }
  return false;
}

int readJoystick() {
  int val = analogRead(JOY_Y);
  int dy = val - CENTER;
  if (abs(dy) < DEADZONE) return 0;
  return (dy < 0) ? 1 : -1;
}

void smoothStep() {
  for (int ch = 0; ch < 8; ch++) {
    if (curPos[ch] < target[ch])      curPos[ch] += STEP_DEG;
    else if (curPos[ch] > target[ch]) curPos[ch] -= STEP_DEG;
    if (abs(curPos[ch] - target[ch]) <= STEP_DEG) curPos[ch] = target[ch];
    setServo(ch, curPos[ch]);
  }
}

// Instantly set all servos (for walk — no smooth needed per step)
void instantSet() {
  for (int ch = 0; ch < 8; ch++) {
    curPos[ch] = target[ch];
    setServo(ch, curPos[ch]);
  }
}
// ──────────────────────────────────────────────────────

// ── WALK GAIT ─────────────────────────────────────────
// Diagonal gait:
// Phase 0: Lift FL+BR (raise shoulder, curl knee)
// Phase 1: Set FL+BR down (restore stand angles)
// Phase 2: Lift FR+BL
// Phase 3: Set FR+BL down
// Repeat

void setWalkPhase(int phase) {
  // Start from STAND angles
  for (int ch = 0; ch < 8; ch++) target[ch] = STAND[ch];

  if (phase == 0) {
    // Lift FL(0,1) and BR(6,7)
    // FL shoulder: reduce angle to lift (shoulder goes "up" = leg lifts)
    target[0] = STAND[0] - LIFT_AMOUNT;  // FL shoulder lifts
    target[1] = STAND[1] - KNEE_CURL;    // FL knee curls
    target[6] = STAND[6] + LIFT_AMOUNT;  // BR shoulder lifts (opposite direction)
    target[7] = STAND[7] + KNEE_CURL;    // BR knee curls
  }
  else if (phase == 1) {
    // Set FL+BR back to stand — all at STAND angles (already set)
  }
  else if (phase == 2) {
    // Lift FR(2,3) and BL(4,5)
    target[2] = STAND[2] + LIFT_AMOUNT;  // FR shoulder lifts
    target[3] = STAND[3] + KNEE_CURL;    // FR knee curls
    target[4] = STAND[4] - LIFT_AMOUNT;  // BL shoulder lifts
    target[5] = STAND[5] - KNEE_CURL;    // BL knee curls
  }
  else if (phase == 3) {
    // Set FR+BL back to stand — all at STAND angles (already set)
  }
}
// ──────────────────────────────────────────────────────

void setup() {
  Serial.begin(9600);
  pinMode(JOY_SW, INPUT_PULLUP);

  pca.begin();
  pca.setOscillatorFrequency(27000000);
  pca.setPWMFreq(50);
  delay(200);

  for (int ch = 0; ch < 8; ch++) {
    setServo(ch, SIT[ch]);
    curPos[ch] = SIT[ch];
    target[ch] = SIT[ch];
  }

  isStanding = false;
  state = IDLE;
  Serial.println("Ready! Press button to STAND.");
}

void loop() {

  // ── BUTTON: sit/stand toggle ───────────────────────
  if (buttonPressed()) {
    if (state == WALKING) {
      // Stop walking, go to stand
      state = IDLE;
      isStanding = true;
      for (int ch = 0; ch < 8; ch++) target[ch] = STAND[ch];
      Serial.println("Stopped walking.");
      return;
    }
    if (state == IDLE) {
      isStanding = !isStanding;
      if (isStanding) {
        Serial.println("STANDING — front legs up...");
        setTargetChs(0, 3, STAND);
        holdChs(4, 7);
        state = ST1_FRONT_UP;
      } else {
        Serial.println("SITTING — back shoulders down...");
        target[4] = SIT[4]; target[6] = SIT[6];
        holdChs(0, 3); holdChs(5, 5); holdChs(7, 7);
        state = SI1_BACK_SHO_DOWN;
      }
    }
  }

  // ── JOYSTICK: walk when standing and IDLE ─────────
  if (state == IDLE && isStanding && readJoystick() == 1) {
    Serial.println("WALKING...");
    walkPhase = 0;
    walkTimer = millis();
    setWalkPhase(0);
    state = WALKING;
  }

  // ── SMOOTH MOTION (sit/stand) ─────────────────────
  unsigned long now = millis();

  if (state != WALKING) {
    if (now - lastStep < STEP_MS) return;
    lastStep = now;
    smoothStep();
  }

  // ── STAND STATE MACHINE ───────────────────────────
  if (state == ST1_FRONT_UP && chsDone(0, 3)) {
    Serial.println("Front up! Pausing 2s...");
    startPause(); state = ST2_PAUSE;
  }
  if (state == ST2_PAUSE && pauseDone(PAUSE_2S)) {
    Serial.println("Back knees extending...");
    target[5] = STAND[5]; target[7] = STAND[7];
    holdChs(4, 4); holdChs(6, 6);
    state = ST3_BACK_KNEES_DOWN;
  }
  if (state == ST3_BACK_KNEES_DOWN && chsDone(5, 5) && chsDone(7, 7)) {
    Serial.println("Pausing 1s...");
    startPause(); state = ST4_PAUSE_KNEES;
  }
  if (state == ST4_PAUSE_KNEES && pauseDone(PAUSE_1S)) {
    Serial.println("Back shoulders rising...");
    target[4] = STAND[4]; target[6] = STAND[6];
    state = ST5_BACK_SHO_UP;
  }
  if (state == ST5_BACK_SHO_UP && chsDone(4, 4) && chsDone(6, 6)) {
    Serial.println("STANDING! Joystick UP to walk.");
    state = IDLE;
  }

  // ── SIT STATE MACHINE ─────────────────────────────
  if (state == SI1_BACK_SHO_DOWN && chsDone(4, 4) && chsDone(6, 6)) {
    startPause(); state = SI2_PAUSE_SHO;
  }
  if (state == SI2_PAUSE_SHO && pauseDone(PAUSE_1S)) {
    target[5] = SIT[5]; target[7] = SIT[7];
    state = SI3_BACK_KNEES_UP;
  }
  if (state == SI3_BACK_KNEES_UP && chsDone(5, 5) && chsDone(7, 7)) {
    startPause(); state = SI4_PAUSE;
  }
  if (state == SI4_PAUSE && pauseDone(PAUSE_2S)) {
    setTargetChs(0, 3, SIT);
    state = SI5_FRONT_DOWN;
  }
  if (state == SI5_FRONT_DOWN && chsDone(0, 3)) {
    Serial.println("SITTING!");
    state = IDLE;
  }

  // ── WALK STATE MACHINE ────────────────────────────
  if (state == WALKING) {
    // Stop if joystick released
    if (readJoystick() != 1) {
      for (int ch = 0; ch < 8; ch++) target[ch] = STAND[ch];
      instantSet();
      state = IDLE;
      Serial.println("Walk stopped.");
      return;
    }

    if (millis() - walkTimer >= STEP_PAUSE) {
      walkTimer = millis();
      walkPhase = (walkPhase + 1) % 4;
      setWalkPhase(walkPhase);
      instantSet();
      Serial.print("Walk phase: "); Serial.println(walkPhase);
    }
  }
}