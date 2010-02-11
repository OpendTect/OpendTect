/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/

static const char* rcsID = "$Id: seisbayesclass.cc,v 1.1 2010-02-11 16:10:44 cvsbert Exp $";

#include "seisbayesclass.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "probdenfunc.h"
#include "keystrs.h"
#include "iopar.h"

const char* SeisBayesClass::sKeyPDFID()		{ return "PDF.ID"; }
const char* SeisBayesClass::sKeySeisInpID()	{ return "Seismics.Input.ID"; }
const char* SeisBayesClass::sKeySeisOutID()	{ return "Seismics.Output.ID"; }


SeisBayesClass::SeisBayesClass( const IOPar& iop )
    	: Executor( "Bayesian inversion" )
	, donorm_(false)
	, nrdone_(0)
	, totalnr_(-2)
	, msg_("Processing")
{
    const char* res = iop.find( sKey::Type );
    is2d_ = res && *res == '2';
}


SeisBayesClass::~SeisBayesClass()
{
    deepErase(pdfs_);
    deepErase(rdrs_);
    deepErase(wrrs_);
}


const char* SeisBayesClass::message() const
{
    return msg_;
}


const char* SeisBayesClass::nrDoneText() const
{
    return "Positions handled";
}


od_int64 SeisBayesClass::nrDone() const
{
    return nrdone_;
}


od_int64 SeisBayesClass::totalNr() const
{
    if ( totalnr_ == -2 )
    {
	SeisBayesClass& self = *(const_cast<SeisBayesClass*>(this));
	self.totalnr_ = -1;
    }
    return totalnr_;
}


#include "timefun.h"

int SeisBayesClass::nextStep()
{
    pErrMsg( "TODO: implement" );
    Time_sleep( 5 );
    return Executor::Finished();
}
