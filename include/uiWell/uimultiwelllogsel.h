#ifndef uimultiwelllogsel_h
#define uimultiwelllogsel_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id: uimultiwelllogsel.h,v 1.1 2011-01-21 14:44:49 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class BufferStringSet;
class IOObj;
class MultiID;
class uiComboBox;
class uiGenInput;
class uiListBox;

/*! brief enables selecting logs for all wells at the same time 
  + a few well stuff like zrange and/or extraction mode ... !*/

mClass uiMultiWellLogSel : public uiGroup
{
public:
			uiMultiWellLogSel(uiParent*);

    void		getSelLogNames(BufferStringSet&) const;
    void		getSelWellNames(BufferStringSet&) const;
    void		getSelWellIDs(TypeSet<MultiID>&) const;

protected :

    ObjectSet<IOObj>	wellobjs_;
    TypeSet<MultiID>	wellids_;

    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
    uiComboBox*		topmarkfld_;
    uiComboBox*		botmarkfld_;
    uiGenInput*		abovefld_;
    uiGenInput*		belowfld_;

    void		init();
};

#endif
