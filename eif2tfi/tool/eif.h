//***************************************************************************
// "eif.h"
// Header file for "eif.c"
//***************************************************************************
// EIF to TFI conversion tool
// Copyright 2015 Javier Degirolmo
//
// This file is part of eif2tfi.
//
// eif2tfi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// eif2tfi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with tfi2eif.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

#ifndef EIF_H
#define EIF_H

// Required headers
#include <stdio.h>
#include "main.h"

// Function prototypes
int read_eif(FILE *, Instrument *);

#endif
