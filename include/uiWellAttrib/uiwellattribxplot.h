#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiwellattribxplot.h,v 1.1 2008-04-17 09:09:05 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class uiListBox;
class uiComboBox;
class uiGenInput;
class IOObj;
namespace Attrib { class DescSet; }


class uiWellAttribCrossPlot : public uiDialog
{
public:
			uiWellAttribCrossPlot(uiParent*,const Attrib::DescSet&);
			~uiWellAttribCrossPlot();

    void		setDescSet(const Attrib::DescSet&);

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

    void		adsChg();
    void		initWin(CallBacker*);

    bool		acceptOK(CallBacker*);
};


#endif
