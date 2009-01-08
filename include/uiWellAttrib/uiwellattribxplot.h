#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiwellattribxplot.h,v 1.6 2009-01-08 09:18:28 cvsranojay Exp $
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
namespace Pick { class Set; }


mClass uiWellAttribCrossPlot : public uiDialog
{
public:
					uiWellAttribCrossPlot(uiParent*,
						const Attrib::DescSet&);
					~uiWellAttribCrossPlot();

    void				setDescSet(const Attrib::DescSet&);
    Notifier<uiWellAttribCrossPlot>	pointsSelected;

    const Pick::Set*			getSelectedPts() const	
    					{ return selptps_; }

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
    Pick::Set*		selptps_;

    void		adsChg();
    bool		extractWellData(const BufferStringSet&,
	    				const BufferStringSet&,
					ObjectSet<DataPointSet>&);
    bool		extractAttribData(DataPointSet&,int);

    void		initWin(CallBacker*);
    void		createPickSet(CallBacker*);

    bool		acceptOK(CallBacker*);
};


#endif
