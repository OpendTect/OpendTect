#ifndef process_time2depth_h
#define process_time2depth_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2009
 RCS:		$Id: process_time2depth.h,v 1.1 2009-04-05 14:51:10 cvskris Exp $
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
