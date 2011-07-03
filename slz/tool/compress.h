//***************************************************************************
// "compress.h"
// Header file for "compress.c"
//***************************************************************************
// Slz compression tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of the slz tool.
//
// The slz tool is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The slz tool is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the slz tool.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

#ifndef COMPRESS_H
#define COMPRESS_H

// Required headers
#include <stdio.h>

// Function prototypes
int compress(FILE *, FILE *, int);

#endif
