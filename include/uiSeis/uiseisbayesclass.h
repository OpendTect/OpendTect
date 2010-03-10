#ifndef uiseisbayesclass_h
#define uiseisbayesclass_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uiseisbayesclass.h,v 1.5 2010-03-10 16:19:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "odusgclient.h"
#include "uivarwizard.h"
class uiSeisBayesPDFInp;
class uiSeisBayesWeights;
class uiSeisBayesSeisInp;
class uiSeisBayesOut;


/*!\brief 'Server' for Seismic Bayesian Inversion. */

mClass uiSeisBayesClass : public uiVarWizard
			, public Usage::Client
{
public:

			uiSeisBayesClass(uiParent*,bool is2d);
			~uiSeisBayesClass();

protected:

    bool		is2d_;

    uiSeisBayesPDFInp*	inppdfdlg_;
    uiSeisBayesWeights*	wghtsdlg_;
    uiSeisBayesSeisInp*	inpseisdlg_;
    uiSeisBayesOut*	outdlg_;

    virtual void	doPart();
    virtual void	closeDown();

    void		getInpPDFs();
    void		inpPDFsGot(CallBacker*);

    void		getWeights();
    void		weightsGot(CallBacker*);

    void		getInpSeis();
    void		inpSeisGot(CallBacker*);

    void		doOutput();
    void		outputDone(CallBacker*);

};


#endif
