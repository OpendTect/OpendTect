#ifndef uimultiwelllogsel_h
#define uimultiwelllogsel_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id: uimultiwelllogsel.h,v 1.2 2011-01-24 16:43:46 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class BufferStringSet;
class IOObj;
class MultiID;
class uiComboBox;
class uiGenInput;
class uiListBox;

/*! brief enables selecting logs for all wells at the same time within a common 
  zrange !*/

mClass uiMultiWellLogSel : public uiGroup
{
public:
			uiMultiWellLogSel(uiParent*);
			~uiMultiWellLogSel();

    void		getSelLogNames(BufferStringSet&) const;
    void		getSelWellNames(BufferStringSet&) const;
    void		getSelWellIDs(BufferStringSet&) const;

    void		getLimitMarkers(const char* top,const char* bot) const;
    void		getLimitDists(float& top,float& bot) const;

    void		update(); //call this when data changed

protected :

    ObjectSet<IOObj>	wellobjs_;
    TypeSet<MultiID>	wellids_;

    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
    uiComboBox*		topmarkfld_;
    uiComboBox*		botmarkfld_;
    uiGenInput*		abovefld_;
    uiGenInput*		belowfld_;
};

#endif
