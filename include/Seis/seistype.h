#ifndef seistype_h
#define seistype_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		29-5-99
 RCS:		$Id: seistype.h,v 1.4 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________

*/

#include <enums.h>

class Seis
{
public:

    enum WaveType       { P, Sh, Sv, UnknowWave };
			DeclareEnumUtils(WaveType);
    enum DataType       { Ampl, Dip, Frequency, Phase, AVOGradient, 
			  Azimuth, UnknowData };
			DeclareEnumUtils(DataType);

    class Event
    {
    public:

	enum Type	{ None, Extr, Max, Min, ZC, ZCNegPos, ZCPosNeg,
			  GateMax, GateMin };
			DeclareEnumUtils(Type);

			Event( float v=mUndefValue, float p=mUndefValue )
			: val(v), pos(p) {}

			float val, pos;
    };

};


#endif
