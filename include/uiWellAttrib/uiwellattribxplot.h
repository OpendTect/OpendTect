#ifndef uiwellattribxplot_h
#define uiwellattribxplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiwellattribxplot.h,v 1.10 2010-08-04 14:49:36 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class IOObj;
class uiListBox;
class uiComboBox;
class uiGenInput;
class DataPointSet;
class DataPointSetDisplayMgr;
class BufferStringSet;
class uiPosFilterSetSel;
namespace Attrib { class DescSet; }


mClass uiWellAttribCrossPlot : public uiDialog
{
public:
					uiWellAttribCrossPlot(uiParent*,
						const Attrib::DescSet&);
					~uiWellAttribCrossPlot();

    void				setDescSet(const Attrib::DescSet&);

    const DataPointSet&			getDPS() const;
    void				setDisplayMgr(
	    					DataPointSetDisplayMgr* mgr)
					{ dpsdispmgr_ = mgr; }

protected:

    const Attrib::DescSet& ads_;
    ObjectSet<IOObj>	wellobjs_;

    uiListBox*		attrsfld_;
    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
    uiComboBox*		topmarkfld_;
    uiComboBox*		botmarkfld_;
    uiGenInput*		radiusfld_;
    uiGenInput*		abovefld_;
    uiGenInput*		belowfld_;
    uiGenInput*		logresamplfld_;
    uiPosFilterSetSel*	posfiltfld_;
    DataPointSet*	curdps_;
    DataPointSetDisplayMgr* dpsdispmgr_;

    void		adsChg();
    bool		extractWellData(const BufferStringSet&,
	    				const BufferStringSet&,
					ObjectSet<DataPointSet>&);
    bool		extractAttribData(DataPointSet&,int);

    void		initWin(CallBacker*);

    bool		acceptOK(CallBacker*);
};


#endif
