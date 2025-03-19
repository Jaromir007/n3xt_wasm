#include <iostream>
#include <iomanip>
#include "clipper2/clipper.h"

using namespace std;
using namespace Clipper2Lib;

int main() {
    PathsD subject = {{{100,50},{9.5,79.4},{65.5,2.4},{65.5,97.6},{9.5,20.7}}};
    PathsD clip = {{{20,20},{80,20},{80,80},{20,80}}};

    PathsD solution = Intersect(subject, clip, FillRule::NonZero);

    cout << setprecision(3) << solution << endl;

    return 0;
}
