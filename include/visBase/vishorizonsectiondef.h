#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

// this header file is the set of const definitions used in the files related to
// horizonsection. don't include it in somewhere else in od.


const int   cMaxNrResolutions	= 10;

const int   cNoTesselationNeeded	= 0;
const int   cShouldRetesselate   = 1;
const int   cMustRetesselate     = 2;

const int   cNumberNodePerTileSide = 65;
const int   cMaximumResolution = 6;
const int   cMinInitialTimes = 3;
const char  cNoneResolution = -1;

enum	GeometryType{ Triangle = 0, Line,Point, WireFrame };


/* define the indexes of 9 tiles around this grid */
#   define LEFTUPTILE 0
#   define UPTILE 1
#   define RIGHTUPTILE 2
#   define LEFTTILE 3
#   define THISTILE 4
#   define RIGHTTILE 5
#   define LEFTBOTTOMTILE 6
#   define BOTTOMTILE 7
#   define RIGHTBOTTOMTILE 8
