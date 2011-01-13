#ifndef uistratsynthcrossplot_h
#define uistratsynthcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratsynthcrossplot.h,v 1.3 2011-01-13 14:52:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "datapack.h"

class SeisTrcBuf;
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

    bool			acceptOK(CallBacker*);

};



#endif
