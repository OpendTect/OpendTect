#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emundo.h"
#include "faultsticksurface.h"


namespace EM
{
class Fault;
class FaultStickSetGeometry;

/*!
\brief FaultGeometry base class.
*/

mExpClass(EarthModel) FaultGeometry : public SurfaceGeometry
{
public:
    virtual bool	insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory)	{ return false; }

    virtual bool	insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    const DBKey* pickeddbkey,
				    const char* pickednm,bool addtohistory)
							{ return false; }
    virtual bool	insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    Pos::GeomID pickedgeomid,bool addtohistory)
							{ return false; }
    virtual bool        insertKnot(const PosID&,
				   const Coord3& pos,bool addtohistory)
							{ return false; }
    virtual bool	removeStick(int sticknr,
				    bool addtohistory)	{ return false; }
    virtual bool	removeKnot(const PosID&,
				   bool addtohistory)	{ return false; }

    virtual Coord3	getEditPlaneNormal(int sticknr) const;
    virtual const DBKey*	pickedDBKey(int stcknr) const
							{ return 0; }
    virtual const char*		pickedName(int sticknr) const
							{ return 0; }

    virtual void	copySelectedSticksTo(FaultStickSetGeometry& destfssg,
					     bool addtohistory) const;

    virtual int		nrSelectedSticks() const;
    virtual void	selectAllSticks(bool select=true);
    virtual void	removeSelectedSticks(bool addtohistory);

    virtual void	selectStickDoubles(bool select=true,
					   const FaultGeometry* ref=0);
    virtual void	removeSelectedDoubles(bool addtohistory,
					      const FaultGeometry* ref=0);
    virtual int		nrStickDoubles(int sticknr,
				       const FaultGeometry* ref=0) const;

protected:
    void		selectSticks(bool select=true,
				     const FaultGeometry* doublesref=0);
    bool		removeSelStick(int selidx,bool addtohistory,
				       const FaultGeometry* doublesref=0);

			FaultGeometry( Surface& surf )
			    : SurfaceGeometry(surf)	{}
};



/*!
\brief Fault Surface base class.
*/

mExpClass(EarthModel) Fault : public Surface
{
public:

    bool			insertStick(int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 Pos::GeomID pickedgeomid,
					 bool addtohistory );
    bool			insertStick(int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 const DBKey* pickeddbkey,
					 const char* pickednm,
					 bool addtohistory );
    bool			insertStick(int sticknr,int firstcol,
					 const Coord3& pos,
					 const Coord3& editnormal,
					 bool addtohistory);
    bool			removeStick(int sticknr,bool);

    bool			insertKnot(const PosID&,const Coord3&,bool);
    bool			removeKnot(const PosID&, bool);

    StepInterval<int>		rowRange() const;
    StepInterval<int>		colRange(int row) const;
    int				nrSticks() const;
    TypeSet<Coord3>		getStick(int sticknr) const;
    unsigned int		totalSize() const;

    int				nrKnots(int sticknr) const
				{ return getStick(sticknr).size(); }
    Coord3			getKnot(RowCol rc) const;
    void			hideKnot(RowCol rc,bool,int scnidx);
    bool			isKnotHidden(RowCol rc,int scnidx=-1) const;

    void			hideStick(int sticknr,bool,int scnidx=-1);
    bool			isStickHidden(int sticknr, int sceneidx ) const;
    bool			isStickSelected(int sticknr);
    void			selectStick(int sticknr,bool);
    void			removeSelectedSticks(bool);

    void			hideAllSticks(bool yn,int sceneidx);
    void			hideAllKnots(bool yn,int sceneidx);

    void			hideSticks(const TypeSet<int>& sticknrs,bool yn,
						int sceneidx);
    void			hideKnots(const TypeSet<RowCol>& rcs,bool yn,
						int sceneidx);

    Coord3			getEditPlaneNormal(int sticknr) const;
    ObjectIterator*		createIterator(const TrcKeyZSampling*) const;

    void			preferStick(int sticknr);
    int				preferredStickNr() const;

    virtual bool		pickedOnPlane(int row) const  { return false; }
    virtual bool		pickedOn2DLine(int row) const { return false; }
    virtual Pos::GeomID		pickedGeomID(int row) const
				{ return Pos::GeomID::get3D(); }
    virtual const DBKey*	pickedDBKey(int sticknr) const{ return 0; }
    virtual const char*		pickedName(int sticknr) const { return 0; }


    virtual void		removeAll();
    virtual FaultGeometry&	geometry()			= 0;
    virtual const FaultGeometry& geometry() const
				{ return const_cast<Fault*>(this)->geometry(); }

protected:
				Fault(const char* nm)
				    : Surface(nm)		{}

    const IOObjContext&		getIOObjContext() const		= 0;
    void			hidStick(int sticknr,bool,int scnidx=-1);
    void			hidKnot(RowCol rc,bool,int scnidx=-1);

};


/*!\brief Fault stick UndoEvent. */

mExpClass(EarthModel) FaultStickUndoEvent : public ::UndoEvent
{
public:
			//Interface for insert
			FaultStickUndoEvent(const EM::PosID&);
			//Interface for removal
			FaultStickUndoEvent(const EM::PosID&,
					    const Coord3& oldpos,
					    const Coord3& oldnormal );
    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    Coord3		pos_;
    Coord3		normal_;
    EM::PosID		posid_;
    bool		remove_;
};


/*!\brief Fault knot UndoEvent. */

mExpClass(EarthModel) FaultKnotUndoEvent : public ::UndoEvent
{
public:
			//Interface for insert
			FaultKnotUndoEvent(const EM::PosID&);
			//Interface for removal
			FaultKnotUndoEvent(const EM::PosID&,
					   const Coord3& oldpos);
    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    Coord3		pos_;
    Coord3		normal_;
    EM::PosID		posid_;
    bool		remove_;
};


} // namespace EM
