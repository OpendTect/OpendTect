#ifndef uiscalebaritem_h
#define uiscalebaritem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "treeitem.h"

namespace Pick { class Set; }

mClass(Annotations) ScaleBarSubItem : public SubItem
{
public:
    			ScaleBarSubItem(Pick::Set&,int displayid=-1);
    bool		init();
    static const char*	sKeyManager() 	{ return "ScaleBarAnnotations"; }

protected:
			~ScaleBarSubItem()	{ removeStuff(); }

    const char*		parentType() const;
    void		fillStoragePar(IOPar&) const;

    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		propertyChange(CallBacker*);

    const char*		managerName() const		{ return sKeyManager();}

    MenuItem		propmnuitem_;
    int			orientation_;

    static const char*		sKeyOrientation()      { return "Orientation"; }
    static const char*		sKeyLineWidth()	       { return "Line width"; }
};


#endif
