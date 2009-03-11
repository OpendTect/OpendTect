#ifndef emfault3d_h
#define emfault3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault3d.h,v 1.4 2009-03-11 08:18:46 cvsjaap Exp $
________________________________________________________________________


-*/

#include "emfault.h"
#include "tableascio.h"
#include "emfaultstickset.h"

namespace Table { class FormatDesc; }

namespace Geometry { class FaultStickSurface; }
namespace Geometry { class FaultStickSet; }

namespace EM
{
class EMManager;

mClass Fault3DGeometry : public FaultGeometry
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

mClass Fault3D : public Fault
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


mClass FaultAscIO : public Table::AscIO
{
public:
    				FaultAscIO( const Table::FormatDesc& fd )
				    : Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc(bool is2d);

    bool			get(std::istream&,EM::Fault&,
				    bool sortsticks=false, 
	    			    const MultiID* linesetmid=0,
				    bool is2d=false) const;
protected:
    bool			isXY() const;
};


} // namespace EM


#endif
