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

    enum BayesStep {
	PDFInp = 10,
	GetNorm = 11,
	SeisInp = 12,
	BayesOut = 13
    };

protected:

    bool		is2d_;

    uiSeisBayesPDFInp*	inppdfdlg_			= nullptr;
    uiSeisBayesNorm*	normdlg_			= nullptr;
    uiSeisBayesSeisInp* inpseisdlg_			= nullptr;
    uiSeisBayesOut*	outdlg_				= nullptr;

    void		doPart() override;
    void		closeDown() override;

    void		getInpPDFs();
    void		inpPDFsGotCB(CallBacker*);

    void		getNorm();
    void		normGotCB(CallBacker*);

    void		getInpSeis();
    void		inpSeisGotCB(CallBacker*);

    void		doOutput();
    void		outputDoneCB(CallBacker*);

};
