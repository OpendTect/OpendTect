#ifndef faulteditor_h
#define faulteditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: faulteditor.h,v 1.6 2009-01-06 10:48:18 cvsranojay Exp $
________________________________________________________________________

-*/

#include "emeditor.h"

namespace EM { class Fault3D; };
template <class T> class Selector;

namespace MPE
{

mClass FaultEditor : public ObjectEditor
{
public:
    				FaultEditor(EM::Fault3D&);
    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    void			getInteractionInfo(EM::PosID& nearestpid0,
					   EM::PosID& nearestpid1,
					   EM::PosID& insertpid,
					   const Coord3&,
					   float zfactor) const;
    bool			removeSelection(const Selector<Coord3>&);

protected:
    float		getNearestStick(int& stick,EM::SectionID& sid,
	    				const Coord3&,float zfactor) const;
    void		getPidsOnStick( EM::PosID& nearestpid0,
			    EM::PosID& nearestpid1,EM::PosID& insertpid,
			    int stick,const EM::SectionID&,const Coord3&,
			    float zfactor) const;

    Geometry::ElementEditor*	createEditor(const EM::SectionID&);
};


}  // namespace MPE

#endif
