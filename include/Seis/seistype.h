#ifndef seistype_h
#define seistype_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		29-5-99
 RCS:		$Id: seistype.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

*/

#include <enums.h>

class Seis
{
public:

    enum WaveType       { P, Sh, Sv, UnknowWave };
			DeclareEnumUtils(WaveType);
    enum DataType       { Ampl, Dip, Frequency, Phase, AVOGradient, UnknowData };
			DeclareEnumUtils(DataType);
};


#endif
