#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "undo.h"
#include "emposid.h"
#include "coord.h"
#include "color.h"

namespace Geometry { class ParametricSurface; }
template <class T> class Array2D;

namespace EM
{
class Horizon3D;

/*!
\brief Set position UndoEvent.
*/

mExpClass(EarthModel) EMUndo : public Undo
{
public:
			EMUndo();
			~EMUndo();

    ObjectID		getCurrentEMObjectID(bool forredo) const;
};


mExpClass(EarthModel) EMUndoEvent : public UndoEvent
{
public:
			~EMUndoEvent();

    virtual ObjectID	getObjectID() const =0;

protected:
			EMUndoEvent();
};


mExpClass(EarthModel) SetPosUndoEvent : public EMUndoEvent
{
public:
			SetPosUndoEvent(const Coord3& oldpos,const PosID&);
			~SetPosUndoEvent();

    const char*		getStandardDesc() const override;
    bool		unDo() override;
    bool		reDo() override;
    ObjectID		getObjectID() const override
			{ return posid_.objectID(); }

protected:
    PosID		posid_;
    Coord3		savedpos_;

    static const char*	savedposstr_;
};


/*!
\brief UndoEvent for setting all positions on a EM::Horizon3D section.
*/

mExpClass(EarthModel) SetAllHor3DPosUndoEvent : public EMUndoEvent
{
public:
			SetAllHor3DPosUndoEvent(Horizon3D*,
				    Array2D<float>*);
			SetAllHor3DPosUndoEvent(Horizon3D*,
				    Array2D<float>*,const RowCol& oldorigin);

    mDeprecated("Use without SectionID")
			SetAllHor3DPosUndoEvent(Horizon3D*,SectionID,
				    Array2D<float>*);
    mDeprecated("Use without SectionID")
			SetAllHor3DPosUndoEvent(Horizon3D*,SectionID,
				    Array2D<float>*,const RowCol& oldorigin);

    const char*		getStandardDesc() const override;
    bool		unDo() override;
    bool		reDo() override;
    ObjectID		getObjectID() const override;

protected:
    bool		setArray(const Array2D<float>&, const RowCol& origin);
			~SetAllHor3DPosUndoEvent();

    Horizon3D*		horizon_;
    SectionID		sid_		= SectionID::def();
    RowCol		oldorigin_;
    RowCol		neworigin_;
    Array2D<float>*	oldarr_;
    Array2D<float>*	newarr_;
};


/*!
\brief UndoEvent for setting position attribute.
*/

mExpClass(EarthModel) SetPosAttribUndoEvent : public EMUndoEvent
{
public:
			SetPosAttribUndoEvent(const PosID&,int attrib,
					      bool yn );

    const char*		getStandardDesc() const override;
    bool		unDo() override;
    bool		reDo() override;
    ObjectID		getObjectID() const override
			{ return posid_.objectID(); }

protected:
			~SetPosAttribUndoEvent();

    PosID		posid_;
    bool		yn_;
    int			attrib_;
};


/*!
\brief Saves information from a EMObject::changePosID call.
*/

mExpClass(EarthModel) PosIDChangeEvent : public EMUndoEvent
{
public:
			PosIDChangeEvent(const PosID& from,
					 const PosID& to,
					 const Coord3& tosprevpos);

    const char*		getStandardDesc() const override;
    bool		unDo() override;
    bool		reDo() override;
    ObjectID		getObjectID() const override { return to_.objectID(); }

protected:
			~PosIDChangeEvent();

    const PosID		from_;
    const PosID		to_;
    Coord3		savedpos_;
};


/*!
\brief UndoEvent to set preferred Color.
*/

mExpClass(EarthModel) SetPrefColorEvent : public EMUndoEvent
{
public:
			SetPrefColorEvent(const ObjectID&,
					  const OD::Color& oldcol,
					  const OD::Color& newcol);
    const char*		getStandardDesc() const override;
    bool		unDo() override;
    bool		reDo() override;
    ObjectID		getObjectID() const override { return objectid_; }

protected:
			~SetPrefColorEvent();

    const ObjectID	objectid_;
    const OD::Color	oldcolor_;
    const OD::Color	newcolor_;
};

} // namespace EM
