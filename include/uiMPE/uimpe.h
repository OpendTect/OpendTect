#ifndef uiemeditor_h
#define uiemeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id: uimpe.h,v 1.3 2005-09-27 19:16:22 cvskris Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "callback.h"

class uiParent;


namespace MPE
{

class ObjectEditor;

class uiEMEditor : public CallBackClass
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


typedef uiEMEditor*(*uiEMEditorCreationFunc)(uiParent*,MPE::ObjectEditor*);


class uiEMEditorFactory
{
public:
    void		addFactory( uiEMEditorCreationFunc f );
    uiEMEditor*		create( uiParent*, MPE::ObjectEditor* e ) const;

protected:
    TypeSet<uiEMEditorCreationFunc>	funcs;

};


class uiMPEEngine 
{
public:
    uiEMEditorFactory		editorfact;
};


uiMPEEngine& uiMPE();

};


#endif
