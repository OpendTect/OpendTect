/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2017
________________________________________________________________________

-*/

#include "uifileseltool.h"
#include "q_uiimpl.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "filesystemaccess.h"
#include "oddirs.h"
#include "separstr.h"

#include "uiparentbody.h"
#include "uidialog.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uistrings.h"

mImplClassFactory( uiFileSelToolProvider, factory );

static const char* filesep_ = ";";


uiFileSelectorSetup::uiFileSelectorSetup( const char* fnm )
    : selmode_(OD::SelectFileForRead)
{
    init( fnm );
}


uiFileSelectorSetup::uiFileSelectorSetup( SelectionMode mode,
					    const char* fnm )
    : selmode_(mode)
{
    init( fnm );
}


void uiFileSelectorSetup::init( const char* fnm )
{
    contenttype_ = OD::GeneralContent;
    if ( fnm && *fnm )
	initialselection_.add( fnm );
    initialselectiondir_.set( GetDataDir() );
    allowallextensions_ = confirmoverwrite_ = true;
    onlylocal_ = false;
}


uiFileSelTool::uiFileSelTool( uiParent* p, const Setup& su )
    : parent_(p)
    , setup_(su)
{
}


BufferString uiFileSelTool::fileName() const
{
    BufferStringSet selected;
    gtFileNames( selected );
    return selected.isEmpty() ? BufferString::empty() : selected.get(0);
}


BufferString uiFileSelTool::joinSelection( const BufferStringSet& bss )
{
    SeparString sepstr( 0, filesep_[0] );
    for ( int idx=0; idx<bss.size(); idx++ )
	sepstr.add( bss.get(idx) );
    return BufferString( sepstr.str() );
}


void uiFileSelTool::separateSelection( const char* inp, BufferStringSet& bss )
{
    bss.setEmpty();
    const SeparString sepstr( inp, filesep_[0] );
    const int sz = sepstr.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	BufferString fname( sepstr[idx] );
	fname.trimBlanks();
#ifdef __win__
	if ( fname.size() == 2 )
	    fname += "\\";
#endif
	fname = File::Path::mkCleanPath( fname, File::Path::Local );
	if ( !fname.isEmpty() )
	    bss.add( fname );
    }
}


uiFileSelector::uiFileSelector( uiParent* p, const Setup& su )
    : parent_(p)
    , setup_(su)
    , seltool_(0)
{
}


uiFileSelector::uiFileSelector( uiParent* p, const char* fnm, bool loc )
    : parent_(p)
    , setup_(fnm)
    , seltool_(0)
{
    setup_.onlylocal_ = loc;
}


uiFileSelector::~uiFileSelector()
{
    delete seltool_;
}


BufferString uiFileSelector::fileName() const
{
    return seltool_ ? seltool_->fileName() : BufferString::empty();
}


void uiFileSelector::getSelected( BufferStringSet& fnms ) const
{
    if ( !seltool_ )
	fnms.setEmpty();
    else
	seltool_->getSelected( fnms );
}


bool uiFileSelector::go()
{
    if ( caption_.isEmpty() )
    {
	if ( isForRead(setup_.selmode_) )
	    caption_ = uiStrings::phrSelect( isDirectory(setup_.selmode_)
			? uiStrings::sDirectory() : uiStrings::sFile() );
	else
	    caption_ = tr("Specify new file name");
    }

    BufferString prot;
    if ( !setup_.onlylocal_ )
    {
	//TODO determine protocol, probably by using an in-between dialog.
	// The initial selection is a default but need poss to switch protocol
    }

    const uiFileSelToolProvider& fstp = uiFileSelToolProvider::get( prot );
    seltool_ = fstp.getSelTool( parent_, setup_ );
    if ( !seltool_ )
	{ pErrMsg("No sel tool"); return false; }

    seltool_->caption() = caption_;
    return seltool_->go();
}


//------- Local file selection --------


class uiLocalFileSelTool : public uiFileSelTool
{ mODTextTranslationClass(uiLocalFileSelector);
public:

			uiLocalFileSelTool(uiParent*,const Setup&);
    virtual		~uiLocalFileSelTool()			{}

protected:

    BufferStringSet	filenames_;

    virtual bool	doSelection();
    virtual void	gtFileNames( BufferStringSet& bss ) const
			{ bss = filenames_; }

};


class uiLocalFileSelToolProvider : public uiFileSelToolProvider
{
public:

static const char* factKeyw()
	{ return File::SystemAccess::getByProtocol(0).protocol(); }
static uiString factUsrName()
	{ return File::SystemAccess::getByProtocol(0).userName(); }

virtual const char* protocol() const { return factKeyw(); }
virtual uiString userName() const { return factUsrName(); }

virtual uiFileSelTool* getSelTool( uiParent* p, const Setup& su ) const
	{ return new uiLocalFileSelTool( p, su ); }

static uiFileSelToolProvider* createInstance()
	{ return new uiLocalFileSelToolProvider; }
static void initClass()
	{ factory().addCreator( createInstance, factKeyw(), factUsrName() ); }

};

void Init_uiLocalFileSelToolProvider_Class()
{
    uiLocalFileSelToolProvider::initClass();
}


static ObjectSet<uiFileSelToolProvider> fileseltoolprovs_;

const uiFileSelToolProvider& uiFileSelToolProvider::get( const char* prot )
{
    for ( int idx=0; idx<fileseltoolprovs_.size(); idx++ )
    {
	if ( FixedString(fileseltoolprovs_[idx]->protocol()) == prot )
	    return *fileseltoolprovs_[idx];
    }

    if ( !prot || !*prot )
	prot = "file";

    uiFileSelToolProvider* res = factory().create( prot );
    if ( !res )
    {
	ErrMsg( BufferString("No selector for access protocol:\n",prot) );
	if ( !fileseltoolprovs_.isEmpty() )
	    return *fileseltoolprovs_[0];
	res = new uiLocalFileSelToolProvider;
    }

    fileseltoolprovs_ += res;
    return *res;
}


uiString uiFileSelToolProvider::userName() const
{
    const BufferString prot = protocol();
    const File::SystemAccess& fsa = File::SystemAccess::getByProtocol( prot );
    return fsa.userName();
}


#include <QFileDialog>
#include <QPushButton>

mUseQtnamespace


class ODQFileDialog : public QFileDialog
{
public:

ODQFileDialog( const QString& dirname, const QString& fltr,
	       QWidget* p, const char* caption )
    : QFileDialog(p,caption,dirname,fltr)
{
    setModal( true );
}

};


static QFileDialog::FileMode qmodeForUiMode( OD::FileSelectionMode mode )
{
    switch( mode )
    {
    case OD::SelectFileForRead		: return QFileDialog::ExistingFile;
    case OD::SelectDirectory		: return QFileDialog::Directory;
    case OD::SelectMultiFile		: return QFileDialog::ExistingFiles;
    default				: return QFileDialog::AnyFile;
    }
}


uiLocalFileSelTool::uiLocalFileSelTool( uiParent* p, const Setup& su )
    : uiFileSelTool(p,su)
{
}


bool uiLocalFileSelTool::doSelection()
{
    filenames_.setEmpty();
    BufferString firstfnm;
    if ( !setup_.initialselection_.isEmpty() )
	firstfnm.set( setup_.initialselection_.get(0) );
    File::Path fpfirst( firstfnm );
    firstfnm = fpfirst.fullPath();
    BufferString dirname;
    if ( File::isDirectory(firstfnm) )
    {
	dirname = firstfnm;
	firstfnm.setEmpty();
    }
    else
    {
	if ( !File::isDirectory(fpfirst.pathOnly()) )
	{
	    dirname = GetPersonalDir();
	    firstfnm.setEmpty();
	}
	else if ( !File::exists(firstfnm)
		    && isFile(setup_.selmode_) && isForRead(setup_.selmode_) )
	{
	    dirname = fpfirst.pathOnly();
	    firstfnm.setEmpty();
	}
	else
	{
	    dirname = fpfirst.pathOnly();
	    firstfnm = fpfirst.fileName();
	}
    }

    if ( GetEnvVarYN("OD_FILE_SELECTOR_BROKEN") )
    {
	uiDialog dlg( parent_, uiDialog::Setup(tr("Specify file name"),
			    tr("System file selection unavailable!"),
                            mNoHelpKey) );
	uiLineEdit* le = new uiLineEdit( &dlg, "File name" );
	le->setText( dirname );
	new uiLabel( &dlg, tr("File name"), le );
	if ( !dlg.go() )
	    return false;
	filenames_.add( le->text() );
	return true;
    }

    QWidget* qparent = 0;
    if ( parent_ && parent_->pbody() )
	qparent = parent_->pbody()->managewidg();

    File::FormatList fmts( setup_.formats_ );
    if ( setup_.allowallextensions_ )
	fmts.addFormat( File::Format::allFiles() );

    BufferString addendum;
    const uiString wintitle =
	uiMainWin::uniqueWinTitle( caption_, 0, &addendum );
    const BufferString utfwintitle( toString(caption_), addendum );
    PtrMan<ODQFileDialog> fd = new ODQFileDialog( QString(dirname),
		    QString(fmts.getFileFilters()), qparent, "File dialog" );
    if ( !firstfnm.isEmpty() )
    {
	fd->selectFile( QString(firstfnm) );
	for ( int idx=1; idx<setup_.initialselection_.size(); idx++ )
	{
	    const File::Path fp( setup_.initialselection_.get(idx) );
	    fd->selectFile( QString(fp.fileName().buf()) );
	}
    }
    fd->setAcceptMode( isForRead(setup_.selmode_) ? QFileDialog::AcceptOpen
						 : QFileDialog::AcceptSave );

    const QFileDialog::FileMode qfmode = qmodeForUiMode( setup_.selmode_ );
    fd->setFileMode( qfmode );
    if ( qfmode == QFileDialog::Directory )
	fd->setOption( QFileDialog::ShowDirsOnly );

    fd->setWindowTitle( toQString(wintitle) );
    fd->setOption(QFileDialog::DontConfirmOverwrite,!setup_.confirmoverwrite_);
    if ( !setup_.initialselectiondir_.isEmpty() )
	fd->setDirectory( QString(setup_.initialselectiondir_.buf()) );
    if ( !setup_.defaultextension_.isEmpty() )
    {
	fd->selectNameFilter( QString(setup_.defaultextension_.buf()) );
	fd->setDefaultSuffix( QString(setup_.defaultextension_.buf()) );
    }

#ifdef __win__
    fd->setViewMode( QFileDialog::Detail );
#endif

    QList<QPushButton*> qpblst = fd->findChildren<QPushButton*>("");
    foreach(QPushButton* qpb,qpblst)
    {
	if ( qpb->text() == toQString(uiStrings::sSave()) ||
	     qpb->text() == toQString(uiStrings::sOpen()) ||
	     qpb->text() == "Choose" )
	     qpb->setText( "OK" );
    }

    if ( fd->exec() != QDialog::Accepted )
	return false;

    const QStringList selfiles = fd->selectedFiles();
    for ( int idx=0; idx<selfiles.size(); idx++ )
    {
	BufferString bs( selfiles[idx] );
#ifdef __win__
	bs.replace( '/', '\\' );
#endif
	filenames_.add( bs );
    }

    return true;
}
