#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emeditor.h"

namespace EM { class FaultStickSet; }
template <class T> class Selector;

namespace MPE
{

/*!
\brief ObjectEditor to edit EM::FaultStickSet.
*/

mExpClass(MPEEngine) FaultStickSetEditor : public ObjectEditor
{
public:
				FaultStickSetEditor(EM::FaultStickSet&);

    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    void			setSceneIdx(int idx)	{ sceneidx_ = idx; }

    void			setEditIDs(const TypeSet<EM::PosID>* editpids);
    void			getEditIDs(TypeSet<EM::PosID>&) const override;

    void			setLastClicked(const EM::PosID&);
    void			setSowingPivot(const Coord3);

    void			setZScale(float);
    void			setScaleVector(const Coord3& v);
				//!< x'=x, y'=v1*x*+v2*y, z'=v3*z

    void			getInteractionInfo( EM::PosID& insertpid,
				    const MultiID* pickedmid,
				    const char* pickednm,
				    Pos::GeomID pickedgeomid,const Coord3& pos,
				    const Coord3* posnorm=0) const;

    bool			removeSelection(const Selector<Coord3>&);
    const EM::PosID		getNearestStick(const Coord3& pos,
						Pos::GeomID pickedgeomid,
						const Coord3* normal) const;
protected:
				~FaultStickSetEditor();

    float		distToStick(int sticknr,
				    const MultiID* pickedmid,
				    const char* pickednm,
				    Pos::GeomID pickedgeomid,const Coord3& pos,
				    const Coord3* posnorm) const;
    bool		getNearestStick(int& sticknr,
				const MultiID* pickedmid,
				const char* pickednm,
				Pos::GeomID pickedgeomid,const Coord3& pos,
				const Coord3* posnorm) const;
    void		getPidsOnStick(EM::PosID& insertpid,int sticknr,
				const Coord3& pos) const;

    Geometry::ElementEditor*	createEditor() override;
    Coord3			scalevector_;
    Coord			xtrans_;
    Coord			ytrans_;
    int				sceneidx_;

    int				getLastClickedStick() const;

    void			cloneMovingNode(CallBacker*) override;

    const TypeSet<EM::PosID>*	editpids_;

    Coord3			sowingpivot_;
    TypeSet<Coord3>		sowinghistory_;

};

} // namespace MPE
