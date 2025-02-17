import numpy as np

"""
///////////////////////////////////////////////////////////////////////////////
// US Standard Atmosphere 1976
// *Calculates the atmospheric properties density pressure and temperature 
//	up to 85 km.
// *Extrapolation above 71 km and beyond 85 km is carried out from 71 km altitude 
// Ref: Public Domain Aeronautical Software (see Web) Fortran Code
//
// Argument Output:
//					rho=Air density - kg/m^3
//					press= Air static pressure - Pa
//					tempk= Air temperature - degKelvin
// Argument Input:
//					balt= Geometrical altitude above S.L. - m
//
// 030318 Created by Peter Zipfel
///////////////////////////////////////////////////////////////////////////////
"""

class atm1976_metric:

	def __init__(self):

		self.rho = 0.0 # Kilograms per meter cubed.
		self.p = 0.0 # Pascals.
		self.a = 0.0 # Meters per second.
		self.g = 0.0 # Meters per second squared.
		self.q = 0.0 # Pascals.
		self.tk = 0.0 # Kelvin.
		self.mach = 0.0 # Non dimensional.

		self.R = 287.0529999999999973
		self.G = 6.672999999999999859e-11
		self.EARTH_MASS = 5.972999999999999813e+24

		self.rearth = 6369.0 # //radius of the earth - km
		self.gmr = 34.163195 # //gas constant
		self.rhosl = 1.22500 # //sea level density - kg/m^3
		self.pressl = 101325. # //sea level pressure - Pa
		self.tempksl = 288.15 # //sea level temperature - dK
		self.htab=[0.0, 11.0, 20.0, 32.0, 47.0, 51.0, 71.0, 84.852] #altitude
		self.ttab=[288.15, 216.65, 216.65, 228.65, 270.65, 270.65, 214.65, 186.946] #temperature
		self.ptab=[1.0, 2.233611e-1, 5.403295e-2, 8.5666784e-3, 1.0945601e-3, 6.6063531e-4, 3.9046834e-5, 3.68501e-6]  #pressure
		self.gtab=[-6.5, 0.0, 1.0, 2.8, 0.0, -2.8, -2.0, 0.0]   #temperature gradient

	def update(self, ALT, SPEED): # Meters, Meters per second.

		delta = None
		ALT /= 1000.0 # Kilometers.
		HGT = ALT * self.rearth / (ALT + self.rearth)

		i = 0
		j = 7
		while True:
			k = int(round(((i + j) / 2), 0))
			if HGT < self.htab[k]:
				j = k
			else:
				i = k
			if j <= (i + 1):
				break

		if ALT < 84.852:

			tgrad = self.gtab[i]
			tbase=self.ttab[i]
			deltah=HGT-self.htab[i]
			tlocal=tbase+tgrad*deltah
			theta=tlocal/self.ttab[0]

			if tgrad == 0:
				delta = self.ptab[i] * np.exp(-self.gmr * deltah / tbase)
			else:
				delta = self.ptab[i] * (tbase / tlocal) ** (self.gmr / tgrad)

			sigma = delta /  theta
			
			self.rho = self.rhosl * sigma
			self.p = self.pressl * delta
			self.tk = self.tempksl * theta

		else:

			self.rho = 0.0
			self.p = 0.0
			self.tk = 186.946

		self.a = np.sqrt(1.4 * self.R * self.tk)
		self.q = 0.5 * self.rho * SPEED * SPEED
		self.mach = SPEED / self.a
		rad = (self.rearth + ALT) * 1000
		self.g = self.G * self.EARTH_MASS / (rad ** 2)

		pause = 0

"""

void atmosphere76(double &rho,double &press,double &tempk, const double balt)
{

          double rearth(6369.0); //radius of the earth - km
          double gmr(34.163195); //gas constant
          double rhosl(1.22500); //sea level density - kg/m^3
          double pressl(101325.); //sea level pressure - Pa
          double tempksl(288.15); //sea level temperature - dK
          double htab[8]={0.0, 11.0, 20.0, 32.0, 47.0, 51.0, 71.0, 84.852}; //altitude
          double ttab[8]={288.15, 216.65, 216.65, 228.65, 270.65, 270.65, 214.65, 186.946}; //temperature
          double ptab[8]={1.0, 2.233611e-1, 5.403295e-2, 8.5666784e-3, 1.0945601e-3, 6.6063531e-4, 3.9046834e-5, 3.68501e-6};  //pressure
          double gtab[8]={-6.5, 0.0, 1.0, 2.8, 0.0, -2.8, -2.0, 0.0};   //temperature gradient

          double delta(0);

          //convert geometric (m) to geopotential altitude (km)
          double alt=balt/1000; 
          double h=alt*rearth/(alt+rearth);

          //binary search determines altitude table entry i below actual altitude 
          int i(0); //offset of first value in table
          int j(7); //offset of last value in table
          for( ; ; )
          {
                    int k=(i+j)/2;     //integer division
                    if(h<htab[k])
                    {
                              j=k;
                    }

                    else
                    {
                              i=k;
                    }
                    if(j<=(i+1))
                    {
                              break;
                    }
          }
          //within stratosphere
          if(alt<84.852)
          {

                    //normalized temperature 'theta' from table look-up and gradient interpolation
                    double tgrad=gtab[i];
                    double tbase=ttab[i];
                    double deltah=h-htab[i];
                    double tlocal=tbase+tgrad*deltah;
                    double theta=tlocal/ttab[0]; 

                    //normalized pressure from hydrostatic equations 
                    if(tgrad==0)
                    {
                              delta=ptab[i]*exp(-gmr*deltah/tbase);
                    }
                    else
                    {
                              delta=ptab[i]*pow((tbase/tlocal),(gmr/tgrad));
                    }

                    //normalized density
                    double sigma=delta/theta;

                    //output
                    rho=rhosl*sigma;
                    press=pressl*delta;
                    tempk=tempksl*theta;

          }
          else
          {
                    //beyond stratosphere
                    rho=0;
                    press=0;
                    tempk=186.946;
          }

}

"""