#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
