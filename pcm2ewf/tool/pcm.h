//***************************************************************************
// "pcm.h"
// Header file for "pcm.c"
//***************************************************************************
// Raw PCM to EWF conversion tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of pcm2ewf.
//
// pcm2ewf is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pcm2ewf is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pcm2ewf.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

#ifndef PCM_H
#define PCM_H

// Required headers
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// Function prototypes
int read_pcm(FILE *, uint8_t **, size_t *);

#endif
