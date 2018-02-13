#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
________________________________________________________________________


-*/

#include "emfault.h"
#include "emfaultstickset.h"
#include "tableascio.h"

namespace Table { class FormatDesc; }

namespace Geometry { class FaultStickSurface; }
namespace Geometry { class FaultStickSet; }
namespace Pos { class Filter; }

namespace EM
{
class ObjectManager;
class FaultAuxData;

/*!
\brief 3D FaultGeometry.
*/

mExpClass(EarthModel) Fault3DGeometry : public FaultGeometry
{
public:
			Fault3DGeometry(Surface&);
			~Fault3DGeometry();

    int			nrSticks() const;
    int			nrKnots(int sticknr) const;

    virtual bool	insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory);
    virtual bool	insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    const DBKey* pickeddbkey,
				    const char* pickednm,bool addtohistory)
							{ return false; }
    virtual bool	insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    Pos::GeomID pickedgeomid,bool addtohistory)
							{ return false; }

    bool		removeStick(int sticknr,
				    bool addtohistory);
    bool		insertKnot(const PosID&,
				   const Coord3& pos,bool addtohistory);
    bool		removeKnot(const PosID&,
				   bool addtohistory);

    bool		areSticksVertical() const;
    bool		areEditPlanesMostlyCrossline() const;

    Geometry::FaultStickSurface*	geometryElement();
    const Geometry::FaultStickSurface*	geometryElement() const;

    ObjectIterator*	createIterator(const TrcKeyZSampling* =0) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    Geometry::FaultStickSurface*	createGeometryElement() const;
};


/*!
\brief 3D Fault
*/

mExpClass(EarthModel) Fault3D : public Fault
{   mDefineEMObjFuncs( Fault3D );
    mODTextTranslationClass( Fault3D );
public:

				mDeclMonitorableAssignment(Fault3D);
				mDeclInstanceCreatedNotifierAccess(Fault3D);
    Fault3DGeometry&		geometry();
    const Fault3DGeometry&	geometry() const;
    void			apply(const Pos::Filter&);
    uiString			getUserTypeStr() const;

    FaultAuxData*		auxData();
    const FaultAuxData*		auxData() const;

protected:

    const IOObjContext&		getIOObjContext() const;

    friend class		Object;
    friend class		ObjectManager;
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
