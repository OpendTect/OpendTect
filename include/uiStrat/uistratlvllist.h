#ifndef uistratlvllist_h
#define uistratlvllist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck / Bruno
 Date:          July 2007 /Sept 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uilistbox.h"

mClass uiStratLvlList : public uiLabeledListBox
{
public:

				uiStratLvlList(uiParent*);
				~uiStratLvlList();
protected:

    void                        editLevel(bool);

    void                        fill(CallBacker*);
    void			removeLvl(CallBacker*);
    void                        selLvlChgCB(CallBacker*);
    void                        rClickLvlCB(CallBacker*);

public:
    void                        setLevels();
    void			setIsLocked(bool yn) { islocked_ = yn; }
protected:
    bool			islocked_;

    bool                	anychange_;

public:
    void                	setNoChg()    		{ anychange_ = false; }
    bool                	anyChg() const          { return anychange_; }

};


#endif
