/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uiimpgprpi.cc,v 1.2 2010-03-15 14:29:45 cvsbert Exp $";

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uiseissel.h"
#include "uitaskrunner.h"
#include "executor.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "seisselectionimpl.h"
#include "strmprov.h"
#include "survinfo.h"
#include "datachar.h"
#include "plugins.h"

// GPR data needs to be scaled, in T, by 1000 to be able to use OpendTect
// effectively
#define mNanoFac 1e-6

static const char* menunm = "&GPR: DZT";


mExternC int GetuiImpGPRPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiImpGPRPluginInfo()
{
    static PluginInfo retpi = {
	"GPR: .DZT import",
	"Bert Bril/Matthias Schuh",
	"0.0.0",
	"Imports GPR data in DZT format.\n" };
    return &retpi;
}


// OK: we need an object to receive the CallBacks. In serious software,
// that may be a 'normal' object inheriting from CallBacker.

class uiImpGPRMgr :  public CallBacker
{
public:

			uiImpGPRMgr(uiODMain&);

    uiODMain&		appl_;
    void		updMnu(CallBacker*);
    void		doWork(CallBacker*);
};


uiImpGPRMgr::uiImpGPRMgr( uiODMain& a )
	: appl_(a)
{
    appl_.menuMgr().dTectMnuChanged.notify( mCB(this,uiImpGPRMgr,updMnu) );
    updMnu(0);
}

void uiImpGPRMgr::updMnu( CallBacker* )
{
    if ( SI().has2D() )
	appl_.menuMgr().getMnu( true, uiODApplMgr::Seis )->
		insertItem(new uiMenuItem(menunm,mCB(this,uiImpGPRMgr,doWork)));
}

#define mStrm (*sd_.istrm)
#define mErrRet(s) { msg_ = s; nrsamples_ = 0; return; }

#define mExtraHdrBytes 512
#define mHdrBytesPerChannel 1024

namespace DZT
{

struct FileHeader
{
    struct Date
    {
	unsigned sec2:5, min:6, hour:5, day:5, month:4, year:7;
    };

			FileHeader() : nsamp(0)	{}

    bool		getFrom(std::istream&,BufferString&);
    bool		isOK() const		{ return nsamp > 0; }

    int			traceNr( int trcidx ) const
			{ return 1 + trcidx * sps; }
    int			nrBytesPerSample() const
    			{ return bits / 8; }
    int			nrBytesPerTrace() const
			{ return nsamp * nrBytesPerSample(); }

    unsigned short tag, data, nsamp, bits;
    short zero;
    float sps, spm, mpm, position, range;
    Date created, modified;
    unsigned short npass, rgain, nrgain, text, ntext, proc, nproc, nchan;
    float epsr, top, depth;
    char dtype, antname[14];
    unsigned short chanmask;
    unsigned short chksum;

};

#define mRdVal(v) strm.read( (char*)(&v), sizeof(v) )
bool FileHeader::getFrom( std::istream& strm, BufferString& emsg )
{
    mRdVal(tag); mRdVal(data); mRdVal(nsamp); mRdVal(bits); mRdVal(zero);
    mRdVal(sps); mRdVal(spm); mRdVal(mpm); mRdVal(position); mRdVal(range);
    mRdVal(created); mRdVal(modified);
    mRdVal(npass); mRdVal(rgain); mRdVal(nrgain); mRdVal(text); mRdVal(ntext);
    mRdVal(proc); mRdVal(nproc); mRdVal(nchan);
    mRdVal(epsr); mRdVal(top); mRdVal(depth);
    char buf[32]; strm.read( buf, 32 ); dtype = buf[32];
    strm.read( antname, 14 );
    mRdVal(chanmask); strm.read( buf, 12 );
    mRdVal(chksum);

    emsg.setEmpty();
#define mRetFalse nsamp = 0; return false
    if ( nsamp < 1 )
	{ emsg = "Zero Nr of samples found."; mRetFalse; }
    if ( sps < 1 )
	{ emsg = "Zero scans per second found."; mRetFalse; }
    if ( range < 1 )
	{ emsg = "Zero trace length found."; mRetFalse; }
    if ( data < 128 )
	{ emsg.add("Invalid data offset found: ").add(data); mRetFalse; }

    // dtype cannot be trusted, it seems
    if ( bits < 32 )
    {
	if ( dtype %2 )
	    dtype = bits == 16 ? 3 : 1;
	else
	    dtype = bits == 16 ? 2 : 0;
    }

    strm.seekg( std::streampos(data), std::ios::beg );
    if ( !strm.good() )
	{ emsg = "Cannot read first trace."; mRetFalse; }

    return true;
}


class Importer : public ::Executor
{
public:

Importer( const char* fnm, const IOObj& ioobj, const LineKey& lk )
    : Executor("Importing DTZ file")
    , nrdone_(0)
    , totalnr_(-1)
    , lk_(lk)
    , wrr_(0)
    , databuf_(0)
    , di_(DataCharacteristics())
{
    sd_ = StreamProvider( fnm ).makeIStream();
    if ( !sd_.usable() )
	return;

    if ( !fh_.getFrom( mStrm, msg_ ) )
	return;

    DataCharacteristics dc( fh_.dtype < 9, fh_.dtype %2 );
    dc.setNrBytes( fh_.dtype > 1 ? (fh_.dtype > 3 ? 4 : 2) : 1 );
    di_.set( dc );

    trc_.data().reSize( fh_.nsamp );
    for ( int ichan=1; ichan<fh_.nchan; ichan++ )
	trc_.data().addComponent( fh_.nsamp, DataCharacteristics() );

    wrr_ = new SeisTrcWriter( &ioobj );
    Seis::RangeSelData* rsd = new Seis::RangeSelData;
    rsd->setIsAll( true );
    rsd->lineKey() = lk;
    wrr_ = new SeisTrcWriter( &ioobj );
    wrr_->setSelData( rsd );

    databuf_ = new char [ fh_.nrBytesPerTrace() ];
    msg_ = "Handling traces";
}

const char* message() const	{ return msg_; }
const char* nrDoneText() const	{ return "Traces handled"; }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totalnr_; }

~Importer()
{
    sd_.close();
    delete wrr_;
    delete [] databuf_;
}

#undef mErrRet
#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }

int nextStep()
{
    if ( !fh_.isOK() )
    {
	if ( !sd_.usable() )
	    mErrRet("Cannot open input file")
	else
	    return ErrorOccurred();
    }

    const int trcbytes = fh_.nrBytesPerTrace();
    mStrm.read( databuf_, trcbytes );
    if ( mStrm.gcount() != trcbytes )
	return Finished();

    trc_.info().nr = fh_.traceNr(nrdone_);
    trc_.info().coord.x = trc_.info().nr;
    trc_.info().coord.y = 0;

    for ( int ichan=0; ichan<fh_.nchan; ichan++ )
    {
	for ( int isamp=0; isamp<fh_.nsamp; isamp++ )
	    trc_.set( isamp, di_.get(databuf_,isamp)+fh_.zero, ichan );
	float firstrealval = trc_.get( 2, ichan );
	trc_.set( 0, firstrealval, ichan );
	trc_.set( 1, firstrealval, ichan );
    }

    if ( !wrr_->put(trc_) )
	mErrRet(wrr_->errMsg())

    nrdone_++;
    return mStrm.good() ? MoreToDo() :  Finished();
}

    SeisTrc		trc_;
    SeisTrcWriter*	wrr_;
    StreamData		sd_;
    LineKey		lk_;

    DZT::FileHeader	fh_;
    char*		databuf_;
    DataInterpreter<float> di_;

    od_int64		nrdone_;
    od_int64		totalnr_;
    BufferString	msg_;

};

}


#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return false; }

class uiGPRImporter : public uiDialog
{
public:

uiGPRImporter( uiParent* p )
    : uiDialog(p,Setup("Import GPR Seismics","Import DTZ Seismics",mNoHelpID))
    , inpfld_(0)
{
    if ( !SI().has2D() )
	{ new uiLabel(this,"TODO: implement 3D loading"); }

    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter( "*.dzt" ).forread( true );
    inpfld_ = new uiFileInput( this, "Input DTZ file", fisu );

    lnmfld_ = new uiGenInput( this, "Output Line name" );
    lnmfld_->attach( alignedBelow, inpfld_ );

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    uiSeisSel::fillContext( Seis::Line, false, ctxt );
    outfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(Seis::Line) );
    outfld_->setAttrNm( "GPR" );
    outfld_->attach( alignedBelow, lnmfld_ );
}

bool acceptOK( CallBacker* )
{
    if ( !inpfld_ ) return true;

    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() ) mErrRet("Please enter the input file name")
    const BufferString lnm( lnmfld_->text() );
    if ( lnm.isEmpty() ) mErrRet("Please enter the output line name")

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj ) return false;

    uiTaskRunner tr( this );
    DZT::Importer importer( fnm, *ioobj, LineKey(lnm,outfld_->attrNm()) );
    return tr.execute( importer );
}

    uiFileInput*	inpfld_;
    uiGenInput*		lnmfld_;
    uiSeisSel*		outfld_;

};


void uiImpGPRMgr::doWork( CallBacker* )
{
    uiGPRImporter dlg( &appl_ );
    dlg.go();
}


mExternC const char* InituiImpGPRPlugin( int, char** )
{
    (void)new uiImpGPRMgr( *ODMainWin() );
    return 0; // All OK - no error messages
}
