//
//  philipshue_color.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 06/03/2015.
//
//

#ifndef __philipshue_color_h
#define __philipshue_color_h

#define HUE_LCT001  0
#define HUE_LCT002  1
#define HUE_LCT003  2
#define HUE_LLC005  3
#define HUE_LLC006  4
#define HUE_LLC007  5
#define HUE_LST001  6
#define HUE_LLC001  7
#define HUE_LLC011  8
#define HUE_LLC012  9
#define HUE_LLC013 10
#define HUE_LLC010 11
#define HUE_LWB004 12
#define HUE_LLM001 13
#define HUE_LLM010 14
#define HUE_LLM011 15
#define HUE_LLM012 16
#define HUE_HBL001 17
#define HUE_HBL002 18
#define HUE_HBL003 19
#define HUE_HEL001 20
#define HUE_HEL002 21
#define HUE_HIL001 22
#define HUE_HIL002 23
#define HUE_HML001 24
#define HUE_HML002 25
#define HUE_HML003 26
#define HUE_HML007 27

typedef struct point_s {
   float x;
   float y;
} point_t;


typedef struct xyBriColor_s {
   float x;
   float y;
   float bri;
} xyBriColor_t;


typedef struct rgbColor_s {
   float r;
   float g;
   float b;
} rgbColor_t;


int16_t getHueGamutID(char *model);

int16_t rgbToXyBriColor(rgbColor_t *rgbColor, xyBriColor_t *xyBriColor);

int16_t xyForGamutID(point_t *xy, int16_t gamutID, point_t *xy_r);

int16_t xyBriColorForGamutID(xyBriColor_t *xyb, int8_t gamutID, xyBriColor_t *xyb_r);

uint32_t rgbUint32FromXyBriColor(xyBriColor_t *xyb);
uint32_t rgbUint32FromX_Y_B(float x, float y, float bri);

int16_t X_Y_B_ForGamutIDFromRgbUint32(uint32_t rgb, int16_t gamutID, float *x, float *y, float *bri);
int16_t X_Y_B_ForModelStringFromRgbString(char *rgbStr, char *model, float *x, float *y, float *bri);

#endif
