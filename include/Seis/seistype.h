#ifndef seistype_h
#define seistype_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		29-5-99
 RCS:		$Id: seistype.h,v 1.2 2001-02-13 17:16:09 bert Exp $
________________________________________________________________________

*/

#include <enums.h>

class Seis
{
public:

    enum WaveType       { P, Sh, Sv, UnknowWave };
			DeclareEnumUtils(WaveType);
    enum DataType       { Ampl, Dip, Frequency, Phase, AVOGradient, UnknowData};
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
