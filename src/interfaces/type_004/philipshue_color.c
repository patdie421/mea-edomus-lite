/*
 *  philipshue_color.c
 *
 *  Created by Patrice Dietsch on 04/03/15.
 *  Copyright 2012 -. All rights reserved.
 *
 */

//
// initial conversion from Javascript lib get from https://github.com/Q42/hue-color-converter
// thanks to Tom Lokhorst
//
// Triangle/model updated from http://www.developers.meethue.com/documentation/supported-lights
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <string.h>

#include "philipshue_color.h"

#define RED   0
#define GREEN 1
#define BLUE  2


struct hueModel_s
{
   char *model;
   int16_t id;
   int8_t gamut;
};


struct hueModel_s hueModels[] = {
   { "LCT001", HUE_LCT001, 2},
   { "LCT002", HUE_LCT002, 2},
   { "LCT003", HUE_LCT003, 2},
   { "LLC001", HUE_LLC001, 1},
   { "LLC005", HUE_LLC005, 1},
   { "LLC006", HUE_LLC006, 1},
   { "LLC007", HUE_LLC007, 1},
   { "LLC011", HUE_LLC011, 1},
   { "LLC012", HUE_LLC012, 1},
   { "LLC013", HUE_LLC013, 1},
   { "LST001", HUE_LST001, 1},
   { "LLC010", HUE_LLC010, 1},
   { "LWB004", HUE_LWB004,-1},
   { "LLM001", HUE_LLM001, 2},
   { "LLM010", HUE_LLM010, 3},
   { "LLM011", HUE_LLM011, 3},   
   { "LLM012", HUE_LLM012, 3},
   { "HBL001", HUE_HBL001, 1},
   { "HBL002", HUE_HBL002, 1},
   { "HBL003", HUE_HBL003, 1},
   { "HEL001", HUE_HEL001, 1},
   { "HEL002", HUE_HEL002, 1},
   { "HIL001", HUE_HIL001, 1},
   { "HIL002", HUE_HIL002, 1},
   { "HML001", HUE_HML001, 0},
   { "HML002", HUE_HML002, 0},
   { "HML003", HUE_HML003, 0},
   { "HML007", HUE_HML007, 0},
   { NULL, -1, -1 }
};


int16_t _triangleForGamutID(int8_t gamut_id, point_t triangle[])
{
   switch(gamut_id)
   {

      case 1:
         triangle[RED].x   = 0.704;
         triangle[RED].y   = 0.296;
         triangle[GREEN].x = 0.2151;
         triangle[GREEN].y = 0.7106;
         triangle[BLUE].x  = 0.138;
         triangle[BLUE].y  = 0.08;
         break;
      case 2:
         triangle[RED].x   = 0.675;
         triangle[RED].y   = 0.322;
         triangle[GREEN].x = 0.409;
         triangle[GREEN].y = 0.518;
         triangle[BLUE].x  = 0.167;
         triangle[BLUE].y  = 0.04;
         break;
      default:
         triangle[RED].x   = 1.0;
         triangle[RED].y   = 0.0;
         triangle[GREEN].x = 0.0;
         triangle[GREEN].y = 1.0;
         triangle[BLUE].x  = 0.0;
         triangle[BLUE].y  = 0.0;
//         return -1;
         break;
   }
   return 0;
}


float _crossProduct(point_t *p1, point_t *p2)
{
   return (p1->x * p2->y - p1->y * p2->x);
}


int16_t _closestPointOnLine(point_t *a, point_t *b, point_t *p, point_t *res)
{
   point_t ap, ab;

   ap.x = p->x - a->x;
   ap.y = p->y - a->y;
   
   ab.x = b->x - a->x;
   ab.y = b->y - a->y;

   
   float ab2 = ab.x * ab.x + ab.y * ab.y;
   float ap_ab = ap.x * ab.x + ap.y * ab.y;

   float t = ap_ab / ab2;
//   t = fmaxf (1, fmaxf(0, t));
   if (t < 0.0f)
      t = 0.0f;
   else if (t > 1.0f)
      t = 1.0f;
    
   res->x = a->x + ab.x * t;
   res->y = a->y + ab.y * t;
   
   return 0;
}


float _distance(point_t *p1, point_t *p2)
{
   float dx = p1->x - p2->x;
   float dy = p1->y - p2->y;

   return  sqrtf(dx * dx + dy * dy);
}


int16_t _isPointInTriangle(point_t *p, point_t triangle[])
{
   point_t *red   = &triangle[RED];
   point_t *green = &triangle[GREEN];
   point_t *blue  = &triangle[BLUE];
   
   point_t v1,v2,q;
   
   v1.x = green->x - red->x;
   v1.y = green->y - red->y;

   v2.x = blue->x - red->x;
   v2.y = blue->y - red->y;
   
   q.x  = p->x - red->x;
   q.y =  p->y - red->y;

   float s = _crossProduct(&q, &v2) / _crossProduct(&v1, &v2);
   float t = _crossProduct(&v1, &q) / _crossProduct(&v1, &v2);
   
   return (s >= 0.0) && (t >= 0.0) && (s + t <= 1.0);
}


int16_t getHueGamutID(char *model)
{
   for(int i=0;hueModels[i].model;i++)
   {
      if(strcmp(hueModels[i].model, model)==0)
         return hueModels[i].gamut;
   }
   
   return -1;
}


int16_t rgbColorToXyBriColor(rgbColor_t *rgbColor, xyBriColor_t *xyBriColor)
{
   if(   rgbColor->r < 0 || rgbColor->r > 1
      || rgbColor->g < 0 || rgbColor->g > 1
      || rgbColor->b < 0 || rgbColor->b > 1 )
   {
      return -1;
   }
   
   float red   = rgbColor->r;
   float green = rgbColor->g;
   float blue  = rgbColor->b;
   
   float r = (red   > 0.04045) ? powf((red   + 0.055) / (1.0 + 0.055), 2.4) : (red   / 12.92);
   float g = (green > 0.04045) ? powf((green + 0.055) / (1.0 + 0.055), 2.4) : (green / 12.92);
   float b = (blue  > 0.04045) ? powf((blue  + 0.055) / (1.0 + 0.055), 2.4) : (blue  / 12.92);
   
   // wide gamut conversion D65
   float X = r * 0.649926 + g * 0.103455 + b * 0.197109;
   float Y = r * 0.234327 + g * 0.743075 + b * 0.022598;
   float Z = r * 0.000000 + g * 0.053077 + b * 1.035763;
   
   float cx = X / (X + Y + Z);
   float cy = Y / (X + Y + Z);
   
   if (isnan(cx)) {
      cx = 0.0f;
   }

   if (isnan(cy)) {
      cy = 0.0f;
   }

   xyBriColor->x = cx;
   xyBriColor->y = cy;
   xyBriColor->bri = Y;
   
   return 0;
}


int16_t xyForGamutID(point_t *xy, int16_t gamutID, point_t *xy_r)
{
   point_t triangle[3];
  
   _triangleForGamutID(gamutID, triangle);

   if(_isPointInTriangle(xy, triangle)) {
      memcpy(xy_r,xy,sizeof(point_t));
      return 0;
   }
   
   point_t pAB, pAC, pBC;
   _closestPointOnLine(&triangle[RED],   &triangle[GREEN], xy, &pAB);
   _closestPointOnLine(&triangle[BLUE],  &triangle[RED],   xy, &pAC);
   _closestPointOnLine(&triangle[GREEN], &triangle[BLUE],  xy, &pBC);

   float dAB = _distance(xy, &pAB);
   float dAC = _distance(xy, &pAC);
   float dBC = _distance(xy, &pBC);
   
   float lowest = dAB;
   point_t *closestPoint = &pAB;
   
   if(dAC < lowest)
   {
      lowest = dAC;
      closestPoint = &pAC;
   }
   if(dBC < lowest)
   {
//      lowest = dBC;
      closestPoint = &pBC;
   }
  
   xy_r->x = closestPoint->x;
   xy_r->y = closestPoint->y;
   
   return 0;
}


int16_t xyBriColorForGamutID(xyBriColor_t *xyb, int8_t gamutID, xyBriColor_t *xyb_r)
{
   point_t xy, xy_r;
   
   xy.x = xyb->x;
   xy.y = xyb->y;
   
   xyForGamutID(&xy, gamutID, &xy_r);
   
   xyb_r->x = xy.x;
   xyb_r->y = xy.y;
   xyb_r->bri = xyb->bri;
   
   return 0;
}


uint32_t rgbUint32FromXyBriColor(xyBriColor_t *xyb)
{
   if(0 > xyb->x || xyb->x > .8)
   {
      return 0xFFFFFFFF;
   }
   
   if(0 > xyb->y || xyb->y > 1)
   {
      return 0xFFFFFFFF;
   }
   
   if(0 > xyb->bri || xyb->bri > 1)
   {
      return 0xFFFFFFFF;
   }
   
   float x = xyb->x;
   float y = xyb->y;

   float z = 1.0 - x - y;
   float Y = xyb->bri;
   float X = (Y / y) * x;
   float Z = (Y / y) * z;
   float r = X * 1.612 - Y * 0.203 - Z * 0.302;
   float g = -X * 0.509 + Y * 1.412 + Z * 0.066;
   float b = X * 0.026 - Y * 0.072 + Z * 0.962;

   r = r <= 0.0031308 ? 12.92 * r : (1.0 + 0.055) * powf(r, (1.0 / 2.4)) - 0.055;
   g = g <= 0.0031308 ? 12.92 * g : (1.0 + 0.055) * powf(g, (1.0 / 2.4)) - 0.055;
   b = b <= 0.0031308 ? 12.92 * b : (1.0 + 0.055) * powf(b, (1.0 / 2.4)) - 0.055;

   r = fmaxf(0, fminf(1, r));
   g = fmaxf(0, fminf(1, g));
   b = fmaxf(0, fminf(1, b));
   
   uint32_t R=(uint32_t)(r * 255.0);
   uint32_t G=(uint32_t)(g * 255.0);
   uint32_t B=(uint32_t)(b * 255.0);
   
   uint32_t RGB = R << 16 | G << 8 | B;
   
   return RGB;
}


uint32_t rgbUint32FromX_Y_B(float x, float y, float bri)
{
   xyBriColor_t xyb;
   
   xyb.x = x;
   xyb.y = y;
   xyb.bri = bri;
   
   return rgbUint32FromXyBriColor(&xyb);
}


int16_t X_Y_B_ForGamutIDFromRgbUint32(uint32_t rgb, int16_t gamutID, float *x, float *y, float *bri)
{
   if(rgb > 0x00FFFFFF)
      return -1;
   rgbColor_t rgbColor;
   
   rgbColor.r= (float)((rgb & 0x00FF0000) >> 16) / 255.0;
   rgbColor.g= (float)((rgb & 0x0000FF00) >>  8) / 255.0;
   rgbColor.b= (float)((rgb & 0x000000FF)      ) / 255.0;

   xyBriColor_t xyBri_Color;
   xyBriColor_t xyBri_Color_corrected;
   
   if(rgbColorToXyBriColor(&rgbColor, &xyBri_Color)<0)
      return -1;

   xyBriColorForGamutID(&xyBri_Color, gamutID, &xyBri_Color_corrected);
 
   *x = xyBri_Color.x;
   *y = xyBri_Color.y;
   *bri = xyBri_Color.bri;
   
   return 0;
}


int16_t X_Y_B_ForModelStringFromRgbString(char *rgbStr, char *model, float *x, float *y, float *bri)
{
   uint32_t rgb;
   int n,l;
   
   n=sscanf(rgbStr,"#%6x%n",&rgb, &l);
   if(n!=1 && l!=strlen(rgbStr))
      return -1;

   int16_t gamutID = getHueGamutID(model);
   
   return X_Y_B_ForGamutIDFromRgbUint32(rgb, gamutID, x, y, bri);
}


/* 
De l'auteur de l'algorithme : Start with a temperature, in Kelvin, somewhere between 1000 and 40000.
(Other values may work, but I can't make any promises about the quality of the algorithm's estimates 
above 40000 K.)
Note also that the temperature and color variables need to be declared as floating-point.

Dans le cas des ampoules Philips Hue ct est compris entre 153 (6535K) et 500 (2000K) donc pas de
risque de dégradation de qualité (voir http://www.developers.meethue.com/documentation/core-concepts).
*/
int16_t ctUint16ToRgbColor(uint16_t ct, rgbColor_t *rgb)
{
   double t;
   double r,g,b;
   
   if(ct == 0)
      return -1;
      
//   ct = 1000000 / t => t = 100000 / ct;
   t=( 1000000.0 / (double)ct ) / 100;
   
   if(t<66)
      r=255;
   else
   {
      r = t - 60;
      r = 329.698727446 * pow(r,-0.1332047592);
      if(r<0)
         r=0;
      if(r>255)
         r=255;
   }

   if(t<66)
   {
      g = 99.4708025861 * log(t) - 161.1195681661;
      if(g<0)
         g=0;
      if (g>255)
         g=255;
   }
   else
   {
      g = t - 60;
      g = 288.1221695283 * pow(g,-0.0755148492);
      if(g<0)
         g=0;
      if(g>255)
         g=255;
   }

   if(t >= 66)
      b = 255;
   else
   {
      if(t <= 19)
         b = 0;
      else
      {
         b = t - 10;
         b = 138.5177312231 * log(b) - 305.0447927307;
         if (b < 0)
            b=0;
         if (b > 255)
         b = 255;
      }
   }
   
   rgb->r = (float)(r / 255);
   rgb->g = (float)(g / 255);
   rgb->b = (float)(b / 255);
   
   return 0;
}


double _fmax3(double a, double b, double c)
{
   double max = a;
   
   if(b > max)
      max = b;
   if(c > max)
      max = c;
      
   return max;
}


double _fmin3(double a, double b, double c)
{
   double min = a;
   
   if(b < min)
      min = b;
   if(c < min)
      min = c;
      
   return min;
}


// https://bgrins.github.io/TinyColor/docs/tinycolor.html
int16_t RgbColorTo_H_S_B(rgbColor_t rgb, uint16_t *h, uint8_t *s, uint8_t *v)
{
   double r=rgb.r;
   double g=rgb.g;
   double b=rgb.b;

   double max = _fmax3(r, g, b), min = _fmin3(r, g, b);
   double _h = 0.0, _s = 0.0, _v = max;

   double d = max - min;
   _s = max == 0 ? 0 : d / max;

   if(max == min) {
      _h = 0;
   }
   else
   {
      if(r == max)
         _h = (g - b) / d + (g < b ? 6 : 0);
      else if(g == max)
         _h = (b - r) / d + 2;
      else if(b == max)
         _h = (r - g) / d + 4;
      _h /= 6;
   }
   
   *h = (uint16_t)(_h * 65535.0);
   *s = (uint8_t)(_s * 255.0);
   *v = (uint8_t)(_v * 255.0);

   return 0;
}


int16_t H_S_B_ToRgbColor(uint16_t h, uint8_t s, uint8_t v, rgbColor_t *rgb)
{
   double r = 0.0;
   double g = 0.0;
   double b = 0.0;
   
   double _h = (double)h / 65535.0 * 6;
   double _s = (double)s / 255.0;
   double _v = (double)v / 255.0;

   double i = floor(_h);
   double f = _h - i;
   double p = _v * (1 - _s);
   double q = _v * (1 - f * _s);
   double t = _v * (1 - (1 - f) * _s);
   double mod = (double)((long)i % 6);
    
   switch((int)mod)
   {
      case 0: r = _v; g = t;  b = p;  break;
      case 1: r = q;  g = _v; b = p;  break;
      case 2: r = p;  g = _v; b = t;  break;
      case 3: r = p;  g = q;  b = _v; break;
      case 4: r = t;  g = p;  b = _v; break;
      case 5: r = _v; g = p;  b = q;  break;
      default: return -1;
   }
    
   rgb->r = r;
   rgb->g = g;
   rgb->b = b;
    
   return 0;
}


/*
int main(int argc, char *argv[])
{
   float x,y,bri;
   
   X_Y_Bri_ForModelStringFromRgbString(argv[1], argv[2], &x, &y, &bri);
   
   printf("x=%f, y=%f, bri=%f\n",x,y,bri);
   
   printf("reverse = %x vs original = %s\n", rgbUint32FromX_Y_B(x,y,bri), argv[1]);
}
*/