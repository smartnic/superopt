#include <iostream>
#include <random>

using namespace std;

default_random_engine gen;

/* Function that provides the probability density given a support variable x */
double pi(double x) {
  if (x > 0.01) {
    return 1.0/x;
  } else {
    return 100.0;
  }
}

/* Proposal distribution */
double generate_y(double x) {
  normal_distribution<double> dist(x, 2.0);
  double y = dist(gen);
  return y;
}

int main() {
  double x = 10.0;
  double y;
  int p[20] = {};

  cout << "pi " << pi(x) << endl << "Dist computed:" << endl;

  for (int i=0; i<10000; i++) {
    y = generate_y(x);
    if (y >= 0 && y < 20) ++p[int(y)];
  }
  
  for (int i=0; i<20; i++)
    cout << i << ": " << p[i] << endl;
  
  return 0;
}
