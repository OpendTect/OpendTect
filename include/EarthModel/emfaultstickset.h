#ifndef emfaultstickset_h
#define emfaultstickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	J.C Glas
 Date:		November 2008
 RCS:		$Id: emfaultstickset.h,v 1.1 2008-11-18 13:28:53 cvsjaap Exp $
________________________________________________________________________


-*/

#include "emfault.h"

namespace Geometry { class FaultStickSet; }

namespace EM
{
class EMManager;

class FaultStickSetGeometry : public FaultGeometry
{
public:
    			FaultStickSetGeometry(Surface&);
			~FaultStickSetGeometry();

    int 		nrSticks(const SectionID&) const;
    int			nrKnots(const SectionID&,int sticknr) const;

    bool		insertStick(const SectionID&,int sticknr,int firstcol,
	    			    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory);
    bool		insertStick(const SectionID&,int sticknr,int firstcol,
	    			    const Coord3& pos,const Coord3& editnormal,
				    const MultiID* lineset,const char* linenm,
				    bool addtohistory);
    bool		removeStick(const SectionID&,int sticknr,
	    			    bool addtohistory);
    bool		insertKnot(const SectionID&,const SubID&,
	    			   const Coord3& pos,bool addtohistory);
    bool		removeKnot(const SectionID&,const SubID&,
	    			   bool addtohistory);
    
    bool		pickedOnPlane(const SectionID&,int sticknr) const;
    bool		pickedOn2DLine(const SectionID&,int sticknr) const;

    const MultiID*	lineSet(const SectionID&,int sticknr) const;
    const char*		lineName(const SectionID&,int sticknr) const;

    const Coord3&	getEditPlaneNormal(const SectionID&,int sticknr) const;

    Geometry::FaultStickSet*
			sectionGeometry(const SectionID&);
    const Geometry::FaultStickSet*
			sectionGeometry(const SectionID&) const;

    EMObjectIterator*	createIterator(const SectionID&,
				       const CubeSampling* =0) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    Geometry::FaultStickSet*	createSectionGeometry() const;

    struct StickInfo
    {
	int		sid;
	int		sticknr;
	MultiID		lineset;
	BufferString	linenm;
    };

    ObjectSet<StickInfo>	 stickinfo_;

};



/*!\brief Fault stick set
*/

class FaultStickSet: public Fault
{ mDefineEMObjFuncs( FaultStickSet );
public:
    FaultStickSetGeometry&		geometry();
    const FaultStickSetGeometry&	geometry() const;

protected:
    const IOObjContext&			getIOObjContext() const;

    FaultStickSetGeometry		geometry_;
};


} // namespace EM


#endif
