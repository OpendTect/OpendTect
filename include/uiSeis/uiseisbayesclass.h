#ifndef uiseisbayesclass_h
#define uiseisbayesclass_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uiseisbayesclass.h,v 1.6 2010/03/15 16:12:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "odusgclient.h"
#include "uivarwizard.h"
class uiSeisBayesPDFInp;
class uiSeisBayesNorm;
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
    uiSeisBayesNorm*	normdlg_;
    uiSeisBayesSeisInp*	inpseisdlg_;
    uiSeisBayesOut*	outdlg_;

    virtual void	doPart();
    virtual void	closeDown();

    void		getInpPDFs();
    void		inpPDFsGot(CallBacker*);

    void		getNorm();
    void		normGot(CallBacker*);

    void		getInpSeis();
    void		inpSeisGot(CallBacker*);

    void		doOutput();
    void		outputDone(CallBacker*);

};


#endif
