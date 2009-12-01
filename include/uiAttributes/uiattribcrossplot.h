#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.16 2009-12-01 09:45:55 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class DataPointSet;
class DataPointSetDisplayMgr;
class uiPosProvider;
class uiPosFilterSetSel;
class uiSeis2DLineNameSel;
class uiListBox;
namespace Attrib { class DescSet; }
namespace PosInfo { class Line2DData; }


mClass uiAttribCrossPlot : public uiDialog
{
public:
					uiAttribCrossPlot(uiParent*,
						  const Attrib::DescSet&);
					~uiAttribCrossPlot();

    void				setDescSet(const Attrib::DescSet&);
    void				setDisplayMgr(
					    DataPointSetDisplayMgr* dispmgr )
					{ dpsdispmgr_ = dispmgr; }

    const DataPointSet&			getDPS() const;

protected:

    const Attrib::DescSet& 		ads_;
    PosInfo::Line2DData* 		l2ddata_;

    uiListBox*				attrsfld_;
    uiPosProvider*			posprovfld_;
    uiPosFilterSetSel*			posfiltfld_;
    uiSeis2DLineNameSel*		lnmfld_;
    DataPointSet*			curdps_;
    DataPointSetDisplayMgr*		dpsdispmgr_;

    void				adsChg();
    void				useLineName(bool);
    void				initWin(CallBacker*);
    void				lnmChg(CallBacker*);

    bool				acceptOK(CallBacker*);
};


#endif
