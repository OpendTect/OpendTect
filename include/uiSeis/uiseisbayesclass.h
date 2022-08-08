#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uivarwizard.h"
#include "uistring.h"

class uiSeisBayesPDFInp;
class uiSeisBayesNorm;
class uiSeisBayesSeisInp;
class uiSeisBayesOut;


/*!\brief 'Server' for Seismic Bayesian Inversion. */

mExpClass(uiSeis) uiSeisBayesClass : public uiVarWizard
{ mODTextTranslationClass(uiSeisBayesClass);
public:

			uiSeisBayesClass(uiParent*,bool is2d);
			~uiSeisBayesClass();

    void		raiseCurrent() override;

protected:

    bool		is2d_;

    uiSeisBayesPDFInp*	inppdfdlg_;
    uiSeisBayesNorm*	normdlg_;
    uiSeisBayesSeisInp*	inpseisdlg_;
    uiSeisBayesOut*	outdlg_;

    void		doPart() override;
    void		closeDown() override;

    void		getInpPDFs();
    void		inpPDFsGot(CallBacker*);

    void		getNorm();
    void		normGot(CallBacker*);

    void		getInpSeis();
    void		inpSeisGot(CallBacker*);

    void		doOutput();
    void		outputDone(CallBacker*);

};


