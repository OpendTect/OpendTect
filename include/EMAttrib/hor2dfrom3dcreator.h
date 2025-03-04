#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "executor.h"
#include "posgeomid.h"

namespace EM { class Horizon3D; class Horizon2D; }
namespace Survey { class Geometry2D; }
class BufferStringSet;

/*!
\brief ExecutorGroup to create EM::Horizon2D from EM::Horizon3D.
*/

mExpClass(EMAttrib) Hor2DFrom3DCreatorGrp : public ExecutorGroup
{
public:
				Hor2DFrom3DCreatorGrp(const EM::Horizon3D&,
						      EM::Horizon2D&);
				~Hor2DFrom3DCreatorGrp();

   void				init(const TypeSet<Pos::GeomID>&);

protected:

    const EM::Horizon3D&	hor3d_;
    EM::Horizon2D&		hor2d_;

};


/*!
\brief Executor to create EM::Horizon2D from EM::Horizon3D.
*/

mExpClass(EMAttrib) Hor2DFrom3DCreator : public Executor
{
public:
				Hor2DFrom3DCreator(const EM::Horizon3D&,
						   EM::Horizon2D&);
				~Hor2DFrom3DCreator();

    bool			setCreator(Pos::GeomID);
    int				nextStep() override;
    od_int64			nrDone() const override { return nrdone_; }
    od_int64			totalNr() const override { return totalnr_; }

protected:

    const EM::Horizon3D&	hor3d_;
    EM::Horizon2D&		hor2d_;
    int				nrdone_		= 0;
    int				totalnr_;
    Pos::GeomID			geomid_;

    const Survey::Geometry2D*	geom2d_		= nullptr;
};
