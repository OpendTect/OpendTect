#ifndef emfault3d_h
#define emfault3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault3d.h,v 1.1 2008-09-30 12:47:40 cvsnanne Exp $
________________________________________________________________________


-*/

#include "emfault.h"
#include "tableascio.h"


namespace Table { class FormatDesc; }

namespace Geometry { class FaultStickSurface; }

namespace EM
{
class EMManager;

class Fault3DGeometry : public FaultGeometry
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
    const Coord3&	getEditPlaneNormal(const SectionID&,int sticknr) const;

    Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&);
    const Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&) const;

    EMObjectIterator*	createIterator(const SectionID&,
	    			       const CubeSampling* =0) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    Geometry::FaultStickSurface*	createSectionGeometry() const;
};



/*!\brief 3D Fault
*/

class Fault3D : public Surface
{ mDefineEMObjFuncs( Fault3D );
public:
    Fault3DGeometry&		geometry();
    const Fault3DGeometry&	geometry() const;

protected:
    const IOObjContext&		getIOObjContext() const;


    friend class		EMManager;
    friend class		EMObject;
    Fault3DGeometry		geometry_;
};


class Fault3DAscIO : public Table::AscIO
{
public:
    				Fault3DAscIO( const Table::FormatDesc& fd )
				    : Table::AscIO(fd)		{}
    static Table::FormatDesc*	getDesc();

    bool			get(std::istream&,EM::Fault3D&) const;

protected:
    bool			isXY() const;
};


} // namespace EM


#endif
