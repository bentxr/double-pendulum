#ifndef CALCULATIONS_H
#define CALCULATIONS_H


typedef struct {

    double l1; // First section of the pendulum
    double l2; // Second section of the pendulum
    double g;  // Gravity
    double m1; // First mass
    double m2; // Second mass
    double c1; // Drag coefficient for mass 1
    double c2; // Drag coefficient for mass 2

} Constraints;

// Generalized coordinates of the system
typedef struct {

    double a;
    double a_;
    double b;
    double b_;

} Vector;


typedef Vector (*f_type)(double, Vector*, Constraints*); // definition of the type of the function for clarity



Vector vectorTimesDouble(Vector *v, double d);

Vector vectorPlusVector(Vector *v1, Vector *v2);

void rungekutta(f_type f, double time, Vector *stateVector, double dt, Constraints *system);

Vector pendulumEquations(double time, Vector *stateVector, Constraints *system);

#endif