// Basic program skeleton for a Sketch File (.sk) Viewer
#include "displayfull.h"
#include "sketch.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Allocate memory for a drawing state and initialise it
state *newState() {
  state *s = malloc(sizeof(state));

  s->x = 0;
  s->y = 0;
  s->tx = 0;
  s->ty = 0;
  s->tool = LINE;
  s->start = 0;
  s->data = 0;
  s->end = false;

  return s;
}

// Release all memory associated with the drawing state
void freeState(state *s) {
  free(s);
}

// Extract an opcode from a byte (two most significant bits).
int getOpcode(byte b) {
  if (b >= 192) return DATA;
  else if (b >= 128) return TOOL;
  else if (b >= 64) return DY;
  else return DX;
}

// Extract an operand (-32..31) from the rightmost 6 bits of a byte.
int getOperand(byte b) {
  if (b >= 128) b -= 128;
  if (b >= 64) b -= 64;

  if (b >= 32) return (b - 64);
  else return b;
}

// Executes the tool command
void doTOOL(display *d, state *s, int OPERAND) {
  switch (OPERAND) {
    case NONE: s->tool = NONE; break;
    case LINE: s->tool = LINE; break;
    case BLOCK: s->tool = BLOCK; break;
    case COLOUR: colour(d, s->data); break;
    case TARGETX: s->tx = s->data; break;
    case TARGETY: s->ty = s->data; break;
    case SHOW: show(d); break;
    case PAUSE: pause(d, s->data); break;
    case NEXTFRAME: s->end = true; break;
    default: break;
  }

  s->data = 0;
}

// Executes the DX command
void doDX(state *s, int OPERAND) {
  s->tx += OPERAND;
}

// Executes the DY command
void doDY(display *d, state *s, int OPERAND) {
  s->ty += OPERAND;

  if (s->tool == LINE) {
    line(d, s->x, s->y, s->tx, s->ty);
  } else if (s->tool == BLOCK) {
    block(d, s->x, s->y, ((s->tx) - (s->x)), ((s->ty) - (s->y)));
  }

  s->x = s->tx;
  s->y = s->ty;
}

// Executes the data command
void doDATA(state *s, int OPERAND) {
  s->data = s->data << 6;
  s->data += (OPERAND < 0) ? OPERAND + 64 : OPERAND;
}

// Execute the next byte of the command sequence.
void obey(display *d, state *s, byte op) {
  int OPCODE = getOpcode(op);
  int OPERAND = getOperand(op);

  switch (OPCODE) {
    case TOOL: doTOOL(d, s, OPERAND); break;
    case DX: doDX(s, OPERAND); break;
    case DY: doDY(d, s, OPERAND); break;
    case DATA: doDATA(s, OPERAND); break;
    default: break;
  }
}

// Draw a frame of the sketch file. For basic and intermediate sketch files
// this means drawing the full sketch whenever this function is called.
// For advanced sketch files this means drawing the current frame whenever
// this function is called.
bool processSketch(display *d, void *data, const char pressedKey) {
  if (data == NULL) return (pressedKey == 27);

  state *s = (state*) data;
  FILE *fp = fopen(getName(d), "rb");
  fseek(fp, (long int)s->start, SEEK_SET);
  byte b;

  while (! s->end) {
    b = fgetc(fp);
    obey(d, s, b);

    if (s->end) s->start = (unsigned int)ftell(fp);

    if (feof(fp)) {
      s->end = true;
      s->start = 0;
      s->data = 0;
    }
  }

  show(d);
  fclose(fp);

  s->x = 0;
  s->y = 0;
  s->tx = 0;
  s->ty = 0;
  s->tool = LINE;
  s->end = false;

  return true;
}

// View a sketch file in a 200x200 pixel window given the filename
void view(char *filename) {
  display *d = newDisplay(filename, 200, 200);
  state *s = newState();
  run(d, s, processSketch);
  freeState(s);
  freeDisplay(d);
}

// Include a main function only if we are not testing (make sketch),
// otherwise use the main function of the test.c file (make test).
#ifndef TESTING
int main(int n, char *args[n]) {
  if (n != 2) { // return usage hint if not exactly one argument
    printf("Use ./sketch file\n");
    exit(1);
  } else view(args[1]); // otherwise view sketch file in argument
  return 0;
}
#endif
