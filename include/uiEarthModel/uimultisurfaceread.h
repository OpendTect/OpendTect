#ifndef uimultisurfaceread_h
#define uimultisurfaceread_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uimultisurfaceread.h,v 1.1 2006-03-23 14:53:53 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiiosurface.h"

class IODirEntryList;
class uiLabeledListBox;

/*! \brief ui for multiple surface read */

class uiMultiSurfaceRead : public uiIOSurface
{
public:
			uiMultiSurfaceRead(uiParent*,bool ishor);
			~uiMultiSurfaceRead()	{}

    void		getSurfaceIds(TypeSet<MultiID>&) const;
    void		getSurfaceSelection(EM::SurfaceIODataSelection&) const;

protected:

    IODirEntryList*	entrylist_;
    uiLabeledListBox*	surfacefld_;

    void		selCB(CallBacker*);
};


#endif
