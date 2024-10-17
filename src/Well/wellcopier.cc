/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellcopier.h"

#include "ioman.h"
#include "welldata.h"
#include "wellreader.h"
#include "welltransl.h"
#include "wellwriter.h"

Well::Copier::Copier( const MultiID& inpid, const char* outwellnm )
    : inpid_( inpid )
    , outwellname_( outwellnm )
{}


Well::Copier::~Copier()
{}


void Well::Copier::setOverwriteAllowed( bool yn )
{
    allowoverwrite_ = yn;
}


PtrMan<IOObj> Well::Copier::getOutputIOObj()
{
    if ( outwellname_.isEmpty() )
    {
	errmsg_ = tr("Output well name not specified");
	return nullptr;
    }

    PtrMan<IOObj> ioobj = IOM().getLocal( outwellname_,
				   WellTranslatorGroup::sGroupName() );
    if ( ioobj )
    {
	if ( allowoverwrite_ )
	    ioobj->implRemove();
	else
	{
	    errmsg_ = tr("%1 alread exists in the database."
			 " Please chose another name" ).arg( outwellname_ );
	    ioobj.erase();
	    return nullptr;
	}
    }

    if ( !ioobj )
    {
	CtxtIOObj ctio( mIOObjContext(Well) );
	ctio.setName( outwellname_ );
	ctio.fillObj();
	ioobj = ctio.ioobj_->clone();
    }

    return ioobj;
}


bool Well::Copier::doCopy()
{
    ConstPtrMan<IOObj> inioobj = IOM().get( inpid_ );
    if ( !inioobj )
    {
	errmsg_ = tr("Could not find input well in the database");
	return false;
    }

    ConstPtrMan<IOObj> outioobj( getOutputIOObj().release() );
    if ( !outioobj )
	return false;

    RefMan<Well::Data> wdin = new Well::Data;
    Well::Reader rdr( inioobj->key(), *wdin );
    if ( !rdr.get() )
    {
	errmsg_ = rdr.errMsg();
	return false;
    }

    const Well::Writer wrr( *outioobj, *wdin );
    if ( !wrr.put() )
    {
	errmsg_ = wrr.errMsg();
	return false;
    }

    outputid_ = outioobj->key();
    IOM().implUpdated.trigger( outputid_ );
    return true;
}


// --- MultiWellCopier ---
MultiWellCopier::MultiWellCopier(
		    const ObjectSet<std::pair<const MultiID&,
					      const BufferString>>& copyset )
    : Executor("Copying wells")
    , copyset_(copyset)
    , nrwells_(copyset.size())
{}


MultiWellCopier::~MultiWellCopier()
{}


void MultiWellCopier::setOverwriteAllowed( bool yn )
{
    allowoverwrite_ = yn;
}


od_int64 MultiWellCopier::totalNr() const
{
    return nrwells_;
}


od_int64 MultiWellCopier::nrDone() const
{
    return nrdone_;
}


uiString MultiWellCopier::uiMessage() const
{
    return errmsg_;
}


uiString MultiWellCopier::uiNrDoneText() const
{
    return tr( "Wells copied" );
}


int MultiWellCopier::nextStep()
{
    if ( copyset_.isEmpty() )
    {
	errmsg_ = tr("No wells to be copied");
	return Finished();
    }

    if ( nrdone_ >= totalNr() )
	return Finished();

    const BufferString& outwellnm = copyset_.get(nrdone_)->second;
    const MultiID& inpid = copyset_.get(nrdone_)->first;
    nrdone_++;
    if ( !inputIsOK(inpid, outwellnm) )
	return MoreToDo();

    Well::Copier copier( inpid, outwellnm );
    copier.setOverwriteAllowed( allowoverwrite_ );
    if ( !copier.doCopy() )
    {
	const StringView inpwellnm
			    = Well::MGR().get(inpid,Well::Inf)->name().str();
	if ( !inpwellnm.isEmpty() )
	    errmsg_.append( ::toUiString(inpwellnm) ).append( tr(": ") );

	errmsg_.append( copier.errMsg() ).addNewLine();
	return MoreToDo();
    }

    outputids_ += copier.copiedWellID();
    return MoreToDo();
}


bool MultiWellCopier::inputIsOK( const MultiID& inpid,
				 const char* outwellnm ) const
{
    ConstRefMan<Well::Data> inpwd = Well::MGR().get( inpid, Well::Inf );
    if ( !inpwd )
    {
	errmsg_.append( Well::MGR().errMsg() ).addNewLine();
	return false;
    }

    const StringView inpwellnm = inpwd->name().buf();
    if ( StringView(outwellnm).isEmpty() )
    {
	errmsg_.append( ::toUiString(inpwellnm) ).append( tr(": ") );
	errmsg_.append( tr("Output file name not specified") );
	return false;
    }

    if ( inpwellnm == outwellnm && !allowoverwrite_ )
    {
	errmsg_.append( ::toUiString(inpwellnm) ).append( tr(": ") );
	errmsg_.append( tr("Output file name cannot be the same as input file"
			   " name") );
	return false;
    }

    return true;
}
