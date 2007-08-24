#ifndef uimultisurfaceread_h
#define uimultisurfaceread_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uimultisurfaceread.h,v 1.5 2007-08-24 11:56:55 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiiosurface.h"

class IODirEntryList;
class uiLabeledListBox;

/*! \brief ui for multiple surface read */

class uiMultiSurfaceRead : public uiIOSurface
{
public:
			uiMultiSurfaceRead(uiParent*,const char* type);
			~uiMultiSurfaceRead();

    void		getSurfaceIds(TypeSet<MultiID>&) const;
    void		getSurfaceSelection(EM::SurfaceIODataSelection&) const;

    Notifier<uiMultiSurfaceRead>	singleSurfaceSelected;

protected:

    IODirEntryList*	entrylist_;
    uiLabeledListBox*	surfacefld_;

    void		dClck(CallBacker*);
    void		selCB(CallBacker*);
};


#endif
