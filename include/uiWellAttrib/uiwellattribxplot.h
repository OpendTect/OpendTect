#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiwellattribxplot.h,v 1.7 2009-04-01 07:38:39 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class IOObj;
class uiListBox;
class uiComboBox;
class uiGenInput;
class DataPointSet;
class BufferStringSet;
class uiPosFilterSetSel;
class uiDataPointSet;
namespace Attrib { class DescSet; }


mClass uiWellAttribCrossPlot : public uiDialog
{
public:
					uiWellAttribCrossPlot(uiParent*,
						const Attrib::DescSet&);
					~uiWellAttribCrossPlot();

    void				setDescSet(const Attrib::DescSet&);
    Notifier<uiWellAttribCrossPlot>	pointsSelected;
    Notifier<uiWellAttribCrossPlot>	pointsToBeRemoved;

    const DataPointSet&			getDPS() const;

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
    uiDataPointSet*	uidps_;

    void		adsChg();
    bool		extractWellData(const BufferStringSet&,
	    				const BufferStringSet&,
					ObjectSet<DataPointSet>&);
    bool		extractAttribData(DataPointSet&,int);

    void		initWin(CallBacker*);
    void		showSelPts(CallBacker*);
    void		removeSelPts(CallBacker*);

    bool		acceptOK(CallBacker*);
};


#endif
