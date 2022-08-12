#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2005
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emeditor.h"

namespace EM { class Fault3D; }
namespace Geometry { class FaultStickSurface; }

template <class T> class Selector;

namespace MPE
{

/*!
\brief ObjectEditor to edit EM::Fault3D.
*/

mExpClass(MPEEngine) FaultEditor : public ObjectEditor
{
public:
				FaultEditor(EM::Fault3D&);

    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    void			setSceneIdx(int idx)	{ sceneidx_ = idx; }

    void			setLastClicked(const EM::PosID&);
    void			setSowingPivot(const Coord3);


    void			setZScale(float);
    void			setScaleVector(const Coord3& v);
				//!< x'=x, y'=v1*x*+v2*y, z'=v3*z

    void			getInteractionInfo(bool& makenewstick,
				    EM::PosID& insertpid,const Coord3& pos,
				    const Coord3* posnormal=0) const;

    bool			removeSelection(const Selector<Coord3>&);
    const EM::PosID		getNearstStick(const Coord3& pos,
					       const Coord3* posnormal) const;
    const EM::PosID		getNearstStick(EM::SectionID&,
					       const Coord3& pos,
					       const Coord3* posnormal) const
				{ return getNearstStick(pos,posnormal); }

protected:
    float		distToStick(const Geometry::FaultStickSurface&,
				    int curstick,const Coord3& pos,
				    const Coord3* posnormal) const;
    float		panelIntersectDist(const Geometry::FaultStickSurface&,
				    int sticknr,const Coord3& mousepos,
				    const Coord3& posnormal) const;
    int			getSecondKnotNr(const Geometry::FaultStickSurface&,
				    int sticknr,const Coord3& mousepos) const;

    float		getNearestStick(int& stick,
			    const Coord3& pos,const Coord3* posnormal) const;
    bool		getInsertStick(int& stick,
			    const Coord3& pos,const Coord3* posnormal) const;
    void		getPidsOnStick( EM::PosID& insertpid,int stick,
			    const Coord3& pos) const;

    Geometry::ElementEditor*	createEditor() override;
    Coord3			scalevector_;
    int				sceneidx_;

    int				getLastClickedStick() const;

    void			cloneMovingNode(CallBacker*) override;

    Coord3			sowingpivot_;
    TypeSet<Coord3>		sowinghistory_;
};

}  // namespace MPE
