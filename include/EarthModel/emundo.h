#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
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

    ObjectID		getCurrentEMObjectID(bool forredo) const;
};


mExpClass(EarthModel) EMUndoEvent : public UndoEvent
{
public:

    virtual ObjectID	getObjectID() const =0;
};


mExpClass(EarthModel) SetPosUndoEvent : public EMUndoEvent
{
public:
			SetPosUndoEvent(const Coord3& oldpos,const PosID&);

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
    const ObjectID	objectid_;
    const OD::Color	oldcolor_;
    const OD::Color	newcolor_;
};

} // namespace EM

