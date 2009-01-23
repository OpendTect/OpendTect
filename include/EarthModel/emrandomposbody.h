#ifndef emrandomposbody_h
#define emrandomposbody_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		January 2009
 RCS:		$Id: emrandomposbody.h,v 1.1 2009-01-23 21:46:46 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "embody.h"
#include "emobject.h"

namespace Pick { class Set; }

namespace EM
{

mClass RandomPosBody : public Body, public EMObject
{ mDefineEMObjFuncs( RandomPosBody );
public:
    
    virtual int			nrSections() const		{ return 1; }
    virtual SectionID		sectionID(int) const		{ return 0; }
    virtual bool		canSetSectionName() const 	{ return 0; }

    Geometry::Element*		sectionGeometry(const SectionID&) { return 0; }
    const Geometry::Element*	sectionGeometry(const SectionID&) const 
    				{ return 0; }

    void			copyFrom(const Pick::Set&);//get my own picks.
    void			setPositions(const TypeSet<Coord3>&);
    const TypeSet<Coord3>&	getPositions() const	{ return locations_; }
    bool			addPos(const Coord3&);

    const TypeSet<EM::SubID>&	posIDs() const 		{ return ids_; }

    Coord3			getPos(const EM::PosID&) const;
    Coord3			getPos(const EM::SectionID&,
	    				const EM::SubID&) const;
    bool			setPos(const EM::PosID&,const Coord3&,
	    				bool addtohistory);
    bool			setPos(const EM::SectionID&,const EM::SubID&,
	    				const Coord3&,bool addtohistory);
    const IOObjContext&		getIOObjContext() const;
    virtual Executor*		saver();
    virtual Executor*		saver(IOObj*);
    virtual Executor*		loader();

    ImplicitBody*               createImplicitBody(TaskRunner*) const 
    				{ return 0; } 
protected:

    TypeSet<Coord3>		locations_;
    TypeSet<EM::SubID>		ids_;
};


}; // Namespace


#endif
