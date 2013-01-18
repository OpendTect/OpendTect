#ifndef faultstickseteditor_h
#define faultstickseteditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          October 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emeditor.h"

namespace EM { class FaultStickSet; };
template <class T> class Selector;

namespace MPE
{

/*!
\brief ObjectEditor to edit EM::FaultStickSet.
*/

mClass(MPEEngine) FaultStickSetEditor : public ObjectEditor
{
public:
    				FaultStickSetEditor(EM::FaultStickSet&);

    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    void			setEditIDs(const TypeSet<EM::PosID>* editpids);
    void			getEditIDs(TypeSet<EM::PosID>&) const;

    void			setLastClicked(const EM::PosID&); 
    void			setSowingPivot(const Coord3);

    void			setZScale(float);
    void			setScaleVector(const Coord3& v);
    				//!< x'=x, y'=v1*x*+v2*y, z'=v3*z

    void			getInteractionInfo( EM::PosID& insertpid,
				    const MultiID* pickedmid,
				    const char* pickednm,const Coord3& pos,
				    const Coord3* posnorm=0) const;

    bool			removeSelection(const Selector<Coord3>&);

protected:
    float		distToStick(int sticknr,const EM::SectionID& sid,
				const MultiID* pickedmid,const char* pickednm,
				const Coord3& pos,const Coord3* posnorm) const;
    bool		getNearestStick(int& sticknr,EM::SectionID& sid,
				const MultiID* pickedmid,const char* pickednm,
				const Coord3& pos,const Coord3* posnorm) const;
    void		getPidsOnStick(EM::PosID& insertpid,int sticknr,
				const EM::SectionID&,const Coord3& pos) const;

    Geometry::ElementEditor*	createEditor(const EM::SectionID&);
    Coord3			scalevector_;
    Coord			xtrans_;
    Coord			ytrans_;

    int				getLastClickedStick() const;

    void			cloneMovingNode();

    const TypeSet<EM::PosID>*	editpids_;

    Coord3			sowingpivot_;
    TypeSet<Coord3>		sowinghistory_;

};


}  // namespace MPE

#endif

