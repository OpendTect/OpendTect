#ifndef faulteditor_h
#define faulteditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: faulteditor.h,v 1.11 2010-08-05 14:19:59 cvsjaap Exp $
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
    void			setSowingPivot(const Coord3);

    void			getInteractionInfo(bool& makenewstick,
					EM::PosID& insertpid,
					const Coord3& mousepos,float zfactor,
					const Coord3* posnormal=0) const;

    bool			removeSelection(const Selector<Coord3>&);

protected:
    float		getNearestStick(int& stick,EM::SectionID& sid,
			    const Coord3& mousepos,float zfactor,
			    const Coord3* posnormal) const;
    bool		getInsertStick(int& stick,EM::SectionID& sid,
			    const Coord3& mousepos,float zfactor,
			    const Coord3* posnormal) const;
    void		getPidsOnStick( EM::PosID& insertpid,int stick,
			    const EM::SectionID&,const Coord3&,
			    float zfactor) const;

    Geometry::ElementEditor*	createEditor(const EM::SectionID&);
    int				getLastClickedStick() const;

    Coord3			sowingpivot_;
    TypeSet<Coord3>		sowinghistory_;
};


}  // namespace MPE

#endif
