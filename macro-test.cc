#include <iostream>

using namespace std;

#define DOGS { C(JACK_RUSSELL), C(BULL_TERRIER), C(ITALIAN_GREYHOUND) }

#undef C
#define C(a) ENUM_##a
enum dog_enums DOGS;
#undef C
#define C(a) #a
string dog_strings[] = DOGS;

string dog_to_string(enum dog_enums dog)
{
  return dog_strings[dog];
}


/* Inputs x, y, z must be side-effect-free expressions. */
#define ADDXY_EXPR(x, y, z) (z GENMODE x + y)

#undef GENMODE
#define GENMODE =

int compute_add(int a, int b, int c) {
  return ADDXY_EXPR(a, b, c);
}

#undef GENMODE
#define GENMODE ==

bool predicate_add(int a, int b, int c) {
  return ADDXY_EXPR(a, b, c);
}

int main() {
  cout << dog_to_string(ENUM_ITALIAN_GREYHOUND) << endl;

  int a = 4, b = 5, c = 10;
  cout << compute_add(a, b, c) << endl;
  cout << predicate_add(a, b, c) << endl;
  c = compute_add(a, b, c);
  cout << predicate_add(a, b, c) << endl;

  return 0;
}
