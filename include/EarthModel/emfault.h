#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id$
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
    virtual bool	insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory)	{ return false; }
    virtual bool        insertKnot(const SectionID&,const SubID&,
	    			   const Coord3& pos,bool addtohistory)
							{ return false; }
    virtual bool	removeStick(const SectionID&,int sticknr,
				    bool addtohistory)	{ return false; }
    virtual bool	removeKnot(const SectionID&,const SubID&,
				   bool addtohistory)	{ return false; }

    virtual const Coord3&	getEditPlaneNormal(const SectionID&,
						   int sticknr) const;
    virtual const MultiID*	pickedMultiID(const SectionID&,int stcknr) const
							{ return 0; }
    virtual const char*		pickedName(const SectionID&,int sticknr) const
							{ return 0; }

    virtual void	copySelectedSticksTo(FaultStickSetGeometry& destfssg,
					     const SectionID& destsid,
					     bool addtohistory) const;

    virtual int		nrSelectedSticks() const;
    virtual void	selectAllSticks(bool select=true);
    virtual void	removeSelectedSticks(bool addtohistory);

    virtual void	selectStickDoubles(bool select=true,
					   const FaultGeometry* ref=0);
    virtual void	removeSelectedDoubles(bool addtohistory,
					      const FaultGeometry* ref=0);
    virtual int		nrStickDoubles(const SectionID&,int sticknr,
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
    virtual void		removeAll();
    virtual FaultGeometry&	geometry()			= 0;
    virtual const FaultGeometry& geometry() const
				{ return const_cast<Fault*>(this)->geometry(); }

    enum FaultAct		{ AllowCrossing=0, ForbidCrossHigher=1, 
				  ForbidCrossLower=2 };
    static void                 getAllFaultActNames(BufferStringSet&);

    static const char*		sKeyFaultAct()	{ return "Fault Act"; }

protected:
    				Fault( EMManager& em )
				    : Surface(em)		{}

    const IOObjContext&		getIOObjContext() const		= 0;
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
    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

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


#endif

