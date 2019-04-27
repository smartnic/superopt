#include <iostream>
#include <random>
#include <algorithm>
#include <string>

#define PDF_SUPPORT 40

using namespace std;

default_random_engine gen;
uniform_real_distribution<double> unidist(0.0, 1.0);

/* Function that provides the probability density given a support variable x */
double pi(double x) {
  double center = 15.0;
  double width = 5.0;
  if (x > center)
    return max(0.0, center + width - x);
  else
    return max(0.0, width - center + x);
}

/* Generate a new sample from proposal distribution */
double generate_y(double x) {
  normal_distribution<double> dist(x, 2.0);
  double y = dist(gen);
  return y;
}

/* compute acceptance function */
double alpha(double x, double y) {
  /* Use the simplified form when proposal distribution is symmetric */
  return min(1.0, pi(y) / pi(x));
}

/* Get the next MCMC sample */
double mh_next(double x) {
  double y = generate_y(x);
  double uni_sample = unidist(gen);
  if (uni_sample < alpha(x, y))
    return y;
  else
    return x;
}

int main() {
  double x = 10.0;
  double y;
  int p[PDF_SUPPORT] = {};
  int nrolls = 100000;
  int nstars = 100;
  double support_lower_end = -10.0;
  double support_higher_end = PDF_SUPPORT + support_lower_end;

  cout << "pi " << pi(x) << endl << "Dist computed:" << endl;

  /* MH iterations */
  for (int i=0; i<nrolls; i++) {
    y = mh_next(x);
    if (y >= support_lower_end && y < support_higher_end) {
      int index = int(y - support_lower_end);
      ++p[index];
    }
    x = y;
  }

  /* Print the distribution */
  for (int i=0; i<PDF_SUPPORT; i++) {
    cout << i + int(support_lower_end) << ": " << string(p[i] * nstars/nrolls, '*') << endl;
  }
  
  return 0;
}
