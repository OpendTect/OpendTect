#ifndef faultsticksetpreditor_h
#define faultstickseteditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          October 2008
 RCS:           $Id: faultstickseteditor.h,v 1.4 2009-10-27 13:43:51 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emeditor.h"

namespace EM { class FaultStickSet; };
template <class T> class Selector;

namespace MPE
{

mClass FaultStickSetEditor : public ObjectEditor
{
public:
    				FaultStickSetEditor(EM::FaultStickSet&);
    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    void			setEditIDs(const TypeSet<EM::PosID>* editpids);
    void			getEditIDs(TypeSet<EM::PosID>&) const;

    void			setLastClicked(const EM::PosID&); 

    void			getInteractionInfo( EM::PosID& insertpid,
				    const MultiID* lineset,const char* linenm,
				    const Coord3&,float zfactor) const;

    bool			removeSelection(const Selector<Coord3>&);

protected:
    bool		getNearestStick(int& sticknr,EM::SectionID& sid,
				    const MultiID* lineset,const char* linenm,
				    const Coord3&,float zfactor) const;
    void		getPidsOnStick(EM::PosID& insertpid,int sticknr,
				    const EM::SectionID&,const Coord3&,
				    float zfactor) const;

    Geometry::ElementEditor*	createEditor(const EM::SectionID&);

    const TypeSet<EM::PosID>*	editpids_;
};


}  // namespace MPE

#endif
