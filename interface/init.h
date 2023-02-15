// Set up pcecs and initialize global data.

// Using "PCECS_INIT_H" rather than "INIT_H" since this file will be
// included externally, and programs including it may already have
// "INITIALIZATION_H" defined somewhere.
#ifndef PCECS_INIT_H
#define PCECS_INIT_H

// Does all initialization needed for pcecs.
// Creates no entities, component types or systems.
void init_pcecs(void);

#endif
