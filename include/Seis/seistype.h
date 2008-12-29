#ifndef seistype_h
#define seistype_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		29-5-99
 RCS:		$Id: seistype.h,v 1.15 2008-12-29 11:25:00 cvsranojay Exp $
________________________________________________________________________

*/

#include "gendefs.h"
class IOPar;

namespace Seis
{

    enum SelType	{ Range, Table, Polygon };
    const char*		nameOf(SelType);
    SelType		selTypeOf(const char*);
    const char**	selTypeNames();

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
    const char*		nameOf(GeomType);
    GeomType		geomTypeOf(const char*);
    inline GeomType	geomTypeOf( bool is2d, bool isps )
			{ return is2d ? (isps?LinePS:Line) : (isps?VolPS:Vol); }
    const char**	geomTypeNames();
    void		putInPar(GeomType,IOPar&);
    bool		getFromPar(const IOPar&,GeomType&);


    enum DataType       { Ampl, Dip, Frequency, Phase, AVOGradient, 
			  Azimuth, Classification, UnknowData };
    bool		isAngle(DataType);
    const char*		nameOf(DataType);
    mGlobal DataType		dataTypeOf(const char*);
    mGlobal const char**	dataTypeNames();

    enum WaveType       { P, Sh, Sv, UnknowWave };
    const char*		nameOf(WaveType);
    WaveType		waveTypeOf(const char*);
    const char**	waveTypeNames();
};


#endif
