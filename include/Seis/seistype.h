#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		29-5-99
________________________________________________________________________

*/

#include "seismod.h"
#include "gendefs.h"
#include "uistring.h"
class IOObjContext;
class BufferStringSet;


/*!\brief Seismics */

namespace Seis
{

    enum SelType		{ Range, Table, Polygon };
    mGlobal(Seis) const char*	nameOf(SelType);
    mGlobal(Seis) SelType	selTypeOf(const char*);

    enum ReadMode		{ PreScan, Scan, Prod };

    enum GeomType		{ Line, LinePS, Vol, VolPS };
    inline bool			is2D( GeomType gt )
				{ return gt < Vol; }
    inline bool			is3D( GeomType gt )
				{ return gt > LinePS; }
    inline bool			isPS( GeomType gt )
				{ return gt == VolPS || gt == LinePS; }
    inline int			dimSize( GeomType gt )
				{ return gt == Line ? 2 : (gt==VolPS ? 4 : 3); }
    mGlobal(Seis) const char*	nameOf(GeomType);
    mGlobal(Seis) const char*	iconIDOf(GeomType);
    mGlobal(Seis) GeomType	geomTypeOf(const char*);
    inline GeomType		geomTypeOf( bool is2d, bool isps )
				{ return is2d ? (isps ? LinePS : Line)
					      : (isps ? VolPS : Vol); }
    mGlobal(Seis) uiString	dataName(GeomType,bool both_pre_post=false);
    mGlobal(Seis) void		putInPar(GeomType,IOPar&);
    mGlobal(Seis) bool		getFromPar(const IOPar&,GeomType&);
    mGlobal(Seis) bool		is2DGeom(const IOPar&);
    mGlobal(Seis) bool		isPSGeom(const IOPar&);
    mGlobal(Seis) IOObjContext*	getIOObjContext(Seis::GeomType,bool forread);

    enum DataType		{ Ampl, Dip, Frequency, Phase, AVOGradient,
				  Azimuth, Classification, IncidenceAngle,
				  UnknownData };
    mGlobal(Seis) bool		isAngle(DataType);
    mGlobal(Seis) const char*	nameOf(DataType);
    mGlobal(Seis) DataType	dataTypeOf(const char*);
    mGlobal(Seis) const BufferStringSet& dataTypeNames();

    enum WaveType		{ P, Sh, Sv, UnknowWave };
    mGlobal(Seis) const char*	nameOf(WaveType);
    mGlobal(Seis) WaveType	waveTypeOf(const char*);

} // namespace Seis
