#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "emcommon.h"
#include "undo.h"
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

mExpClass(EarthModel) Undo : public ::Undo
{
public:

    DBKey		getCurrentEMObjectID(bool forredo) const;

};


mExpClass(EarthModel) UndoEvent : public ::UndoEvent
{
public:

    virtual DBKey	getObjectID() const =0;
};


mExpClass(EarthModel) SetPosUndoEvent : public UndoEvent
{
public:
			SetPosUndoEvent(const Coord3& oldpos,const EM::PosID&);

    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();
    DBKey		getObjectID() const { return DBKey::getInvalid(); }

protected:
    EM::PosID		posid_;
    Coord3		savedpos_;

    static const char*	savedposstr_;
};


/*!
\brief UndoEvent for setting all positions on a EM::Horizon3D section.
*/

mExpClass(EarthModel) SetAllHor3DPosUndoEvent : public UndoEvent
{
public:
			SetAllHor3DPosUndoEvent(EM::Horizon3D*,
				    Array2D<float>*);
			SetAllHor3DPosUndoEvent(EM::Horizon3D*,
				    Array2D<float>*,const RowCol& oldorigin);

    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();
    DBKey		getObjectID() const;

protected:
    bool		setArray(const Array2D<float>&, const RowCol& origin);
			~SetAllHor3DPosUndoEvent();

    EM::Horizon3D*	horizon_;
    RowCol		oldorigin_;
    RowCol		neworigin_;
    Array2D<float>*	oldarr_;
    Array2D<float>*	newarr_;
};


/*!
\brief UndoEvent for setting position attribute.
*/

mExpClass(EarthModel) SetPosAttribUndoEvent : public UndoEvent
{
public:
			SetPosAttribUndoEvent(const EM::PosID&,int attrib,
					      bool yn );

    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();
    DBKey		getObjectID() const { return DBKey::getInvalid(); }

protected:
    EM::PosID		posid_;
    bool		yn_;
    int			attrib_;
};



/*!
\brief UndoEvent to set preferred Color.
*/

mExpClass(EarthModel) SetPrefColorEvent : public UndoEvent
{
public:
			SetPrefColorEvent(const DBKey&,
					  const Color& oldcol,
					  const Color& newcol);
    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();
    DBKey		getObjectID() const { return objectid_; }

protected:
    const DBKey		objectid_;
    const Color		oldcolor_;
    const Color		newcolor_;
};

} // namespace EM
