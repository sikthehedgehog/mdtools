//***************************************************************************
// "vgi.h"
// Header file for "vgi.c"
//***************************************************************************
// VGI to EIF conversion tool
// Copyright 2012 Javier Degirolmo
//
// This file is part of vgi2eif.
//
// vgi2eif is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// vgi2eif is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with vgi2eif.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

#ifndef VGI_H
#define VGI_H

// Required headers
#include <stdio.h>
#include "main.h"

// Function prototypes
int read_vgi(FILE *, Instrument *);

#endif
