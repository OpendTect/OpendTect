#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          23-10-1996
 Contents:      Ranges
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "notify.h"
#include "emposid.h"
#include "factory.h"
#include "coord.h"

class BufferStringSet;
namespace Geometry { class ElementEditor; }
namespace EM { class Object; }

namespace MPE
{

/*!
\brief Abstraction of EM Object editing. It gives lists of which nodes that may
be moved, and in what manner.

   Editing has three easy steps:
   \code
   startEdit( pid );
   setPosition( newpos )
   finishEdit();
   \endcode

   When moving an editing node, several other nodes may be moved along
   with it. If this is possible, this can be done in different styles
   which are listed by getAlongMovingStyleNames().
*/

mExpClass(MPEEngine) ObjectEditor : public RefCount::Referenced
				  , public CallBacker
{
public:

    mDefineFactory1ParamInClass( ObjectEditor, EM::Object&, factory );

			ObjectEditor(EM::Object&);

    const EM::Object&	emObject() const	{ return *emobject_; }

    virtual void	startEdit( const EM::PosID& );
    virtual bool	setPosition(const Coord3&);
    virtual void	finishEdit();

    bool		canSnapAfterEdit(const EM::PosID&) const;
    bool		getSnapAfterEdit() const;
    void		setSnapAfterEdit(bool yn);

    void		addUser()		{ nrusers_++; }
    void		removeUser()		{ nrusers_--; }
    int			nrUsers()   const	{ return nrusers_; }

    virtual const
    BufferStringSet*	getAlongMovingStyleNames() const { return 0; }
			/*!<\returns a list with names of the different
			     styles in which nodes can follow along the moved
			     node. */

    virtual int		getAlongMovingStyle() const { return -1; }
			/*!<\returns the index of the style in the
			     list returned by getAlongMovingStyleNames(). */
    virtual void	setAlongMovingStyle(int index ) {}
			/*!<\param index refers to the
			     list returned by getAlongMovingStyleNames(). */

    virtual void	getEditIDs(TypeSet<EM::PosID>&) const;
			/*!<Gives all nodes that can be moved. */
    virtual bool	addEditID( const EM::PosID& );
			/*!<Add node for editing. Note that this may not be
			    possible, and false may be returned.  */
    virtual bool	removeEditID( const EM::PosID& );
			/*!<Remove editing node. Note that this may not be
			    possible, and false may be returned.  */

    virtual Coord3	getPosition(const EM::PosID&) const;
    virtual bool	mayTranslate1D(const EM::PosID&) const;
    virtual Coord3	translation1DDirection(const EM::PosID&) const;

    virtual bool	mayTranslate2D(const EM::PosID&) const;
    virtual Coord3	translation2DNormal(const EM::PosID&) const;

    virtual bool	mayTranslate3D(const EM::PosID&) const;

    virtual bool	maySetNormal(const EM::PosID&) const;
    virtual Coord3	getNormal(const EM::PosID&) const;

    virtual bool	maySetDirection(const EM::PosID&) const;
    virtual Coord3	getDirectionPlaneNormal(const EM::PosID&) const;
    virtual Coord3	getDirection(const EM::PosID&) const;

    Notifier<ObjectEditor>	editpositionchange;
				/*!<Won't trigger on position-changes,
				    but when new edit positions are avaliable
				    or editpositions has been removed */

    static void			enableNodeCloning(bool yn=true);

protected:
					~ObjectEditor();

    virtual bool			setPosition(const EM::PosID&,
						    const Coord3&);
    Geometry::ElementEditor*		getEditor();
    const Geometry::ElementEditor*	getEditor() const;
    virtual Geometry::ElementEditor*	createEditor()=0;

    void				editPosChangeTrigger(CallBacker*);

    virtual void			getAlongMovingNodes( const EM::PosID&,
					    TypeSet<EM::PosID>&,
					    TypeSet<float>* ) const;
					/*!<Gets the positions that are moved
					    along and their corresponding
					    factors. */

    virtual void			cloneMovingNode(CallBacker*)	{}

    RefMan<EM::Object>			emobject_;
    EM::PosID				movingnode_;
    Coord3				startpos_;
    TypeSet<EM::PosID>			changedpids_;
    TypeSet<EM::PosID>			alongmovingnodes_;
    TypeSet<Coord3>			alongmovingnodesstart_;
    TypeSet<float>			alongmovingnodesfactors_;

    int					nrusers_;

private:
    Geometry::ElementEditor*		geeditor_;

    bool				snapafterthisedit_;

    bool				snapafteredit_;
};

} // namespace MPE
