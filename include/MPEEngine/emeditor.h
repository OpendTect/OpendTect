#ifndef emeditor_h
#define emeditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "callback.h"
#include "emposid.h"
#include "factory.h"
#include "position.h"
#include "refcount.h"

class BufferStringSet;
template <class T> class TypeSet;

namespace EM { class EMObject; class EdgeLineSet; };
namespace Geometry { class ElementEditor; };

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

mClass(MPEEngine) ObjectEditor : public CallBacker
{ mRefCountImpl( ObjectEditor );
public:
    			ObjectEditor( EM::EMObject& );

    const EM::EMObject&	emObject() const	{ return emobject; }

    virtual void	startEdit( const EM::PosID& );
    virtual bool	setPosition(const Coord3&);
    virtual void	finishEdit();

    bool		canSnapAfterEdit(const EM::PosID&) const;
    bool		getSnapAfterEdit() const;
    void		setSnapAfterEdit(bool yn);

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

    void			restartInteractionLine( const EM::PosID& );
    				/*!<\note Object does only have one line. If
					the provided sectionID differs from the
				 	existing line's, the sectionID of the
				 	existing line will be changed. */

    bool			closeInteractionLine( bool doit = true );
    				/*!<If doit is false, no change will be done
				    and the return status indicates wether it
				    can be done. */
    bool			interactionLineInteraction( const EM::PosID&,
	    						    bool doit = true );
    				/*!<If pos is on the line, but not on the first
				    node, it will be cut off at that location.
				    If it is not on the line, or on the first
				    node, the line will be extended to pos.
				    If doit is false, no change will be done
				    and the return status indicates wether it
				    can be done. */
				    
    const EM::EdgeLineSet*	getInteractionLine() const;

    static void			enableNodeCloning(bool yn=true);

protected:

    virtual bool			setPosition(const EM::PosID&,
	    					    const Coord3&);
    Geometry::ElementEditor*		getEditor( const EM::SectionID& );
    const Geometry::ElementEditor*	getEditor( const EM::SectionID& ) const;
    virtual Geometry::ElementEditor*	createEditor( const EM::SectionID& )=0;

    void				editPosChangeTrigger(CallBacker*);
    void				emSectionChange(CallBacker*);

    virtual void			getAlongMovingNodes( const EM::PosID&,
	    				    TypeSet<EM::PosID>&,
					    TypeSet<float>* ) const;
    					/*!<Gets the positions that are moved
					    along and their corresponding
					    factors. */

    virtual void			cloneMovingNode()		{};

    EM::EMObject&			emobject;
    EM::PosID				movingnode;
    EM::EdgeLineSet*			interactionline;
    Coord3				startpos;
    TypeSet<EM::PosID>			changedpids;
    TypeSet<EM::PosID>			alongmovingnodes;
    TypeSet<Coord3>			alongmovingnodesstart;
    TypeSet<float>			alongmovingnodesfactors;

private:
    ObjectSet<Geometry::ElementEditor>	geeditors;
    TypeSet<EM::SectionID>		sections;

    bool				snapafterthisedit;

    bool				snapafteredit;
};


mDefineFactory1Param( MPEEngine, ObjectEditor, EM::EMObject&, EditorFactory );


};

#endif


