#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "faultsticksurface.h"
#include "undo.h"


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
    virtual		~FaultGeometry();

    virtual bool	insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory)
			{ return false; }
    virtual bool	insertKnot(const SubID&,const Coord3& pos,
				   bool addtohistory)
			{ return false; }
    virtual bool	removeStick(int sticknr,bool addtohistory)
			{ return false; }
    virtual bool	removeKnot(const SubID&,bool addtohistory)
			{ return false; }

    virtual const Coord3&	getEditPlaneNormal(int sticknr) const;
    virtual const MultiID*	pickedMultiID(int stcknr) const
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

// Deprecated public functions
    mDeprecated("Use without SectionID")
    virtual bool	insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory)
			{ return insertStick(sticknr,firstcol,pos,
					     editnormal,addtohistory); }
    mDeprecated("Use without SectionID")
    virtual bool	insertKnot(const SectionID&,const SubID& subid,
				   const Coord3& pos,bool addtohistory)
			{ return insertKnot(subid,pos,addtohistory); }
    mDeprecated("Use without SectionID")
    virtual bool	removeStick(const SectionID&,int sticknr,
				    bool addtohistory)
			{ return removeSelStick(sticknr,addtohistory); }
    mDeprecated("Use without SectionID")
    virtual bool	removeKnot(const SectionID&,const SubID& subid,
				   bool addtohistory)
			{ return removeKnot(subid,addtohistory); }

    mDeprecated("Use without SectionID")
    virtual const Coord3&	getEditPlaneNormal(const SectionID&,
						   int sticknr) const
				{ return getEditPlaneNormal(sticknr); }
    mDeprecated("Use without SectionID")
    virtual const MultiID*	pickedMultiID(const SectionID&,int stcknr) const
				{ return pickedMultiID(stcknr); }
    mDeprecated("Use without SectionID")
    virtual const char*		pickedName(const SectionID&,int sticknr) const
				{ return pickedName(sticknr); }

    mDeprecated("Use without SectionID")
    virtual void	copySelectedSticksTo(FaultStickSetGeometry& dest,
					     const SectionID& sid,
					     bool addtohistory) const
			{ copySelectedSticksTo(dest,addtohistory); }
    mDeprecated("Use without SectionID")
    virtual int		nrStickDoubles(const SectionID&,int sticknr,
				       const FaultGeometry* ref=0) const
			{ return nrStickDoubles(sticknr,ref); }

protected:
			FaultGeometry(Surface&);

    void		selectSticks(bool select=true,
				     const FaultGeometry* doublesref=0);
    bool		removeSelStick(int selidx,bool addtohistory,
				       const FaultGeometry* doublesref=0);
};



/*!
\brief Fault Surface base class.
*/

mExpClass(EarthModel) Fault : public Surface
{
public:
    virtual			~Fault();

    void			removeAll() override;
    FaultGeometry&		geometry() override			= 0;
    const FaultGeometry&	geometry() const override
				{ return const_cast<Fault*>(this)->geometry(); }

protected:
				Fault(EMManager&);

    const IOObjContext&		getIOObjContext() const override	= 0;
};


/*!
\brief Fault stick UndoEvent.
*/

mExpClass(EarthModel) FaultStickUndoEvent : public UndoEvent
{
public:
			//Interface for insert
			FaultStickUndoEvent(const EM::PosID&);
			//Interface for removal
			FaultStickUndoEvent(const EM::PosID&,
					    const Coord3& oldpos,
					    const Coord3& oldnormal );
    const char*		getStandardDesc() const override;
    bool		unDo() override;
    bool		reDo() override;

protected:
    Coord3		pos_;
    Coord3		normal_;
    EM::PosID		posid_;
    bool		remove_;
};


/*!
\brief Fault knot UndoEvent.
*/

mExpClass(EarthModel) FaultKnotUndoEvent : public UndoEvent
{
public:
			//Interface for insert
			FaultKnotUndoEvent(const EM::PosID&);
			//Interface for removal
			FaultKnotUndoEvent(const EM::PosID&,
					   const Coord3& oldpos);
    const char*		getStandardDesc() const override;
    bool		unDo() override;
    bool		reDo() override;

protected:
    Coord3		pos_;
    Coord3		normal_;
    EM::PosID		posid_;
    bool		remove_;
};

} // namespace EM
