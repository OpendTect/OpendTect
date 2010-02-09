#ifndef uiseisbayesclass_h
#define uiseisbayesclass_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uiseisbayesclass.h,v 1.1 2010-02-09 16:01:56 cvsbert Exp $
________________________________________________________________________

-*/

#include "odusgclient.h"
#include "iopar.h"
class uiParent;
class uiSeisBayesPDFInp;
class uiSeisBayesSeisInp;
class uiSeisBayesOut;


/*!\brief 'Server' for Seismic Bayesian Inversion. */

mClass uiSeisBayesClass : public CallBacker
			, public Usage::Client
{
public:

    enum State		{ Cancelled, Finished, Wait4Dialog,
			  InpPDFS, InpSeis, Output };

			uiSeisBayesClass(uiParent*,bool is2d,
					 const IOPar* iop=0);
			~uiSeisBayesClass();

    IOPar&		pars()			{ return pars_; }
    const IOPar&	pars() const		{ return pars_; }

    bool		go();

    Notifier<uiSeisBayesClass> processEnded;
    State		state() const		{ return state_; }

    static const char*	sKeyPDFID();

protected:

    bool		is2d_;
    State		state_;
    uiParent*		parent_;
    IOPar		pars_;

    uiSeisBayesPDFInp*	inppdfdlg_;
    uiSeisBayesSeisInp*	inpseisdlg_;
    uiSeisBayesOut*	outdlg_;

    void		nextAction();
    void		closeDown();

    void		getInpPDFs();
    void		inpPDFsGot(CallBacker*);

    void		getInpSeis();
    void		inpSeisGot(CallBacker*);

    void		doOutput();
    void		outputDone(CallBacker*);

};


#endif
