#ifndef emundo_h
#define emundo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "undo.h"
#include "emposid.h"
#include "position.h"
#include "color.h"

class IOPar;
namespace Geometry { class ParametricSurface; };
template <class T> class Array2D;

namespace EM
{
class Horizon3D;

/*!
\ingroup EarthModel
\brief Set position UndoEvent.
*/

mExpClass(EarthModel) SetPosUndoEvent : public UndoEvent
{
public:
			SetPosUndoEvent( const Coord3& oldpos,
					    const EM::PosID& );

    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    EM::PosID		posid;
    Coord3		savedpos;

    static const char*	savedposstr;
};


/*!
\ingroup EarthModel
\brief UndoEvent for setting all positions on a EM::Horizon3D section.
*/

mExpClass(EarthModel) SetAllHor3DPosUndoEvent : public UndoEvent
{
public:
			SetAllHor3DPosUndoEvent(EM::Horizon3D*,EM::SectionID,
				    Array2D<float>*);
			SetAllHor3DPosUndoEvent(EM::Horizon3D*,EM::SectionID,
				    Array2D<float>*,const RowCol& oldorigin);

    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    bool		setArray(const Array2D<float>&, const RowCol& origin);
			~SetAllHor3DPosUndoEvent();

    EM::Horizon3D*	horizon_;
    EM::SectionID	 sid_;
    RowCol		oldorigin_;
    RowCol		neworigin_;
    Array2D<float>*	oldarr_;
    Array2D<float>*	newarr_;
};


/*!
\ingroup EarthModel
\brief UndoEvent for setting position attribute.
*/

mExpClass(EarthModel) SetPosAttribUndoEvent : public UndoEvent
{
public:
			SetPosAttribUndoEvent( const EM::PosID&,
						  int attrib, bool yn );

    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    EM::PosID		posid;
    bool		yn;
    int			attrib;
};


/*!
\ingroup EarthModel
\brief Saves information from a EMObject::changePosID call.
*/

mExpClass(EarthModel) PosIDChangeEvent : public UndoEvent
{
public:
    			PosIDChangeEvent( const EM::PosID& from,
					  const EM::PosID& to,
					  const Coord3& tosprevpos );
    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    const EM::PosID	from;
    const EM::PosID	to;
    Coord3		savedpos;
};


/*!
\ingroup EarthModel
\brief UndoEvent to set preferred Color.
*/

mExpClass(EarthModel) SetPrefColorEvent : public UndoEvent
{
public:
    			SetPrefColorEvent(const EM::ObjectID&,
					  const Color& oldcol,
					  const Color& newcol);
    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    const EM::ObjectID	objectid_;
    const Color		oldcolor_;
    const Color		newcolor_;
};


}; // Namespace


#endif

