#ifndef seistype_h
#define seistype_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		29-5-99
 RCS:		$Id: seistype.h,v 1.9 2006-12-05 16:48:45 cvsbert Exp $
________________________________________________________________________

*/

namespace Seis
{

    enum SelType	{ All, Range, Table, TrcNrs };

    enum ReadMode	{ PreScan, Scan, Prod };

    enum GeomType	{ Vol, VolPS, Line, LinePS };
    inline bool		is2D( GeomType gt )
    			{ return gt > VolPS; }
    inline bool		is3D( GeomType gt )
    			{ return gt < Line; }
    inline bool		isPS( GeomType gt )
    			{ return gt == VolPS || gt == LinePS; }
    inline int		dimSize( GeomType gt )
			{ return gt == Line ? 2 : (gt == VolPS ? 4 : 3); }


    enum DataType       { Ampl, Dip, Frequency, Phase, AVOGradient, 
			  Azimuth, Classification, UnknowData };
    bool		isAngle(DataType);
    const char*		nameOf(DataType);
    DataType		dataTypeOf(const char*);
    const char**	dataTypeNames();

    enum WaveType       { P, Sh, Sv, UnknowWave };
    const char*		nameOf(WaveType);
    WaveType		waveTypeOf(const char*);
    const char**	waveTypeNames();
};


#endif
