#ifndef uiwellattribxplot_h
#define uiwellattribxplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiwellattribxplot.h,v 1.13 2012/01/05 06:28:17 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class IOObj;
class uiListBox;
class uiGenInput;
class DataPointSet;
class DataPointSetDisplayMgr;
class BufferStringSet;
class uiDataPointSet;
class uiMultiWellLogSel;
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
    ObjectSet<uiDataPointSet>	dpsset_;

    uiListBox*		attrsfld_;
    uiGenInput*		radiusfld_;
    uiGenInput*		logresamplfld_;
    uiMultiWellLogSel*	welllogselfld_;
    uiPosFilterSetSel*	posfiltfld_;
    DataPointSet*	curdps_;
    DataPointSetDisplayMgr* dpsdispmgr_;

    void		adsChg();
    bool		extractWellData(const BufferStringSet&,
	    				const BufferStringSet&,
					ObjectSet<DataPointSet>&);
    bool		extractAttribData(DataPointSet&,int);

    bool		acceptOK(CallBacker*);
    void		initWin(CallBacker*);
};


#endif
