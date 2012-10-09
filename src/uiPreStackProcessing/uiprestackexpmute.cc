/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiprestackexpmute.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "file.h"
#include "mathfunc.h"
#include "oddirs.h"
#include "prestackmutedeftransl.h"
#include "prestackmutedef.h"
#include "strmprov.h"
#include "survinfo.h"

namespace PreStack
{

uiExportMute::uiExportMute( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export Mute Function",
	    			 "Specify output format", "103.2.6"))
    , ctio_(*mMkCtxtIOObj(MuteDef))
{
    setCtrlStyle( DoAndStay );

    infld_ = new uiIOObjSel( this, ctio_, "Input Mute Def" );

    coordfld_ = new uiGenInput( this, "Write coordinates as",
	   		        BoolInpSpec(true,"X/Y","Inl/Crl") );
    coordfld_->attach( alignedBelow, infld_ );

    outfld_ = new uiFileInput( this, "Output Ascii file",
	   		       uiFileInput::Setup().forread(false) );
    outfld_->attach( alignedBelow, coordfld_ );
}


uiExportMute::~uiExportMute()
{
    delete ctio_.ioobj; delete &ctio_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportMute::writeAscii()
{
    if ( !ctio_.ioobj ) mErrRet("Cannot find mute def in database");

    MuteDefTranslator* tr = (MuteDefTranslator*)ctio_.ioobj->getTranslator();
    if ( !tr ) return false;

    MuteDef mutedef;
    BufferString bs;
    const bool retval = tr->retrieve( mutedef, ctio_.ioobj, bs );
    if ( !retval ) mErrRet( bs.buf() );

    const BufferString fname = outfld_->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( "Cannot open output file" );
    }

    const bool isxy = coordfld_->getBoolValue();
    
    BufferString str;
    for ( int pos=0; pos<mutedef.size(); pos++ )
    {
	const BinID binid = mutedef.getPos( pos );
	const TypeSet<float>& offsetvals = mutedef.getFn( pos ).xVals();
	for ( int offsetidx=0; offsetidx<offsetvals.size(); offsetidx++ )
	{
	    if( !isxy )
		*sdo.ostrm << binid.inl << '\t' << binid.crl;
	    else
	    {
		const Coord coord = SI().transform( binid );
		// ostreams print doubles awfully
		str.setEmpty();
		str += coord.x; str += "\t"; str += coord.y;
		*sdo.ostrm << str;
	    }

	    *sdo.ostrm << '\t' << offsetvals[offsetidx] << '\t' << 
		       mutedef.getFn( pos ).getValue( offsetvals[offsetidx] );

	    *sdo.ostrm << '\n';
	}
    }
    
    sdo.close();
    return true;
}


bool uiExportMute::acceptOK( CallBacker* )
{
    if (!strcmp(outfld_->fileName(),"") )
	mErrRet( "Please select output file" );

    if ( File::exists(outfld_->fileName()) && 
	    		!uiMSG().askContinue("Output file exists. Continue?") )
	return false;

    if ( writeAscii() )
	uiMSG().message( "Export finished successfully" );

    return false;
}

} // namespace PreStack
