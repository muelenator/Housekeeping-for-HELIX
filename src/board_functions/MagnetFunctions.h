#pragma once

#include "Core_protocol.h"
#include <iostream>
#include <cstdint>
#include <cstring>

/* This response to convert 4 byte thermistor resistances to floats */
void whatToDoIfThermistorsTest(housekeeping_hdr_t * hdr_in);

/* This response to convert 4 byte temp probes to floats */
void whatToDoIfTempProbes(housekeeping_hdr_t * hdr_in);

/* This response to convert 4 byte temp probes to floats */
void whatToDoIfFloat(housekeeping_hdr_t * hdr_in);

/* This response to convert 4 byte temp probes to floats */
void whatToDoIfPressure(housekeeping_hdr_t * hdr_in);

/* This response to convert 4 byte temp probes to floats */
void whatToDoIfFlow(housekeeping_hdr_t * hdr_in);
