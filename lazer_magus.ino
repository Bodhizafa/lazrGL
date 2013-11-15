
//#include <stdint.h>
//#include <stdlib.h>
unsigned long frames = 0;
// 0, 0 is bottom right ??
// We will not project outside of this, (0, 0) - (4095,4095) is the max
long win_x_min = 0;
long win_y_min = 0;
long win_x_max = 4095;
long win_y_max = 4095;
int blanking = 31;
#define SET_LASER(on) digitalWrite(blanking, on)
//#define SET_LASER(on) digitalWrite(blanking, 0) // Uncomment this to entirely disable laser.
 
// return val if it is between mn and mx
#define clamp(val, mn, mx) (val < mn ? mn : (val > mx ? mx : val))
// Move the laser... duh
#define move(x, y) { analogWrite(DAC0, (int)clamp(y, win_y_min, win_y_max)); analogWrite(DAC1, (int)clamp(x, win_x_min, win_x_max));}

#define pi (3.14159)

static boolean enable;
#define INTERNODE_DELAY_US 150
#define INTERPAT_DELAY_US 400
#define FLAG_BLANKING 1
#define FLAG_END (1 << 1)

// Create a callback like render_circle, which should fill in 'frame'
// Call mkPat with some epsilon (t step), and a function reference to get a pattern
// Set up mvMatrix, and call display_pattern to display it wherever.
struct frame_s {
  float x;
  float y;
  uint8_t flags;
};

struct pattern_s {
  struct frame_s *frames;
  int numFrames;
};
 
struct pattern_s* mkPat(void(*cb)(float, struct frame_s*, void*), float epsilon, void* params) {
  int numFrames = floor(1 / epsilon);
  struct pattern_s* ret = (struct pattern_s*)malloc(sizeof(struct pattern_s));
  ret->numFrames = numFrames;
  ret->frames = (struct frame_s*)malloc(numFrames * sizeof(struct frame_s));
  for (int i = 0; i < numFrames; i++) {
    cb((float) i / (float)numFrames, ret->frames + i, params);
  }
  Serial.print("Rendered ");
  Serial.print(numFrames);
  Serial.print(" Frames\n");
  return ret;
}
 
void render_circle(float t, struct frame_s* f, void* params) {
  f->x = sin(t * 2 * PI);
  f->y = cos(t * 2 * PI);
  f->flags = FLAG_BLANKING;
}

void render_box(float t, struct frame_s* f, void* params) {
  f->flags = FLAG_BLANKING;
  if (t < 0.25) {
    f->x = -1;
    f->y = t * 8 - 1.;
  } else if (t < 0.5) {
    f->x = (t - 0.25) * 8 - 1;
    f->y = 1;
  } else if (t < 0.75) {
    f->x = 1;
    f->y = 1 - ((t - 0.5) * 8);
  } else {
    f->x = 1 - ((t - 0.75) * 8);
    f->y = -1;
  }
}
 
// display pat under the current mvmatrix
void display_pattern(struct pattern_s* pat) {
  SET_LASER(0);
  mvMove(pat->frames[0].x, pat->frames[0].y);
  delayMicroseconds(INTERPAT_DELAY_US);
  for (int i = 0; i < pat->numFrames; i++) {
    SET_LASER(pat->frames[i].flags & FLAG_BLANKING);
    mvMove(pat->frames[i].x, pat->frames[i].y);
    delayMicroseconds(INTERNODE_DELAY_US);
  }
}
 

void dump_xy(float x, float y) {
  Serial.print("\tX: ");
  Serial.print(x);
  Serial.print("\tY: ");
  Serial.print(y);
}

void dump_pattern(struct pattern_s* pat) 
{
  for (int i = 0; i < pat->numFrames; i++) {
    Serial.print(i);
    Serial.print("\t");
    Serial.print((pat->frames[i].flags & FLAG_BLANKING) ? "!" : "-");
    Serial.print("\tX: ");
    dump_xy(pat->frames[i].x, pat->frames[i].y);
    Serial.print(" -> ");
    Serial.print("\n");
  }
}
boolean debug = false;

void iterate(long frame) {
  float th = (float)frame / 23.0; 
  mvMatrixTranslate(frame, frame);
  mvMatrixRotate(th * 1.3);
  mvMatrixScale(1024, 1024);
}
static long steps;
static pattern_s* pattern = NULL; // Currently there's only one pattern. This is cause I'm lazy.
void check_cmd() {
  long smillis;
  if (Serial.available()) {
    debug = true;
    char cmd = Serial.read();
    switch(cmd) {
      // Enable the laser
      case 'o':
      case 'O':
        enable = true;
        Serial.print(steps);
        Serial.write(" steps. Enabling\n");
        break;
      // Disable the laser
      case 'z':
      case 'Z':
        enable = false;
        SET_LASER(LOW);
        Serial.print(steps);
        Serial.write(" steps. Disabling\n");
        break;
      // Display the window as a box
      case 'w':
      case 'W':
        Serial.write("Window Xm XM Ym YM ");
        Serial.print(win_x_min);
        Serial.print(" ");
        Serial.print(win_x_max);
        Serial.print(" ");
        Serial.print(win_y_min);
        Serial.print(" ");
        Serial.print(win_y_max);
        Serial.print(" ");
        Serial.write("\n");
        SET_LASER(HIGH);
        smillis = millis();
        while (millis() < smillis + 500) {
          move(win_x_min, win_y_min);
          delayMicroseconds(INTERNODE_DELAY_US * 5);
          move(win_x_min, win_y_max);
          delayMicroseconds(INTERNODE_DELAY_US * 5);
          move(win_x_max, win_y_max);
          delayMicroseconds(INTERNODE_DELAY_US * 5);
          move(win_x_max, win_y_min);
          delayMicroseconds(INTERNODE_DELAY_US * 5);
        }
        SET_LASER(LOW);
        Serial.write("Done\n");
        break;
      case 'D':
      case 'd':
        dump_pattern(pattern);
        Serial.print(" [");
        dumpMvMatrix();
        Serial.print(" ]\n");
        break;
      case 'R':
        steps = 0;
        mvMatrixIdentity();
        Serial.print("identified\n");
        break;
      case 'r':
        steps = 0;
        mvMatrixDefault();
        Serial.print("resettin'\n");
        break;
      case 's':
        steps++;
        Serial.print("Iterating step ");
        Serial.print(steps);
        debug = true;
        mvMatrixPush();
        iterate(steps);
        mvMatrixPop();
        debug = false;
        Serial.print(".\n");
        break;
      case 'p':
      case 'P':
        smillis = millis();
        while(millis() < smillis + 500) {
          display_pattern(pattern);
        }
        SET_LASER(0);
        break;
      default: 
        Serial.write("Unknown command\n");
    }
    debug = false;
  }
}
static long frame = 0;

void setup() {
  // open a serial connection
  Serial.begin(9600); 
  // make our digital pin an output
  analogWriteResolution(12); // 0 - 4095 values
  pinMode(blanking, OUTPUT);
  frame = 0;
  SET_LASER(LOW);  //turn off

  pattern = mkPat(render_box, 0.02, NULL);
  
  mvMatrixPush();
  mvMatrixIdentity();
  mvMatrixTranslate(0.5, 0.5);
  while (Serial.available() == 0) {
    // derp
  }
}

void loop() {
  check_cmd();
  float th;
  if (!enable) {
    delayMicroseconds(INTERNODE_DELAY_US);
    goto end;
  }
  mvMatrixPush();
  iterate(steps);
  display_pattern(pattern);
  mvMatrixPop();
  end:
    steps++;
}
