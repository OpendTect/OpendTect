#ifndef process_time2depth_h
#define process_time2depth_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2009
 RCS:		$Id: process_time2depth.h,v 1.2 2009-07-22 16:01:18 cvsbert Exp $
________________________________________________________________________

-*/


//!Keys for process_time2depth.cc

mClass ProcessTime2Depth
{
public:

    static const char*	sKeyInputVolume()	{ return "Input volume"; }
    static const char*	sKeyOutputVolume()	{ return "Output volume"; }
    static const char*	sKeyVelocityModel()	{ return "Velocity model"; }
    static const char*	sKeyIsTimeToDepth()	{ return "Time to depth"; }
};


#endif
