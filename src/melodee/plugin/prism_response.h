#pragma once
#include <math.h>

enum { EQ_BANDS=4, EQ_PARAMS=12, EQ_STAGES=4 };
enum { EQ_FREQ, EQ_GAIN, EQ_Q, EQ_SHAPE, EQ_SLOPE, EQ_ROUTE,
       EQ_RANGE, EQ_THRESH, EQ_ATTACK, EQ_RELEASE, EQ_ENABLE, EQ_SOLO };
enum { EQ_BELL, EQ_LOW_SHELF, EQ_HIGH_SHELF, EQ_HIGH_PASS, EQ_LOW_PASS,
       EQ_NOTCH, EQ_BAND_PASS, EQ_TILT, EQ_ALL_PASS };
typedef struct { double b0,b1,b2,a1,a2; } EqCoefficients;
typedef struct { EqCoefficients c[EQ_STAGES]; int count; } EqCurve;

static inline double eq_hz(double v) { return 20 * pow(1000, v); }
static inline double eq_q(double v) { return .25 * pow(80, v); }
static inline EqCoefficients eq_biquad(int shape, double hz, double q, double gain)
{
    const double w=6.283185307179586*hz/44100, c=cos(w), a=sin(w)/(2*q);
    const double A=pow(10,gain/40);
    double b0=1,b1=0,b2=0,a0=1,a1=-2*c,a2=1-a;
    switch (shape) {
    case EQ_BELL: b0=1+a*A;b1=-2*c;b2=1-a*A;a0=1+a/A;a2=1-a/A;break;
    case EQ_HIGH_PASS: b0=(1+c)/2;b1=-2*b0;b2=b0;a0=1+a;break;
    case EQ_LOW_PASS: b0=(1-c)/2;b1=2*b0;b2=b0;a0=1+a;break;
    case EQ_NOTCH: b0=b2=1;b1=-2*c;a0=1+a;break;
    case EQ_BAND_PASS: b0=a;b1=0;b2=-a;a0=1+a;break;
    case EQ_ALL_PASS: b0=1-a;b1=-2*c;b2=1+a;a0=1+a;break;
    case EQ_LOW_SHELF:
    case EQ_HIGH_SHELF: {
        const double sign=shape==EQ_LOW_SHELF?1:-1;
        const double edge=2*sqrt(A)*a;
        b0=A*((A+1)-sign*(A-1)*c+edge);
        b1=sign*2*A*((A-1)-sign*(A+1)*c);
        b2=A*((A+1)-sign*(A-1)*c-edge);
        a0=(A+1)+sign*(A-1)*c+edge;
        a1=-sign*2*((A-1)+sign*(A+1)*c);
        a2=(A+1)+sign*(A-1)*c-edge;
        break;
    }
    }
    EqCoefficients result={b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};
    return result;
}

static inline EqCurve eq_curve(const double *p, double dynamic_db)
{
    EqCurve curve={{{0,0,0,0,0}},0};
    if (p[EQ_ENABLE]<.5) return curve;
    const int shape=(int)lround(8*p[EQ_SHAPE]);
    const double hz=eq_hz(p[EQ_FREQ]), q=eq_q(p[EQ_Q]);
    const double gain=48*p[EQ_GAIN]-24+dynamic_db;
    if (shape==EQ_TILT) {
        curve.c[0]=eq_biquad(EQ_LOW_SHELF,hz,q,-gain*.5);
        curve.c[1]=eq_biquad(EQ_HIGH_SHELF,hz,q,gain*.5);
        curve.count=2;
    } else {
        const int pass=shape==EQ_HIGH_PASS || shape==EQ_LOW_PASS;
        curve.count=pass ? 1+(int)lround(3*p[EQ_SLOPE]) : 1;
        for (int n=0;n<curve.count;++n) {
            // Butterworth poles for cascaded cuts, with Q controlling resonance.
            const double stage_q=pass ? q*.70710678/cos(3.141592653589793*(2*n+1)/(4*curve.count)) : q;
            curve.c[n]=eq_biquad(shape,hz,stage_q,gain);
        }
    }
    return curve;
}

static inline double eq_magnitude(const EqCurve *curve,double hz)
{
    const double w=6.283185307179586*hz/44100;
    const double c=cos(w),s=sin(w),c2=cos(2*w),s2=sin(2*w);
    double db=0;
    for(int j=0;j<curve->count;++j) {
        const EqCoefficients b=curve->c[j];
        const double nr=b.b0+b.b1*c+b.b2*c2, ni=b.b1*s+b.b2*s2;
        const double dr=1+b.a1*c+b.a2*c2, di=b.a1*s+b.a2*s2;
        db+=10*log10(fmax(1e-12,(nr*nr+ni*ni)/fmax(1e-18,dr*dr+di*di)));
    }
    return db;
}

static inline EqCurve eq_fixed_curve(unsigned section,double frequency,double gain_or_slope,double q_or_enable)
{
    static const int shapes[]={EQ_HIGH_PASS,EQ_LOW_SHELF,EQ_BELL,EQ_HIGH_SHELF,EQ_LOW_PASS};
    const int cut=section==0 || section==4;
    double controls[EQ_PARAMS]={0};
    controls[EQ_FREQ]=frequency;
    controls[EQ_SHAPE]=shapes[section]/8.;
    controls[EQ_GAIN]=cut ? .5 : gain_or_slope;
    controls[EQ_SLOPE]=gain_or_slope;
    controls[EQ_Q]=cut ? .237 : q_or_enable;
    controls[EQ_ENABLE]=cut ? q_or_enable : 1.;
    return eq_curve(controls,0);
}
