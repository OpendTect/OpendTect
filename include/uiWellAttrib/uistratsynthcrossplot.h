#ifndef uistratsynthcrossplot_h
#define uistratsynthcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratsynthcrossplot.h,v 1.1 2011-01-06 15:24:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class SeisTrcBuf;
namespace Strat { class LayerModel; }
class uiAttribDescSetBuild;


/*!\brief Dialog specifying what to crossplot */

mClass uiStratSynthCrossplot : public uiDialog
{
public:
				uiStratSynthCrossplot(uiParent*,
						      const SeisTrcBuf&,
						      const Strat::LayerModel&);
				~uiStratSynthCrossplot();

protected:

    const SeisTrcBuf&		tbuf_;
    const Strat::LayerModel&	lm_;

    uiAttribDescSetBuild*	seisattrfld_;

    bool			acceptOK(CallBacker*);

};



#endif
