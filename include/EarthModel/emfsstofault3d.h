#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emposid.h"
#include "coord.h"
#include "sets.h"


namespace Geometry { class FaultStickSet; }

namespace EM
{

class FaultStickSet;
class Fault3D;

/*!
\brief FaultStickSet to Fault3D converter.
*/

mExpClass(EarthModel) FSStoFault3DConverter
{
public:

    mExpClass(EarthModel) Setup
    {
    public:
				Setup();

	enum			DirSpec { Auto, Horizontal, Vertical };

	mDefSetupMemb(DirSpec,pickplanedir)		// Default Auto
	mDefSetupMemb(bool,sortsticks)			// Default true
	mDefSetupMemb(double,zscale)			// Default SI().zScale()

	mDefSetupMemb(bool,useinlcrlslopesep)		// Default false
	mDefSetupMemb(double,stickslopethres)		// Default mUdf(double)
	mDefSetupMemb(DirSpec,stickseldir)		// Default Auto

	mDefSetupMemb(bool,addtohistory)		// Default false
    };

				FSStoFault3DConverter(const Setup&,
						      const FaultStickSet&,
						      Fault3D&);
    bool			convert(bool forimport);

protected:

    mExpClass(EarthModel) FaultStick
    {
    public:
				FaultStick(int sticknr);

	int			sticknr_;
	TypeSet<Coord3>		crds_;
	Coord3			normal_;
	bool			pickedonplane_;

	Coord3			findPlaneNormal() const;
	double			slope(double zscale) const;
	bool			pickedOnInl() const;
	bool			pickedOnCrl() const;
	bool			pickedOnTimeSlice() const;
	bool			pickedOnHorizon() const;
    };

    const FaultStickSet&	fss_;
    Fault3D&			fault3d_;
    Setup			setup_;
    ObjectSet<FaultStick>	sticks_;

    const Geometry::FaultStickSet* curfssg_;

    bool			readSection();
    bool			readSectionForImport();
    bool			preferHorPicked() const;
    void			selectSticks( bool selhorpicked );
    void			geometricSort(double zscale,bool forimport);
    void			untwistSticks(double zscale);
    void			resolveUdfNormals();
    bool			writeSection() const;
};

} // namespace EM
