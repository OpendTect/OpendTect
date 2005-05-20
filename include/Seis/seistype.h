#ifndef seistype_h
#define seistype_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		29-5-99
 RCS:		$Id: seistype.h,v 1.6 2005-05-20 11:50:10 cvsbert Exp $
________________________________________________________________________

*/

#include "enums.h"

class Seis
{
public:

    enum WaveType       { P, Sh, Sv, UnknowWave };
			DeclareEnumUtils(WaveType);
    enum DataType       { Ampl, Dip, Frequency, Phase, AVOGradient, 
			  Azimuth, UnknowData };
			DeclareEnumUtils(DataType);

};


#endif
