//#include <iostream>
//#include <cmath>
//using namespace std;

#include <stdio.h>
#include <math.h>
#include "prob.h"
  
// Returns the erf() of a value (not super precice, but ok)
//double erf(double x);
//double pdf(double x, double mu, double sigma);
//extern double cdf(double x, double mu, double sigma);
//int main();
  
erf (double x) 
{
  
  
1.061405429 * y 
		   -1.453152027) * y 
		  +1.421413741) * y 
		 -0.284496736) * y 



// Returns the probability of x, given the distribution described by mu and sigma.
  double
pdf (double x, double mu, double sigma) 
{
  
    //Constants
  static const double pi = 3.14159265;
  
								  sqrt (2 *
									pi));



// Returns the probability of [-inf,x] of a gaussian distribution
  double
cdf (double x, double mu, double sigma) 
{
  



/*
int main()
{
	
	double x, mu, sigma;
	cout << "x, mu, sigma: ";
	cin >> x >> mu >> sigma;

	cout << "PDF of x is: " << pdf(x,mu,sigma) << endl;
	cout << "CDF of x is: " << cdf(x,mu,sigma) << endl;

        x = 1;
        mu = 10;
        sigma = 2.5;
        printf ("CDF->%e\n",cdf(x,mu,sigma));

	return 0;
}
*/ 