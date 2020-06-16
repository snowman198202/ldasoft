/*
 *  Copyright (C) 2019 Tyson B. Littenberg (MSFC-ST12), Neil J. Cornish
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with with program; see the file COPYING. If not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 */


#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "LISA.h"
#include "Constants.h"

void print_LISA_ASCII_art(FILE *fptr)
{
    fprintf(fptr,"                                          \n");
    fprintf(fptr,"                                OOOOO     \n");
    fprintf(fptr,"                               OOOOOOO    \n");
    fprintf(fptr,"                             11111OOOOO   \n");
    fprintf(fptr,"  OOOOO            11111111    O1OOOOO    \n");
    fprintf(fptr," OOOOOOO  1111111             11OOOO      \n");
    fprintf(fptr," OOOOOOOO                    11           \n");
    fprintf(fptr," OOOOO1111                 111            \n");
    fprintf(fptr,"   OOOO 1111             111              \n");
    fprintf(fptr,"           1111    OOOOOO11               \n");
    fprintf(fptr,"              111OOOOOOOOOO               \n");
    fprintf(fptr,"                OOOOOOOOOOOO              \n");
    fprintf(fptr,"                OOOOOOOOOOOO              \n");
    fprintf(fptr,"                OOOOOOOOOOOO              \n");
    fprintf(fptr,"                 OOOOOOOOOO               \n");
    fprintf(fptr,"                   OOOOOO                 \n");
}

void interpolate_orbits(struct Orbit *orbit, double t, double *x, double *y, double *z)
{
    int i;
    
    for(i=0; i<3; i++)
    {
        x[i+1] = gsl_spline_eval(orbit->dx[i], t, orbit->acc);
        y[i+1] = gsl_spline_eval(orbit->dy[i], t, orbit->acc);
        z[i+1] = gsl_spline_eval(orbit->dz[i], t, orbit->acc);
    }
}

/*************************************************************************/
/*        Rigid approximation position of each LISA spacecraft           */
/*************************************************************************/
void analytic_orbits(struct Orbit *orbit, double t, double *x, double *y, double *z)
{
    
    //double alpha = PI2*t/YEAR;
    double alpha = PI2*t*3.168753575e-8;
    
    /*
     double beta1 = 0.;
     double beta2 = 2.0943951023932; //2.*pi/3.;
     double beta3 = 4.18879020478639;//4.*pi/3.;
     */
    
    double sa = sin(alpha);
    double ca = cos(alpha);
    
    double sa2  = sa*sa;
    double ca2  = ca*ca;
    double saca = sa*ca;
    double AUca = AU*ca;
    double AUsa = AU*sa;
    
    double sb,cb;
    
    sb = 0.0;//sin(beta1);
    cb = 1.0;//cos(beta1);
    x[1] = AUca + orbit->R*(saca*sb - (1. + sa2)*cb);
    y[1] = AUsa + orbit->R*(saca*cb - (1. + ca2)*sb);
    z[1] = -SQ3*orbit->R*(ca*cb + sa*sb);
    
    sb = 0.866025403784439;//sin(beta2);
    cb = -0.5;//cos(beta2);
    x[2] = AUca + orbit->R*(saca*sb - (1. + sa2)*cb);
    y[2] = AUsa + orbit->R*(saca*cb - (1. + ca2)*sb);
    z[2] = -SQ3*orbit->R*(ca*cb + sa*sb);
    
    sb = -0.866025403784438;//sin(beta3);
    cb = -0.5;//cos(beta3);
    x[3] = AUca + orbit->R*(saca*sb - (1. + sa2)*cb);
    y[3] = AUsa + orbit->R*(saca*cb - (1. + ca2)*sb);
    z[3] = -SQ3*orbit->R*(ca*cb + sa*sb);
    
}
void initialize_analytic_orbit(struct Orbit *orbit)
{
    //store armlenght & transfer frequency in orbit structure.
    orbit->L     = Larm;
    orbit->fstar = CLIGHT/(2.0*M_PI*Larm);
    orbit->ecc   = Larm/(2.0*SQ3*AU);
    orbit->R     = AU*orbit->ecc;
    orbit->orbit_function = &analytic_orbits;
    
}

void initialize_numeric_orbit(struct Orbit *orbit)
{
    fprintf(stdout,"==== Initialize LISA Orbit Structure ====\n\n");
    
    int n,i;
    double junk;
    
    FILE *infile = fopen(orbit->OrbitFileName,"r");
    
    //how big is the file
    n=0;
    while(!feof(infile))
    {
        /*columns of orbit file:
         t sc1x sc1y sc1z sc2x sc2y sc2z sc3x sc3y sc3z
         */
        n++;
        fscanf(infile,"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",&junk,&junk,&junk,&junk,&junk,&junk,&junk,&junk,&junk,&junk);
    }
    n--;
    rewind(infile);
    
    //allocate memory for local workspace
    orbit->Norb = n;
    double *t   = calloc(orbit->Norb,sizeof(double));
    double **x  = malloc(sizeof(double *)*3);
    double **y  = malloc(sizeof(double *)*3);
    double **z  = malloc(sizeof(double *)*3);
    for(i=0; i<3; i++)
    {
        x[i]  = calloc(orbit->Norb,sizeof(double));
        y[i]  = calloc(orbit->Norb,sizeof(double));
        z[i]  = calloc(orbit->Norb,sizeof(double));
    }
    
    //allocate memory for orbit structure
    orbit->t  = calloc(orbit->Norb,sizeof(double));
    orbit->x  = malloc(sizeof(double *)*3);
    orbit->y  = malloc(sizeof(double *)*3);
    orbit->z  = malloc(sizeof(double *)*3);
    orbit->dx = malloc(sizeof(gsl_spline *)*3);
    orbit->dy = malloc(sizeof(gsl_spline *)*3);
    orbit->dz = malloc(sizeof(gsl_spline *)*3);
    orbit->acc= gsl_interp_accel_alloc();
    
    for(i=0; i<3; i++)
    {
        orbit->x[i]  = calloc(orbit->Norb,sizeof(double));
        orbit->y[i]  = calloc(orbit->Norb,sizeof(double));
        orbit->z[i]  = calloc(orbit->Norb,sizeof(double));
        orbit->dx[i] = gsl_spline_alloc(gsl_interp_cspline, orbit->Norb);
        orbit->dy[i] = gsl_spline_alloc(gsl_interp_cspline, orbit->Norb);
        orbit->dz[i] = gsl_spline_alloc(gsl_interp_cspline, orbit->Norb);
    }
    
    //read in orbits
    for(n=0; n<orbit->Norb; n++)
    {
        //First time sample must be at t=0 for phasing
        fscanf(infile,"%lg",&t[n]);
        for(i=0; i<3; i++) fscanf(infile,"%lg %lg %lg",&x[i][n],&y[i][n],&z[i][n]);
        
        orbit->t[n] = t[n];
        
        //Repackage orbit positions into arrays for interpolation
        for(i=0; i<3; i++)
        {
            orbit->x[i][n] = x[i][n];
            orbit->y[i][n] = y[i][n];
            orbit->z[i][n] = z[i][n];
        }
    }
    fclose(infile);
    
    //calculate derivatives for cubic spline
    for(i=0; i<3; i++)
    {
        gsl_spline_init(orbit->dx[i],t,orbit->x[i],orbit->Norb);
        gsl_spline_init(orbit->dy[i],t,orbit->y[i],orbit->Norb);
        gsl_spline_init(orbit->dz[i],t,orbit->z[i],orbit->Norb);
    }
    
    //calculate average arm length
    printf("Estimating average armlengths -- assumes evenly sampled orbits\n\n");
    double L12=0.0;
    double L23=0.0;
    double L31=0.0;
    double x1,x2,x3,y1,y2,y3,z1,z2,z3;
    for(n=0; n<orbit->Norb; n++)
    {
        x1 = orbit->x[0][n];
        x2 = orbit->x[1][n];
        x3 = orbit->x[2][n];
        
        y1 = orbit->y[0][n];
        y2 = orbit->y[1][n];
        y3 = orbit->y[2][n];
        
        z1 = orbit->z[0][n];
        z2 = orbit->z[1][n];
        z3 = orbit->z[2][n];
        
        
        //L12
        L12 += sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2) );
        
        //L23
        L23 += sqrt( (x3-x2)*(x3-x2) + (y3-y2)*(y3-y2) + (z3-z2)*(z3-z2) );
        
        //L31
        L31 += sqrt( (x1-x3)*(x1-x3) + (y1-y3)*(y1-y3) + (z1-z3)*(z1-z3) );
    }
    L12 /= (double)orbit->Norb;
    L31 /= (double)orbit->Norb;
    L23 /= (double)orbit->Norb;
    
    printf("Average arm lengths for the constellation:\n");
    printf("  L12 = %g\n",L12);
    printf("  L31 = %g\n",L31);
    printf("  L23 = %g\n",L23);
    printf("\n");
    
    //are the armlenghts consistent?
    double L = (L12+L31+L23)/3.;
    printf("Fractional deviation from average armlength for each side:\n");
    printf("  L12 = %g\n",fabs(L12-L)/L);
    printf("  L31 = %g\n",fabs(L31-L)/L);
    printf("  L23 = %g\n",fabs(L23-L)/L);
    printf("\n");
    
    //store armlenght & transfer frequency in orbit structure.
    orbit->L     = L;
    orbit->fstar = CLIGHT/(2.0*M_PI*L);
    orbit->ecc   = L/(2.0*SQ3*AU);
    orbit->R     = AU*orbit->ecc;
    orbit->orbit_function = &interpolate_orbits;
    
    //free local memory
    for(i=0; i<3; i++)
    {
        free(x[i]);
        free(y[i]);
        free(z[i]);
    }
    free(t);
    free(x);
    free(y);
    free(z);
    fprintf(stdout,"=========================================\n\n");
    
}
/*************************************************************************/

void free_orbit(struct Orbit *orbit)
{
    for(int i=0; i<3; i++)
    {
        free(orbit->x[i]);
        free(orbit->y[i]);
        free(orbit->z[i]);
        free(orbit->dx[i]);
        free(orbit->dy[i]);
        free(orbit->dz[i]);
    }
    free(orbit->x);
    free(orbit->y);
    free(orbit->z);
    free(orbit->dx);
    free(orbit->dy);
    free(orbit->dz);
    free(orbit->t);
    
    free(orbit);
}

void LISA_tdi(double L, double fstar, double T, double ***d, double f0, long q, double *M, double *A, double *E, int BW, int NI)
{
    int i,j,k;
    int BW2   = BW*2;
    int BWon2 = BW/2;
    double fonfs;
    double c3, s3, c2, s2, c1, s1;
    double f;
    double X[BW2+1],Y[BW2+1],Z[BW2+1];
    double phiLS, cLS, sLS;
    double sqT=sqrt(T);
    double invfstar = 1./fstar;
    double invSQ3 = 1./SQ3;
    
    //phiLS = PI2*f0*(0.5-LonC);//1 s sampling rate
    //TODO: sampling rate is hard-coded into tdi function
    phiLS = PI2*f0*(7.5-L/CLIGHT);//15 s sampling rate
                                  //phiLS = PI2*f0*(dt/2.0-LonC);//arbitrary sampling rate
    cLS = cos(phiLS);
    sLS = sin(phiLS);
    
    for(i=1; i<=BW; i++)
    {
        k = 2*i;
        j = k-1;
        
        f = ((double)(q + i-1 - BWon2))/T;
        fonfs = f*invfstar;
        c3 = cos(3.*fonfs);  c2 = cos(2.*fonfs);  c1 = cos(fonfs);
        s3 = sin(3.*fonfs);  s2 = sin(2.*fonfs);  s1 = sin(fonfs);
        
        X[j] =	(d[1][2][j]-d[1][3][j])*c3 + (d[1][2][k]-d[1][3][k])*s3 +
        (d[2][1][j]-d[3][1][j])*c2 + (d[2][1][k]-d[3][1][k])*s2 +
        (d[1][3][j]-d[1][2][j])*c1 + (d[1][3][k]-d[1][2][k])*s1 +
        (d[3][1][j]-d[2][1][j]);
        
        X[k] =	(d[1][2][k]-d[1][3][k])*c3 - (d[1][2][j]-d[1][3][j])*s3 +
        (d[2][1][k]-d[3][1][k])*c2 - (d[2][1][j]-d[3][1][j])*s2 +
        (d[1][3][k]-d[1][2][k])*c1 - (d[1][3][j]-d[1][2][j])*s1 +
        (d[3][1][k]-d[2][1][k]);
        
        M[j] = sqT*(X[j]*cLS - X[k]*sLS);
        M[k] =-sqT*(X[j]*sLS + X[k]*cLS);
        
        //save some CPU time when only X-channel is needed
        if(NI>1)
        {
            Y[j] =	(d[2][3][j]-d[2][1][j])*c3 + (d[2][3][k]-d[2][1][k])*s3 +
            (d[3][2][j]-d[1][2][j])*c2 + (d[3][2][k]-d[1][2][k])*s2+
            (d[2][1][j]-d[2][3][j])*c1 + (d[2][1][k]-d[2][3][k])*s1+
            (d[1][2][j]-d[3][2][j]);
            
            Y[k] =	(d[2][3][k]-d[2][1][k])*c3 - (d[2][3][j]-d[2][1][j])*s3+
            (d[3][2][k]-d[1][2][k])*c2 - (d[3][2][j]-d[1][2][j])*s2+
            (d[2][1][k]-d[2][3][k])*c1 - (d[2][1][j]-d[2][3][j])*s1+
            (d[1][2][k]-d[3][2][k]);
            
            Z[j] =	(d[3][1][j]-d[3][2][j])*c3 + (d[3][1][k]-d[3][2][k])*s3+
            (d[1][3][j]-d[2][3][j])*c2 + (d[1][3][k]-d[2][3][k])*s2+
            (d[3][2][j]-d[3][1][j])*c1 + (d[3][2][k]-d[3][1][k])*s1+
            (d[2][3][j]-d[1][3][j]);
            
            Z[k] =	(d[3][1][k]-d[3][2][k])*c3 - (d[3][1][j]-d[3][2][j])*s3+
            (d[1][3][k]-d[2][3][k])*c2 - (d[1][3][j]-d[2][3][j])*s2+
            (d[3][2][k]-d[3][1][k])*c1 - (d[3][2][j]-d[3][1][j])*s1+
            (d[2][3][k]-d[1][3][k]);
            
            /*
             XLS[j] =  (X[j]*cLS-X[k]*sLS);
             XLS[k] = -(X[j]*sLS+X[k]*cLS);
             YLS[j] =  (Y[j]*cLS-Y[k]*sLS);
             YLS[k] = -(Y[j]*sLS+Y[k]*cLS);
             ZLS[j] =  (Z[j]*cLS-Z[k]*sLS);
             ZLS[k] = -(Z[j]*sLS+Z[k]*cLS);
             */
            
            A[j] =  sqT*((2.0*X[j]-Y[j]-Z[j])*cLS-(2.0*X[k]-Y[k]-Z[k])*sLS)*0.33333333;
            A[k] = -sqT*((2.0*X[j]-Y[j]-Z[j])*sLS+(2.0*X[k]-Y[k]-Z[k])*cLS)*0.33333333;
            
            E[j] =  sqT*((Z[j]-Y[j])*cLS-(Z[k]-Y[k])*sLS)*invSQ3;
            E[k] = -sqT*((Z[j]-Y[j])*sLS+(Z[k]-Y[k])*cLS)*invSQ3;
        }
    }
}

void LISA_tdi_FF(double L, double fstar, double T, double ***d, double f0, long q, double *M, double *A, double *E, int BW, int NI)
{
    int i,j,k;
    int BW2   = BW*2;
    int BWon2 = BW/2;
    double fonfs,fonfs2;
    double c3, s3, c2, s2, c1, s1;
    double f;
    double X[BW2+1],Y[BW2+1],Z[BW2+1];
    double phiSL, cSL, sSL;
    double sqT=sqrt(T);
    double invfstar = 1./fstar;
    double invSQ3 = 1./SQ3;
    
    phiSL = PIon2 - PI2*f0*(L/CLIGHT);//15 s sampling rate
    cSL = cos(phiSL);
    sSL = sin(phiSL);
    
    for(i=1; i<=BW; i++)
    {
        k = 2*i;
        j = k-1;
        
        f = ((double)(q + i-1 - BWon2))/T;
        fonfs = f*invfstar;
        fonfs2= 2.*fonfs;
        
        //TODO: make use of recursion relationships to get rid of trig calls
        c3 = cos(3.*fonfs);  c2 = cos(fonfs2);  c1 = cos(fonfs);
        s3 = sin(3.*fonfs);  s2 = sin(fonfs2);  s1 = sin(fonfs);
        
        X[j] =	(d[1][2][j]-d[1][3][j])*c3 + (d[1][2][k]-d[1][3][k])*s3 +
        (d[2][1][j]-d[3][1][j])*c2 + (d[2][1][k]-d[3][1][k])*s2 +
        (d[1][3][j]-d[1][2][j])*c1 + (d[1][3][k]-d[1][2][k])*s1 +
        (d[3][1][j]-d[2][1][j]);
        
        X[k] =	(d[1][2][k]-d[1][3][k])*c3 - (d[1][2][j]-d[1][3][j])*s3 +
        (d[2][1][k]-d[3][1][k])*c2 - (d[2][1][j]-d[3][1][j])*s2 +
        (d[1][3][k]-d[1][2][k])*c1 - (d[1][3][j]-d[1][2][j])*s1 +
        (d[3][1][k]-d[2][1][k]);
        
        M[j] = sqT*fonfs2*(X[j]*cSL - X[k]*sSL);
        //TODO: changed GB phase to match LDC, but why?
        //M[k] =-sqT*fonfs2*(X[j]*sSL + X[k]*cSL);
        M[k] = sqT*fonfs2*(X[j]*sSL + X[k]*cSL);
        
        //save some CPU time when only X-channel is needed
        if(NI>1)
        {
            Y[j] =	(d[2][3][j]-d[2][1][j])*c3 + (d[2][3][k]-d[2][1][k])*s3 +
            (d[3][2][j]-d[1][2][j])*c2 + (d[3][2][k]-d[1][2][k])*s2+
            (d[2][1][j]-d[2][3][j])*c1 + (d[2][1][k]-d[2][3][k])*s1+
            (d[1][2][j]-d[3][2][j]);
            
            Y[k] =	(d[2][3][k]-d[2][1][k])*c3 - (d[2][3][j]-d[2][1][j])*s3+
            (d[3][2][k]-d[1][2][k])*c2 - (d[3][2][j]-d[1][2][j])*s2+
            (d[2][1][k]-d[2][3][k])*c1 - (d[2][1][j]-d[2][3][j])*s1+
            (d[1][2][k]-d[3][2][k]);
            
            Z[j] =	(d[3][1][j]-d[3][2][j])*c3 + (d[3][1][k]-d[3][2][k])*s3+
            (d[1][3][j]-d[2][3][j])*c2 + (d[1][3][k]-d[2][3][k])*s2+
            (d[3][2][j]-d[3][1][j])*c1 + (d[3][2][k]-d[3][1][k])*s1+
            (d[2][3][j]-d[1][3][j]);
            
            Z[k] =	(d[3][1][k]-d[3][2][k])*c3 - (d[3][1][j]-d[3][2][j])*s3+
            (d[1][3][k]-d[2][3][k])*c2 - (d[1][3][j]-d[2][3][j])*s2+
            (d[3][2][k]-d[3][1][k])*c1 - (d[3][2][j]-d[3][1][j])*s1+
            (d[2][3][k]-d[1][3][k]);
            
            /*
             XSL[j] =  fonfs2*(X[j]*cSL-X[k]*sSL);
             XSL[k] = -fonfs2*(X[j]*sSL+X[k]*cSL);
             YSL[j] =  fonfs2*(Y[j]*cSL-Y[k]*sSL);
             YSL[k] = -fonfs2*(Y[j]*sSL+Y[k]*cSL);
             ZSL[j] =  fonfs2*(Z[j]*cSL-Z[k]*sSL);
             ZSL[k] = -fonfs2*(Z[j]*sSL+Z[k]*cSL);
             */
            
            
            //TODO: changed GB phase to match LDC, but why?
            /*
             A[j] =  sqT*fonfs2*((2.0*X[j]-Y[j]-Z[j])*cSL-(2.0*X[k]-Y[k]-Z[k])*sSL)*0.33333333;
             A[k] = -sqT*fonfs2*((2.0*X[j]-Y[j]-Z[j])*sSL+(2.0*X[k]-Y[k]-Z[k])*cSL)*0.33333333;
             
             E[j] =  sqT*fonfs2*((Z[j]-Y[j])*cSL-(Z[k]-Y[k])*sSL)*invSQ3;
             E[k] = -sqT*fonfs2*((Z[j]-Y[j])*sSL+(Z[k]-Y[k])*cSL)*invSQ3;
             */
            A[j] =  sqT*fonfs2*((2.0*X[j]-Y[j]-Z[j])*cSL-(2.0*X[k]-Y[k]-Z[k])*sSL)*0.33333333;
            A[k] =  sqT*fonfs2*((2.0*X[j]-Y[j]-Z[j])*sSL+(2.0*X[k]-Y[k]-Z[k])*cSL)*0.33333333;
            
            E[j] =  sqT*fonfs2*((Z[j]-Y[j])*cSL-(Z[k]-Y[k])*sSL)*invSQ3;
            E[k] =  sqT*fonfs2*((Z[j]-Y[j])*sSL+(Z[k]-Y[k])*cSL)*invSQ3;
            
            
            /* TODO: HORRIBLE HACK!  PUT X,Y,Z into A,E,M arrays for checking against LDC
             A[j] = sqT*fonfs2*(X[j]*cSL - X[k]*sSL);
             A[k] = sqT*fonfs2*(X[j]*sSL + X[k]*cSL);
             E[j] = sqT*fonfs2*(Y[j]*cSL - Y[k]*sSL);
             E[k] = sqT*fonfs2*(Y[j]*sSL + Y[k]*cSL);
             M[j] = sqT*fonfs2*(Z[j]*cSL - Z[k]*sSL);
             M[k] = sqT*fonfs2*(Z[j]*sSL + Z[k]*cSL);*/
            
        }
    }
}


static double ipow(double x, int n)
{
    int i;
    double xn = x;
    switch(n)
    {
        case 0:
            xn = 1.0;
            break;
        case 1:
            xn = x;
            break;
        default:
            for(i=2; i<=n; i++) xn *= x;
            break;
    }
    return xn;
}

/*
 adapted from LDC git
 https://gitlab.in2p3.fr/LISA/LDC/-/blob/master/ldc/lisa/noise/noise.py
 commit #491bf4b3
 (c) LISA 2019
*/
static void get_noise_levels(char model[], double f, double *Spm, double *Sop)
{
    if (strcmp(model, "radler") == 0)
    {
      *Spm = 2.5e-48*(1.0 + pow(f/1e-4,-2))/f/f;
      *Sop = 1.8e-37*(0.5)*(0.5)*f*f;
    }
    else if (strcmp(model, "scirdv1") == 0)
    {
        
        double fonc = PI2*f/CLIGHT;
        
        double DSoms_d = 2.25e-22;//15.e-12*15.e-12;
        double DSa_a = 9.00e-30;//3.e-15*3.e-15;
      
      double Sa_a = DSa_a * (1.0 +(0.4e-3/f)*(0.4e-3/f)) * (1.0 + pow((f/8.e-3),4)); // in acceleration

      double Sa_d = Sa_a*pow(PI2*f,-4.); // in displacement
      
      *Spm = Sa_d*(fonc)*(fonc);
      
      double Soms_d  = DSoms_d*(1. + pow(2.e-3/f, 4.) );
      double Soms_nu = Soms_d*(fonc)*(fonc);
      *Sop = Soms_nu;
    }
    /* more else if clauses */
    else /* default: */
    {
        fprintf(stderr,"Unrecognized noise model %s\n",model);
        exit(1);
    }
}

double XYZnoise_FF(double L, double fstar, double f)
{
    double Spm, Sop;
    
    get_noise_levels("scirdv1",f,&Spm,&Sop);
    
    double x = f/fstar;
    double sinx  = sin(x);
    double cosx  = cos(x);

    return 16. * sinx*sinx * ( 2.*(1.0 + cosx*cosx)*Spm + Sop );
}

double AEnoise_FF(double L, double fstar, double f)
{

    double Spm, Sop;
    
    get_noise_levels("radler",f,&Spm,&Sop);

    double x = f/fstar;
    
    double sinx  = sin(x);
    double cosx  = cos(x);
    double cos2x = cos(2.*x);
    
    return  8. * sinx*sinx * ( 2.*Spm*(3. + 2.*cosx + cos2x) + Sop*(2. + cosx) );
    
}

/*
 adapted from LDC git
 https://gitlab.in2p3.fr/LISA/LDC/-/blob/master/ldc/lisa/noise/noise.py
 commit #491bf4b3
 (c) LISA 2019
*/
double GBnoise_FF(double T, double fstar, double f)
{
    double x = f/fstar;
    double t = 4.*x*x*sin(x)*sin(x);
    
    double Ampl = 1.2826e-44;
    double alpha = 1.629667;
    double fr2 = 4.810781e-4;
    double af1 = -2.235e-1;
    double bf1 = -2.7040844;
    double afk = -3.60976122e-1;
    double bfk = -2.37822436;
    
    double fr1 = pow(10., af1*log10(T/31457280.) + bf1);
    double knee = pow(10., afk*log10(T/31457280.) + bfk);
    double SG_sense = Ampl * exp(-pow(f/fr1,alpha)) * pow(f,-7./3.) * 0.5 * (1.0 + tanh(-(f-knee)/fr2) );
    double SGXYZ = t*SG_sense;
    double SGAE  = 1.5*SGXYZ;
    return SGAE;
}

static double rednoise(double f)
{
    return 1. + 16.0*(pow((2.0e-5/f), 10.0)+ ipow(1.0e-4/f,2));
}

double AEnoise(double L, double fstar, double f)
{
    //Power spectral density of the detector noise and transfer frequency
    
    double fonfstar = f/fstar;
    double trans = ipow(sin(fonfstar),2.0);
    
    return (16.0/3.0)*trans*( (2.0+cos(fonfstar))*(SPS + SLOC) + 2.0*( 3.0 + 2.0*cos(fonfstar) + cos(2.0*fonfstar) ) * ( SLOC/2.0 + SACC/ipow(PI2*f,4)*rednoise(f) ) ) / ipow(2.0*L,2);
    
    
}

double XYZnoise(double L, double fstar, double f)
{
    //Power spectral density of the detector noise and transfer frequency
    
    double fonfstar = f/fstar;
    double trans = ipow(sin(fonfstar),2.0);
    
    return (4.0)*trans*( (4.0)*(SPS + SLOC) + 8.0*( 1.0 +                ipow(cos(fonfstar),2) ) * ( SLOC/2.0 + SACC*(1./(ipow(PI2*f,4)))*rednoise(f) ) ) / ipow(2.0*L,2);
}

double GBnoise(double T, double f)
{
    /* Fits to confusion noise from Cornish and Robson https://arxiv.org/pdf/1703.09858.pdf */
    double A = 1.8e-44;
    double alpha;
    double beta;
    double kappa;
    double gamma;
    double fk;
    
    //map T to number of years
    double Tyear = T/YEAR;
    
    if(Tyear>3)
    {
        alpha = 0.138;
        beta  = -221.0;
        kappa = 512.0;
        gamma = 1680.0;
        fk    = 0.00113;
    }
    else if(Tyear>1.5)
    {
        alpha = 0.165;
        beta  = 299.;
        kappa = 611.;
        gamma = 1340.;
        fk    = 0.00173;
    }
    else if(Tyear>0.75)
    {
        alpha = 0.171;
        beta  = 292.0;
        kappa = 1020.;
        gamma = 1680.0;
        fk    = 0.00215;
    }
    else
    {
        alpha = 0.133;
        beta  = 243.0;
        kappa = 482.0;
        gamma = 917.0;
        fk    = 0.00258;
    }
    return A*pow(f,-7./3.)*exp(-pow(f,alpha) + beta*f*sin(kappa*f))*(1. + tanh(gamma*(fk-f)));
}

//double AEnoise(double L, double fstar, double f)
//{
//  //Power spectral density of the detector noise and transfer frequency
//  double Sn;
//
//
//  // Calculate the power spectral density of the detector noise at the given frequency
//  Sn = 16.0/3.0*ipow(sin(f/fstar),2.0)*( ( (2.0+cos(f/fstar))*Sps + 2.0*(3.0+2.0*cos(f/fstar)+cos(2.0*f/fstar))*Sacc*(1.0/ipow(2.0*M_PI*f,4))) / ipow(2.0*L,2.0));
//
//  return Sn;
//}
//
//void instrument_noise(double f, double *SAE, double *SXYZ)
//{
//  //Power spectral density of the detector noise and transfer frequency
//  double Sn, red, confusion_noise;
//  double Sloc;
//  double f1, f2;
//  double A1, A2, slope;
//  FILE *outfile;
//
//
//  red = 16.0*(pow((2.0e-5/f), 10.0)+ (1.0e-4/f)*(1.0e-4/f));
//
//  Sloc = 2.89e-24;
//
//  // Calculate the power spectral density of the detector noise at the given frequency
//
//  *SAE = 16.0/3.0*pow(sin(f/fstar),2.0)*( (2.0+cos(f/fstar))*(Sps+Sloc) + 2.0*(3.0+2.0*cos(f/fstar)+cos(2.0*f/fstar))*(Sloc + Sacc/pow(2.0*pi*f,4.0)*(1.0+red)) ) / pow(2.0*L,2.0);
//
//  *SXYZ = 4.0*pow(sin(f/fstar),2.0)*( 4.0*(Sps+Sloc) + 8.0*(1.0+pow(cos(f/fstar),2.0))*(Sloc + Sacc/pow(2.0*pi*f,4.0)*(1.0+red)) ) / pow(2.0*L,2.0);
//  
//}
