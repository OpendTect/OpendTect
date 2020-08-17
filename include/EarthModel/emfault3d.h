#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emfault.h"
#include "tableascio.h"
#include "emfaultstickset.h"

namespace Table { class FormatDesc; }

namespace Geometry { class FaultStickSurface; }
namespace Geometry { class FaultStickSet; }
namespace Pos { class Filter; }

namespace EM
{
class EMManager;
class FaultAuxData;

/*!
\brief 3D FaultGeometry.
*/

mExpClass(EarthModel) Fault3DGeometry : public FaultGeometry
{
public:
    			Fault3DGeometry(Surface&);
			~Fault3DGeometry();

    int			nrSticks(const SectionID&) const;
    int			nrKnots(const SectionID&,int sticknr) const;

    bool		insertStick(const SectionID&,int sticknr,int firstcol,
	    			    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory);
    bool		removeStick(const SectionID&,int sticknr,
				    bool addtohistory);
    bool		insertKnot(const SectionID&,const SubID&,
	    			   const Coord3& pos,bool addtohistory);
    bool		removeKnot(const SectionID&,const SubID&,
	    			   bool addtohistory);
    
    bool		areSticksVertical(const SectionID&) const;
    bool		areEditPlanesMostlyCrossline() const;

    Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&);
    const Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&) const;

    EMObjectIterator*	createIterator(const SectionID&,
				       const TrcKeyZSampling* =0) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    Geometry::FaultStickSurface*	createSectionGeometry() const;
};


/*!
\brief 3D Fault
*/

mExpClass(EarthModel) Fault3D : public Fault
{ mDefineEMObjFuncs( Fault3D );
public:
    Fault3DGeometry&		geometry();
    const Fault3DGeometry&	geometry() const;
    void			apply(const Pos::Filter&);
    uiString			getUserTypeStr() const;

    FaultAuxData*		auxData();
    const FaultAuxData*		auxData() const;

protected:

    const IOObjContext&		getIOObjContext() const;

    friend class		EMManager;
    friend class		EMObject;
    Fault3DGeometry		geometry_;
    FaultAuxData*		auxdata_;
};


/*!
\brief Ascii I/O for Fault.
*/

mExpClass(EarthModel) FaultAscIO : public Table::AscIO
{
public:
    				FaultAscIO( const Table::FormatDesc& fd )
				    : Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc(bool is2d);

    bool			get(od_istream&,EM::Fault&,
				    bool sortsticks=false, 
				    bool is2d=false) const;
protected:
    bool			isXY() const;
};


/*!
\brief Class to hold Fault-stick coordinates and compute the normal.
*/

mExpClass(EarthModel)	FaultStick
{
public:
    			FaultStick(int idx)	: stickidx_(idx)	{}

    Coord3		getNormal(bool is2d) const;

    int			stickidx_;
    TypeSet<Coord3>	crds_;
    BufferString	lnm_;
};

} // namespace EM


