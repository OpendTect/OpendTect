#ifndef uistratsynthcrossplot_h
#define uistratsynthcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratsynthcrossplot.h,v 1.4 2011-01-20 15:09:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "datapack.h"

class uiLabel;
class SeisTrcBuf;
class uiGenInput;
class uiComboBox;
namespace Strat { class LayerModel; }
class uiAttribDescSetBuild;
class uiStratLaySeqAttribSetBuild;


/*!\brief Dialog specifying what to crossplot */

mClass uiStratSynthCrossplot : public uiDialog
{
public:
				uiStratSynthCrossplot(uiParent*,DataPack::ID,
						      const Strat::LayerModel&);
				~uiStratSynthCrossplot();

protected:

    const Strat::LayerModel&	lm_;
    DataPack::ID		packid_;

    uiAttribDescSetBuild*	seisattrfld_;
    uiStratLaySeqAttribSetBuild* layseqattrfld_;
    uiLabel*			emptylbl_;
    uiComboBox*			reflvlfld_;
    uiGenInput*			snapfld_;
    uiGenInput*			extrwinfld_;

    bool			acceptOK(CallBacker*);

};



#endif
