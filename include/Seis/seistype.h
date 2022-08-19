#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "gendefs.h"
#include "ptrman.h"
#include "uistring.h"

class CommandLineParser;
class IOObj;
class IOObjContext;
class IOPar;
namespace Coords { class CoordSystem; }
namespace OS { class MachineCommand; }
namespace Seis	{ class SelData; }

/*!\brief Seismics */

namespace Seis
{

    enum SelType	{ Range, Table, Polygon };
    mGlobal(Seis) const char*	nameOf(SelType);
    mGlobal(Seis) SelType	selTypeOf(const char*);
    mGlobal(Seis) const char**	selTypeNames();

    enum ReadMode	{ PreScan, Scan, Prod };

    enum GeomType	{ Vol, VolPS, Line, LinePS };
    mGlobal(Seis) inline bool	is2D( GeomType gt )
			{ return gt > VolPS; }
    mGlobal(Seis) inline bool	is3D( GeomType gt )
			{ return gt < Line; }
    mGlobal(Seis) inline bool	isPS( GeomType gt )
			{ return gt == VolPS || gt == LinePS; }
    mGlobal(Seis) inline int	dimSize( GeomType gt )
			{ return gt == Line ? 2 : (gt == VolPS ? 4 : 3); }
    mGlobal(Seis) const char*	nameOf(GeomType);
    mGlobal(Seis) GeomType	geomTypeOf(const char*);
    mGlobal(Seis) inline GeomType geomTypeOf( bool is2d, bool isps )
			{ return is2d ? (isps?LinePS:Line) : (isps?VolPS:Vol); }
    mGlobal(Seis) uiString dataName(GeomType,bool both_pre_post=false);
    mGlobal(Seis) const char**	geomTypeNames();
    mGlobal(Seis) void		putInPar(GeomType,IOPar&);
    mGlobal(Seis) void		putInMC(GeomType,OS::MachineCommand&);
    mGlobal(Seis) bool		getFromPar(const IOPar&,GeomType&);
    mGlobal(Seis) bool		getFromCLP(const CommandLineParser&,GeomType&);
    mGlobal(Seis) bool		is2DGeom(const IOPar&);
    mGlobal(Seis) bool		isPSGeom(const IOPar&);
    mGlobal(Seis) IOObjContext*	getIOObjContext(Seis::GeomType,bool forread);

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


} // namespace Seis
