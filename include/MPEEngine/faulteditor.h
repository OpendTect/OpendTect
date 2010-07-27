#ifndef faulteditor_h
#define faulteditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: faulteditor.h,v 1.10 2010-07-27 08:59:04 cvsjaap Exp $
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

    void			setLastClicked(const EM::PosID&);
    const EM::PosID&		getLastClicked() const;
    void			setSowingPivot(const Coord3);

    void			getInteractionInfo(bool& makenewstick,
					EM::PosID& insertpid,
					const Coord3&,float zfactor) const;

    bool			removeSelection(const Selector<Coord3>&);

protected:
    float		getNearestStick(int& stick,EM::SectionID& sid,
	    				const Coord3&,float zfactor) const;
    void		getPidsOnStick( EM::PosID& nearestpid0,
			    EM::PosID& nearestpid1,EM::PosID& insertpid,
			    int stick,const EM::SectionID&,const Coord3&,
			    float zfactor) const;

    Geometry::ElementEditor*	createEditor(const EM::SectionID&);

    Coord3			sowingpivot_;
    TypeSet<Coord3>		sowinghistory_;
};


}  // namespace MPE

#endif
