#include "displayfull.h"

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Operation and tool types
enum { DX = 0, DY = 1, TOOL = 2, DATA = 3};
enum { NONE = 0, LINE = 1, BLOCK = 2, COLOUR = 3, TARGETX = 4, TARGETY = 5};

// For conciseness
typedef unsigned char byte;

// Data structure holding all information from a pgm file
typedef struct pgm {char *type; int h; int w; int maxVal; byte **data;} pgm;

// Checks if a file has been opened properly and throws an error if not
void checkFileError(FILE *fp, char *fileName) {
  if (!fp) {
    printf("Unable to open file %s.\n", fileName);
    exit(1);
  }
}

// ----------------------------- PGM -> SK FUNCTIONS ---------------------------

// Allocate memory for a 2D array of bytes
byte **malloc2DByteArray(int dim1, int dim2) {
  byte **bpp;

  bpp = (byte**) malloc(dim1 * sizeof(byte*));

  for (int i = 0; i < dim1; i++) {
    bpp[i] = (byte*) malloc(dim2 * sizeof(byte));
  }

  return bpp;
}

// Release memory associated with a 2D array of bytes
void free2DByteArray(byte **bpp, int dim1) {
  for (int i = 0; i < dim1; i++) {
    free(bpp[i]);
  }

  free(bpp);
}

// Create a new pgm stucture and initialise values
pgm *newPgm(char *metaData) {
  pgm *p = malloc(sizeof(pgm));
  char *type = strtok(metaData, " ");
  char *width = strtok(NULL, " ");
  char *height = strtok(NULL, " ");
  char *maxVal = strtok(NULL, " ");

  p->type = type;
  p->h = atoi(height);
  p->w = atoi(width);
  p->maxVal = atoi(maxVal);
  p->data = malloc2DByteArray(p->w, p->h);

  return p;
}

// Free a pgm structure
void freePgm(pgm *p) {
  free2DByteArray(p->data, p->w);
  free(p);
}

// Read metadata and gray values from a pgm file into a pgm structure
pgm * readPgmFile(FILE *fp) {
  byte b = fgetc(fp);
  char metaData[15];

  for (int i = 0; i < 14; i++) {
    metaData[i] = b;
    b = fgetc(fp);
  }

  metaData[14] = '\0';
  pgm *p = newPgm(metaData);
  b = fgetc(fp);

  for (int i = 0; i < p->h; i++) {
    for (int j = 0; j < p->w; j++) {
      p->data[i][j] = b;
      b = fgetc(fp);
    }
  }

  return p;
}

byte *getColourCommands(byte currentGreyVal) {
  byte *colourCommands = malloc(sizeof(byte) * 7);

  colourCommands[0] = (DATA << 6) + (currentGreyVal >> 2);

  colourCommands[1] = (DATA << 6) + (currentGreyVal & 0x30) + (currentGreyVal >> 4);

  colourCommands[2] = (DATA << 6) + ((currentGreyVal & 0x0f) << 2) + (currentGreyVal >> 6);

  colourCommands[3] = (DATA << 6) + (currentGreyVal & 0x3f);

  colourCommands[4] = (DATA << 6) + 63u;

  colourCommands[5] = (DATA << 6) + 3u;

  colourCommands[6] = (TOOL << 6) + COLOUR;

  return colourCommands;
}

byte *getPositionCommands(byte i, byte j) {
  byte *commands = malloc(sizeof(byte) * 7);
  int c = 0;

  if (j > 127) {
    commands[c] = (DATA << 6) + ((j & 0xc0) >> 6);
    c++;
  }

  commands[c] = (DATA << 6) + (j & 0x3f);
  c++;

  commands[c] = (TOOL << 6) + TARGETX;
  c++;

  if (i > 127) {
    commands[c] = (DATA << 6) + ((i & 0xc0) >> 6);
    c++;
  }

  commands[c] = (DATA << 6) + (j & 0x3f);
  c++;

  commands[c] = (TOOL << 6) + TARGETY;
  c++;

  commands[c] = (DY << 6);
  c++;

  byte *positionCommands = malloc(sizeof(byte) * c);

  memcpy(positionCommands, commands, c * sizeof(byte));

  return positionCommands;
}

byte *getColourAndPositionCommands(byte currentGreyVal, byte i, byte j) {
  int c = 5;
  if (i > 127) c++;
  if (j > 127) c++;

  byte *colourCommands = getColourCommands(currentGreyVal);
  byte *positionCommands = getPositionCommands(i, j);
  byte *colourAndPositionCommands = malloc(sizeof(byte) * (7 + c));

  memcpy(colourAndPositionCommands, colourCommands, 7 * sizeof(byte));
  memcpy(colourAndPositionCommands + c, positionCommands, c * sizeof(byte));

  free(colourCommands);
  free(positionCommands);
  return colourAndPositionCommands;
}

// For a given gray value, position of pixel and whether or not it equals its
// neighbours, return list of relevant sketch commands
byte *generateCommands(byte currentGreyVal, byte i, byte j, bool equalsPrev, bool equalsNext) {
  if ((! equalsPrev) && (! equalsNext)) {
    return getColourAndPositionCommands(currentGreyVal, i, j);
  } else if (! equalsPrev && equalsNext) {
    return getColourCommands(currentGreyVal);
  } else if (equalsPrev && ! equalsNext) {
    return getPositionCommands(i, j);
  } else {
    return getColourAndPositionCommands(currentGreyVal, i, j);
  }
}

// Open-ended task 1 (converting from .pgm to .sk)
char *pgmToSk(char *fileName) {
  FILE *fp = fopen(fileName, "rb");
  checkFileError(fp, fileName);

  pgm *p = readPgmFile(fp);
  fclose(fp);

  char *name = strtok(fileName, ".");
  strcat(name, ".sk");
  fp = fopen(name, "wb");
  checkFileError(fp, name);

  byte currentGreyVal;
  for (int i = 0; i < p->h; i++) {
    for (int j = 0; j < p->w; j++) {
      currentGreyVal = p->data[i][j];
      bool equalsPrev = false;
      bool equalsNext = false;

      if (j > 0) {
        equalsPrev = (p->data[i][j-1] != p->data[i][j]) ? true : false;
      }

      if (j < 199) {
        equalsNext = (p->data[i][j] == p->data[i][j+1]) ? true : false;
      }

      // If a pixel is the same as both its neighbours, no sketch commands need
      // to be written
      if (! (equalsPrev && equalsNext)) {
        byte *commands = generateCommands(currentGreyVal, (byte)(i), (byte)(j), equalsPrev, equalsNext);

        int c = 5;
        if (i > 127) c++;
        if (j > 127) c++;

        int lengthOfCommands = ((! equalsPrev) && (! equalsNext)) ? 7 + c : c;

        fwrite(commands, 1, (lengthOfCommands / sizeof(byte)), fp);
        free(commands);
      }
    }
  }

  freePgm(p);
  fclose(fp);
  return name;
}

// ----------------------------- TESTING FUNCTIONS -----------------------------

// Takes two arrays and their sizes. Returns false if their sizes are different
// Iterates through the arrays and compares their elements, returns false if
// any corresponding elements are not equal
bool compareArrays(byte *arr1, byte *arr2, int n1, int n2) {
  if (n1 == n2) {
    for (int i = 0; i < n1; i++) {
      if (arr1[i] != arr2[i]) return false;
    }
  } else {
    return false;
  }

  return true;
}

// If a test fails, print the line number
void assert(int line, bool b) {
  if (b) return;
  printf("The test on line %d fails.\n", line);
  exit(1);
}

// Run tests over the generateCommands function
// getColourCommands, getPositionCommands and getColourAndPositionCommands
// functions are implicitly tested by testing generateCommands
void testGenerateCommands() {
  assert(__LINE__, compareArrays(generateCommands(0, 0, 56, false, false), getColourAndPositionCommands(0, 0, 56), 12, 12));
  assert(__LINE__, compareArrays(generateCommands(0, 128, 4, false, false), getColourAndPositionCommands(0, 128, 4), 13, 13));
  assert(__LINE__, compareArrays(generateCommands(0, 90, 128, false, false), getColourAndPositionCommands(0, 90, 128), 13, 13));
  assert(__LINE__, compareArrays(generateCommands(0, 128, 200, false, false), getColourAndPositionCommands(0, 128, 200), 14, 14));

  assert(__LINE__, compareArrays(generateCommands(100, 0, 0, false, true), getColourCommands(100), 7, 7));
  assert(__LINE__, compareArrays(generateCommands(255, 0, 0, false, true), getColourCommands(255), 7, 7));

  assert(__LINE__, compareArrays(generateCommands(128, 0, 0, true, false), getPositionCommands(0, 0), 5, 5));
  assert(__LINE__, compareArrays(generateCommands(128, 0, 199, true, false), getPositionCommands(0, 199), 6, 6));
  assert(__LINE__, compareArrays(generateCommands(128, 199, 0, true, false), getPositionCommands(199, 0), 6, 6));
  assert(__LINE__, compareArrays(generateCommands(128, 199, 199, true, false), getPositionCommands(199, 199), 7, 7));

  assert(__LINE__, compareArrays(generateCommands(69, 0, 0, true, true), getColourAndPositionCommands(69, 0, 0), 12, 12));
  assert(__LINE__, compareArrays(generateCommands(69, 0, 134, true, true), getColourAndPositionCommands(69, 0, 134), 13, 13));
  assert(__LINE__, compareArrays(generateCommands(69, 134, 0, true, true), getColourAndPositionCommands(69, 134, 0), 13, 13));
  assert(__LINE__, compareArrays(generateCommands(69, 134, 134, true, true), getColourAndPositionCommands(69, 134, 134), 14, 14));
}

// Run suite of testing functions
void doTesting() {
  testGenerateCommands();
  printf("All tests pass.\n");
}

int main(int n, char *args[n]) {
  if (n == 1) {
    doTesting();
  } else if (n == 2) {
    char *fileName = args[1];
    if (strstr(fileName, ".pgm")) {
      printf("File %s has been written.\n", pgmToSk(fileName));
    } else {
      printf("Unable to open file %s.\n", fileName);
      exit(1);
    }
  } else {
    printf("Please use format: ./converter myimage.pgm\nor enter no arguments for testing.\n");
  }
  return 0;
}
