#ifndef vishorizonsectiondef_h
#define vishorizonsectiondef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________

-*/


// this header file is the set of const definitions used in the files related to
// horizonsection. don't include it in somewhere else in od.


const int   cMaxNrTiles		= 15;
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

#define mDefineRCRange(section, op )\
    const StepInterval<int> rrg = section op userchangedisplayrg_ ?\
    section op displayrrg_ : section op geometry_->rowRange(); \
    const StepInterval<int>  crg = section op userchangedisplayrg_ ?\
    section op displaycrg_ : section op geometry_->colRange();\

#endif
