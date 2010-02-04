#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.40 2010-02-04 17:20:24 cvsjaap Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "faultsticksurface.h"

namespace EM
{
class Fault;
class FaultStickSetGeometry;

/*!\brief FaultGeometry base class */

mClass FaultGeometry : public SurfaceGeometry
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
    virtual const MultiID*	lineSet(const SectionID&,int sticknr) const
							{ return 0; }
    virtual const char*		lineName(const SectionID&,int sticknr) const
							{ return 0; }

    virtual void	copySelectedSticksTo(FaultStickSetGeometry& tofssg,
					     const SectionID& tosid) const;
    virtual void	selectAllSticks(bool select=true);
    virtual void	removeSelectedSticks();
    virtual int		nrSelectedSticks() const;

protected:
    bool		removeNextSelStick();

    			FaultGeometry( Surface& surf )
			    : SurfaceGeometry(surf)	{}
};



/*!\brief Fault base class */

mClass Fault : public Surface
{
public:
    virtual FaultGeometry&	geometry()			= 0;
    virtual const FaultGeometry& geometry() const
				{ return const_cast<Fault*>(this)->geometry(); }

protected:
    				Fault( EMManager& em )
				    : Surface(em)		{}

    const IOObjContext&		getIOObjContext() const		= 0;
};


} // namespace EM


#endif
