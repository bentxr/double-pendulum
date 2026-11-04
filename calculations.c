#include <math.h>
#include "calculations.h"


Vector vectorTimesDouble(Vector *v, double d) {
    return (Vector){v->a * d, 
                    v->a_ * d,
                    v->b * d,
                    v->b_ * d};
}

Vector vectorPlusVector(Vector *v1, Vector *v2) {
    return (Vector){v1->a + v2->a, 
                    v1->a_ + v2->a_,
                    v1->b + v2->b,
                    v1->b_ + v2->b_};
}

// Updates the state vector with the integrated values
void rungekutta(f_type f, double time, Vector *stateVector, double dt, Constraints *system) {

    Vector k1 = f(time, stateVector, system);

    // Vector way of doing  k2_ = {t + dt/2, v + (dt/2)*k1}
    Vector k2_0 = vectorTimesDouble(&k1, dt/2);
    Vector k2_1 = vectorPlusVector(stateVector, &k2_0);
    Vector k2 = f(time + dt/2, &k2_1, system);

    // Vector way of doing k3_ = {t + dt/2, v + (dt/2)*k2}
    Vector k3_0 = vectorTimesDouble(&k2, dt/2);
    Vector k3_1 = vectorPlusVector(stateVector, &k3_0);
    Vector k3 = f(time + dt/2, &k3_1, system);

    // Vector way of doing k4_ = {t + dt, v + dt*k3}
    Vector k4_0 = vectorTimesDouble(&k3, dt); 
    Vector k4_1 = vectorPlusVector(stateVector, &k4_0);
    Vector k4 = f(time + dt, &k4_1, system);

    // The new state is approximated with v + (dt/6)*(k1+2*k2+2*k3+k4)
    Vector new_state = {
        .a = stateVector->a + (dt/6) * (k1.a + 2*k2.a + 2*k3.a + k4.a),
        .a_ = stateVector->a_ + (dt/6) * (k1.a_ + 2*k2.a_ + 2*k3.a_ + k4.a_),
        .b = stateVector->b + (dt/6) * (k1.b + 2*k2.b + 2*k3.b + k4.b),
        .b_ = stateVector->b_ + (dt/6) * (k1.b_ + 2*k2.b_ + 2*k3.b_ + k4.b_)
    };

    // The state vector is updated with the new state
    stateVector->a = new_state.a;
    stateVector->a_ = new_state.a_;
    stateVector->b = new_state.b;
    stateVector->b_ = new_state.b_;
}


// Takes positions and velocities and returns velocities and accelerations
Vector pendulumEquations(double time, Vector *stateVector, Constraints *system) {

    double l1 = system->l1;
    double l2 = system-> l2;
    double g = system->g;
    double m1 = system->m1;
    double m2 = system->m2;
    double c1 = system->c1;
    double c2 = system->c2;

    double a = stateVector->a;
    double a_ = stateVector->a_;
    double b = stateVector->b;
    double b_ = stateVector->b_;

    double d = a - b;
    double sin_d = sin(d);
    double cos_d = cos(d);
    double e = m1 + m2 * sin_d * sin_d;

    // Equations derived from the Lagrangian
    double a2_ = (m1 * g * cos(a) - m2 * sin_d * (g * sin(b) + l2 * b_ * b_ + l1 * a_ * a_ * cos_d)) / (l1 * e);
    double b2_ = sin_d * ((m1 + m2) * (g * sin(a) + l1 * a_ * a_) + m2 * l2 * b_ * b_ * cos_d) / (l2 * e);

    // Adding drag terms
    double v1 = l1 * fabs(a_);
    double v2 = sqrt(l1*l1*a_*a_ + l2*l2*b_*b_ + 2*l1*l2*a_*b_*cos_d);

    a2_ += -(c1*v1 + c2*v2*sin_d*sin_d) * a_ / e;
    b2_ += (l1/l2) * (c1*v1*m2 - c2*v2*m1) * cos_d * a_ / (m2*e) - (c2*v2/m2) * b_;

    return (Vector){stateVector->a_, a2_, stateVector->b_, b2_};
}