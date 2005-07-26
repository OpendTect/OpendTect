#ifndef seistype_h
#define seistype_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		29-5-99
 RCS:		$Id: seistype.h,v 1.7 2005-07-26 08:41:38 cvsbert Exp $
________________________________________________________________________

*/

namespace Seis
{

    enum SelType	{ All, Range, Table, TrcNrs };
    enum GeomType	{ Vol, VolPS, Line, LinePS };
    enum ReadMode	{ PreScan, Scan, Prod };

    enum DataType       { Ampl, Dip, Frequency, Phase, AVOGradient, 
			  Azimuth, UnknowData };
    const char*		nameOf(DataType);
    DataType		dataTypeOf(const char*);
    const char**	dataTypeNames();

    enum WaveType       { P, Sh, Sv, UnknowWave };
    const char*		nameOf(WaveType);
    WaveType		waveTypeOf(const char*);
    const char**	waveTypeNames();
};


#endif
