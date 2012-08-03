#ifndef seistype_h
#define seistype_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		29-5-99
 RCS:		$Id: seistype.h,v 1.18 2012-08-03 13:00:38 cvskris Exp $
________________________________________________________________________

*/

#include "seismod.h"
#include "gendefs.h"
class IOPar;

namespace Seis
{

    mGlobal(Seis) enum SelType	{ Range, Table, Polygon };
    mGlobal(Seis) const char*		nameOf(SelType);
    mGlobal(Seis) SelType		selTypeOf(const char*);
    mGlobal(Seis) const char**	selTypeNames();

    enum ReadMode	{ PreScan, Scan, Prod };

    enum GeomType	{ Vol, VolPS, Line, LinePS };
    mGlobal(Seis) inline bool		is2D( GeomType gt )
    			{ return gt > VolPS; }
    mGlobal(Seis) inline bool		is3D( GeomType gt )
    			{ return gt < Line; }
    mGlobal(Seis) inline bool		isPS( GeomType gt )
    			{ return gt == VolPS || gt == LinePS; }
    mGlobal(Seis) inline int		dimSize( GeomType gt )
			{ return gt == Line ? 2 : (gt == VolPS ? 4 : 3); }
    mGlobal(Seis) const char*		nameOf(GeomType);
    mGlobal(Seis) GeomType		geomTypeOf(const char*);
    mGlobal(Seis) inline GeomType	geomTypeOf( bool is2d, bool isps )
			{ return is2d ? (isps?LinePS:Line) : (isps?VolPS:Vol); }
    mGlobal(Seis) const char**	geomTypeNames();
    mGlobal(Seis) void		putInPar(GeomType,IOPar&);
    mGlobal(Seis) bool		getFromPar(const IOPar&,GeomType&);


    enum DataType       { Ampl, Dip, Frequency, Phase, AVOGradient, 
			  Azimuth, Classification, UnknowData };
    mGlobal(Seis) bool		isAngle(DataType);
    mGlobal(Seis) const char*		nameOf(DataType);
    mGlobal(Seis) DataType		dataTypeOf(const char*);
    mGlobal(Seis) const char**	dataTypeNames();

    enum WaveType       { P, Sh, Sv, UnknowWave };
    mGlobal(Seis) const char*		nameOf(WaveType);
    mGlobal(Seis) WaveType		waveTypeOf(const char*);
    mGlobal(Seis) const char**	waveTypeNames();
};


#endif

