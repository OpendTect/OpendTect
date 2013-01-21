#ifndef uimpe_h
#define uimpe_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "bufstring.h"
#include "callback.h"
#include "color.h"
#include "draw.h"
#include "emposid.h"
#include "emseedpicker.h"
#include "randcolor.h"

#include "uigroup.h"

class uiParent;

namespace Attrib { class DescSet; };


namespace MPE
{

class ObjectEditor;
class SectionTracker;

/*!
\ingroup uiMPE
\brief Interface for the ui interaction with MPE::ObjectEditor.
Object is implemented in separate classes inheriting uiEMEditor that
can be created by:

\code
    PtrMan<uiEMEditor> uieditor =
    	MPE::uiMPE().editorfact.create( uiparent, editor );
\endcode
*/

mExpClass(uiMPE) uiEMEditor : public CallBacker
{
public:
    			uiEMEditor(uiParent*);
    virtual		~uiEMEditor();
    virtual void	setActiveNode( const EM::PosID& n ) { node=n; }
    virtual void	createNodeMenus(CallBacker*) {}
    virtual void	handleNodeMenus(CallBacker*) {}
    virtual void	createInteractionLineMenus(CallBacker*) {}
    virtual void	handleInteractionLineMenus(CallBacker*) {}

protected:
    EM::PosID		node;
    uiParent*		parent;
};


/*! Factory function that can produce a MPE::uiEMEditor* given a 
    uiParent* and a MPE::ObjectEditor*. \note that the function should
    return a zero pointer if the MPE::ObjectEditor* is of the wrong kind. */


typedef uiEMEditor*(*uiEMEditorCreationFunc)(uiParent*,MPE::ObjectEditor*);

/*! Factory that is able to create MPE::uiEMEditor objects given a uiParent*
    and a MPE::ObjectEditor*. Each class that wants to be able to procuce
    instances of itself must register itself with the addFactory startup. */

mExpClass(uiMPE) uiEMEditorFactory
{
public:
    void		addFactory( uiEMEditorCreationFunc f );
    uiEMEditor*		create( uiParent*, MPE::ObjectEditor* e ) const;
			/*!<Iterates through all added factory functions
			    until one of the returns a non-zero pointer. */

protected:
    TypeSet<uiEMEditorCreationFunc>	funcs;

};


/*! Interface to track-setup groups. Implementations can be retrieved through
    MPE::uiSetupGroupFactory. */


mExpClass(uiMPE) uiSetupGroup : public uiGroup
{
public:
			uiSetupGroup(uiParent*,const char* helpref);
    virtual void	setSectionTracker(SectionTracker*)	{}
    virtual void	setAttribSet(const Attrib::DescSet*)	{}
    virtual void	setMode(const EMSeedPicker::SeedModeOrder) {}
    virtual int		getMode()				=0;
    virtual void	setColor(const Color&)			{}
    virtual const Color& getColor()				=0;
    virtual void	setMarkerStyle(const MarkerStyle3D&)	{}
    virtual const MarkerStyle3D& getMarkerStyle()		=0;
    virtual void	setAttribSelSpec(const Attrib::SelSpec*) {}
    virtual bool	isSameSelSpec(const Attrib::SelSpec*) const
			{ return true; }

    virtual NotifierAccess*	modeChangeNotifier()		{ return 0; }
    virtual NotifierAccess*	propertyChangeNotifier()	{ return 0; }
    virtual NotifierAccess*	eventChangeNotifier()		{ return 0; }
    virtual NotifierAccess*	similartyChangeNotifier()	{ return 0; }

    virtual bool	commitToTracker(bool& fieldchg) const   { return true; }
    virtual bool	commitToTracker() const; 

    BufferString	helpref_;
};
    

/*! Factory function that can produce a MPE::uiSetupGroup* given a 
    uiParent* and an Attrib::DescSet*. */

typedef uiSetupGroup*(*uiSetupGrpCreationFunc)(uiParent*,const char* typestr,	
					       const Attrib::DescSet*);

/*! Factory that is able to create MPE::uiSetupGroup* given a uiParent*,
    and an Attrib::DescSet*. Each class that wants to
    be able to procuce instances of itself must register itself with the
    addFactory startup. */

mExpClass(uiMPE) uiSetupGroupFactory
{
public:
    void		addFactory(uiSetupGrpCreationFunc f, const char* name);
    uiSetupGroup*	create(const char* nm,uiParent*,const char* typestr,
	    		       const Attrib::DescSet*);
			/*!<Iterates through all added factory functions
			    until one of the returns a non-zero pointer. */

protected:
    BufferStringSet			names_;
    TypeSet<uiSetupGrpCreationFunc>	funcs;

};


/*!
\ingroup uiMPE
\brief Holder class for MPE ui-factories.
  Is normally only retrieved by MPE::uiMPE().
*/


mExpClass(uiMPE) uiMPEEngine 
{
public:
    uiEMEditorFactory		editorfact;
    uiSetupGroupFactory		setupgrpfact;
};



/*!
\brief Access function for an instance (and normally the only instance) of
  MPE::uiMPEEngine.
*/
mGlobal(uiMPE) uiMPEEngine& uiMPE();

};

#endif

