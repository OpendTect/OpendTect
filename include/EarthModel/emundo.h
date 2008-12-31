#ifndef emundo_h
#define emundo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emundo.h,v 1.4 2008-12-31 09:08:40 cvsranojay Exp $
________________________________________________________________________


-*/

#include "undo.h"
#include "emposid.h"
#include "position.h"

class IOPar;
namespace Geometry { class ParametricSurface; };
template <class T> class Array2D;

namespace EM
{
class Horizon3D;

mClass SetPosUndoEvent : public UndoEvent
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


// Undo for setting all positions on a horizon3d-section
mClass SetAllHor3DPosUndoEvent : public UndoEvent
{
public:
			SetAllHor3DPosUndoEvent(EM::Horizon3D*,EM::SectionID,
						Array2D<float>*);

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


mClass SetPosAttribUndoEvent : public UndoEvent
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


/*! Saves information from a EMObject::changePosID call */

mClass PosIDChangeEvent : public UndoEvent
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


}; // Namespace


#endif
