#ifndef uimultiwelllogsel_h
#define uimultiwelllogsel_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id: uimultiwelllogsel.h,v 1.4 2012-02-24 14:27:54 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "bufstringset.h"

class IOObj;
class MultiID;
class uiComboBox;
class uiGenInput;
class uiListBox;

namespace Well { class MarkerSet; } 

/*! brief: UI facilities to select/extract log data with zrg and extraction methods!*/


mClass uiWellZRangeSel : public uiGroup
{
public:

			uiWellZRangeSel(uiParent*,bool withzstep=false,
					bool withresampling=false);

    void		clear();

    void		addMarkers(const Well::MarkerSet&);
    void		addMarkers(const BufferStringSet&);

    const char*		getTopMarker() const;
    const char*		getBottomMarker() const;
    void		getLimitMarkers(BufferString& t,BufferString& b) const;
    void		getLimitDists(float& top,float& bot) const;

    float		getStep() const;
    int			getResamplingType() const;

protected:

    BufferStringSet	markernms_;

    uiComboBox*		topmarkfld_;
    uiComboBox*		botmarkfld_;
    uiGenInput*		abovefld_;
    uiGenInput*		belowfld_;
    uiGenInput*		stepfld_;

    uiGroup*		attach_;

    uiGenInput*		logresamplfld_;
};



mClass uiMultiWellLogSel : public uiWellZRangeSel
{
public:
			uiMultiWellLogSel(uiParent*,bool withresampling=false);
			~uiMultiWellLogSel();

    void		getSelLogNames(BufferStringSet&) const;
    void		getSelWellNames(BufferStringSet&) const;
    void		getSelWellIDs(BufferStringSet&) const;

    void		update(); //call this when data changed

protected :

    ObjectSet<IOObj>	wellobjs_;
    TypeSet<MultiID>	wellids_;

    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
};

#endif
