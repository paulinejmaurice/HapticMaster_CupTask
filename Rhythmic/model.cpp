#include "model.h"
#include <iostream>

Model::Model(double massOfPendulum, double lengthOfPendulum,  double dampingInPendulum, double pendulumInitialAngle, double pendulumInitialVelocity, double gravityMagnitude)
{
	gravity = gravityMagnitude;
	pendulumMass = massOfPendulum;
	pendulumLength = lengthOfPendulum;
	pendulumDamping = dampingInPendulum;

	InitializeState(pendulumInitialAngle, pendulumInitialVelocity);
}

Model::~Model()
{
	// Do nothing, no dynamic memory allocation
}

double Model::ComputePendulumForceOnCart(double cartAcceleration)
{
	// Compute the force exerted by the pendulum on the cart (assume pendulum state is up to date)
	double forceOnCart = pendulumMass * pendulumLength * ( - cos(pendulumAngle) * pendulumAcceleration + sin(pendulumAngle) * pendulumVelocity * pendulumVelocity );
	return forceOnCart;
}

double Model::GetPendulumAngle()
{
	return pendulumAngle;
}


double Model::GetPendulumAngularVelocity()
{
	return pendulumVelocity;
}


double Model::GetPendulumAngularAcceleration()
{
	return pendulumAcceleration;
}

void Model::InitializeState(double pendulumInitialAngle, double pendulumInitialVelocity)
{
	pendulumAngle = pendulumInitialAngle;
	pendulumVelocity = pendulumInitialVelocity;
	pendulumAcceleration = 0.;
}


double Model::ComputePendulumAcceleration(double cartAcceleration, double pendAngle, double pendVel)
{
	double acceleration = - (cartAcceleration / pendulumLength * cos(pendAngle) + gravity / pendulumLength * sin(pendAngle)) - pendulumDamping / (pendulumMass * pendulumLength * pendulumLength) * pendVel;
	return acceleration;
}


void Model::UpdatePendulumState(double cartAcceleration, double integrationTimeStep)
{
	// Compute current pendulum acceleration from the cart motion, and update the pendulum state by integrating acceleration
	pendulumAcceleration = ComputePendulumAcceleration(cartAcceleration, pendulumAngle, pendulumVelocity);

	// Use Runge-Kutta to ingrate the equation of motion to give the current ball angle (theta) and angular velocity (omega)
	// RK4 avec derivee 2nd (cf Wikipedia)
	double k1 = ComputePendulumAcceleration(cartAcceleration, pendulumAngle, pendulumVelocity);
	double k2 = ComputePendulumAcceleration(cartAcceleration, pendulumAngle + 0.5 * integrationTimeStep * pendulumVelocity, pendulumVelocity + 0.5 * integrationTimeStep* k1);
	double k3 = ComputePendulumAcceleration(cartAcceleration, pendulumAngle + 0.5 * integrationTimeStep * pendulumVelocity + 0.25 * integrationTimeStep * integrationTimeStep * k1, pendulumVelocity + 0.5 * integrationTimeStep * k2);
	double k4 = ComputePendulumAcceleration(cartAcceleration, pendulumAngle + integrationTimeStep * pendulumVelocity + 0.5 * integrationTimeStep * integrationTimeStep * k2, pendulumVelocity + integrationTimeStep * k3);

	pendulumAngle = pendulumAngle + integrationTimeStep * pendulumVelocity + integrationTimeStep * integrationTimeStep / 6.0 * (k1 + k2 + k3);
	pendulumVelocity = pendulumVelocity + integrationTimeStep / 6.0 * (k1 + 2*k2 + 2*k3 +k4);
}



