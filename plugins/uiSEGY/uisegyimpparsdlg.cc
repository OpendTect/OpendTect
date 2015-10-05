/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id:$";

#include "uisegyimpparsdlg.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "safefileio.h"
#include "file.h"
#include "repos.h"
#include "ascstream.h"
#include "od_iostream.h"
#include "manobjectset.h"


namespace Repos
{

class IOPar : public ::IOPar
{
public:

		IOPar( Source src )
		    : ::IOPar(""), src_(src)		{}

    Source	src_;

};


class IOParSet : public ManagedObjectSet<IOPar>
{
public:

			IOParSet(const char* basenm);

    int			find(const char*) const;
    ObjectSet<const IOPar> getEntries(Source) const;

    bool		write(Source) const;
    bool		write(const Source* s=0) const;

protected:

    const BufferString	basenm_;

};

} // namespace Repos


Repos::IOParSet::IOParSet( const char* basenm )
    : basenm_(basenm)
{
    FileProvider rfp( basenm_ );
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );

	SafeFileIO sfio( fnm );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	while ( sfio.istrm().isOK() )
	{
	    IOPar* par = new IOPar( rfp.source() );
	    par->getFrom( astrm );
	    if ( par->isEmpty() )
		{ delete par; continue; }

	    int paridx = find( par->name() );
	    if ( paridx < 0 )
		*this += par;
	    else
		replace( paridx, par );
	}
	sfio.closeSuccess();
    }
}


ObjectSet<const Repos::IOPar> Repos::IOParSet::getEntries(
					Repos::Source src ) const
{
    ObjectSet<const IOPar> ret;
    for ( int idx=0; idx<size(); idx++ )
    {
	const IOPar* entry = (*this)[idx];
	if ( entry->src_ == src )
	    { ret += entry; break; }
    }
    return ret;
}


int Repos::IOParSet::find( const char* nm ) const
{
    int ret = -1;
    for ( int idx=0; idx<size(); idx++ )
	if ( ((*this)[idx])->name() == nm )
	    { ret = idx; break; }
    return ret;
}


bool Repos::IOParSet::write( Repos::Source reqsrc ) const
{
    return write( &reqsrc );
}


bool Repos::IOParSet::write( const Repos::Source* reqsrc ) const
{
    bool rv = true;

    FileProvider rfp( basenm_ );
    while ( rfp.next() )
    {
	const Source cursrc = rfp.source();
	if ( reqsrc && *reqsrc != cursrc )
	    continue;

	ObjectSet<const IOPar> srcentries = getEntries( cursrc );
	if ( srcentries.isEmpty() )
	    continue;

	const BufferString fnm( rfp.fileName() );
	if ( File::exists(fnm) && !File::isWritable(fnm) )
	    { rv = false; continue; }

	SafeFileIO sfio( fnm );
	if ( !sfio.open(false) )
	    { rv = false; continue; }

	ascostream astrm( sfio.ostrm() );
	astrm.putHeader( basenm_ );
	for ( int idx=0; idx<srcentries.size(); idx++ )
	    srcentries[idx]->putTo( astrm );

	if ( sfio.ostrm().isOK() )
	    sfio.closeSuccess();
	else
	    { rv = false; sfio.closeFail(); }
    }

    return rv;
}


static const char* sNoSavedYet = "<No saved setups yet>";


uiSEGYImpParsDlg::uiSEGYImpParsDlg( uiParent* p, bool isread, const char* dfnm )
    : uiDialog(p,Setup(isread?tr("Read SEG-Y setup"):tr("Store SEG-Y setup"),
			mNoDlgTitle,mNoHelpKey))
    , parset_(*new Repos::IOParSet("SEGYSetup"))
    , parname_(dfnm)
{
    BufferStringSet nms;
    for ( int idx=0; idx<parset_.size(); idx++ )
	nms.add( parset_[idx]->name() );

    if ( nms.size() > 1 )
	nms.sort();
    else if ( nms.isEmpty() && isread )
	nms.add( sNoSavedYet );

    listfld_ = new uiListBox( this, "Stored Setups" );
    listfld_->addItems( nms );
}


uiSEGYImpParsDlg::~uiSEGYImpParsDlg()
{
    delete &parset_;
}


bool uiSEGYImpParsDlg::acceptOK( CallBacker* )
{
    return doIO();
}


uiSEGYReadImpParsDlg::uiSEGYReadImpParsDlg( uiParent* p, const char* defnm )
    : uiSEGYImpParsDlg(p,true,defnm)
{
    setHelpKey( mTODOHelpKey );
}


const IOPar* uiSEGYReadImpParsDlg::pars() const
{
    const int idx = parset_.find( parname_ );
    return idx < 0 ? 0 : parset_[idx];
}


#define mErrRet(s) { uiMSG().error( s ); return false; }


bool uiSEGYReadImpParsDlg::doIO()
{
    parname_ = listfld_->getText();
    if ( !pars() )
    {
	if ( parname_ != sNoSavedYet )
	    mErrRet( uiStrings::phrSelect(tr("SEG-Y Setup")) )
	else
	    return false;
    }

    mErrRet( tr("TODO: Not impl yet") );
}


uiSEGYStoreImpParsDlg::uiSEGYStoreImpParsDlg( uiParent* p, const IOPar& iop,
					      const char* defnm )
    : uiSEGYImpParsDlg(p,false,defnm)
{
    setHelpKey( mTODOHelpKey );

    namefld_ = new uiGenInput( this, tr("Store as"), StringInpSpec(defnm) );
    namefld_->attach( alignedBelow, listfld_ );
}


bool uiSEGYStoreImpParsDlg::doIO()
{
    mErrRet( tr("Sorry, not implemented yet") );
    return false;
}
