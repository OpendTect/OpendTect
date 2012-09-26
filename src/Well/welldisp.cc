/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "welldisp.h"
#include "settings.h"
#include "keystrs.h"


static const char* sKeyTrackNmIsAbove = "Track Name Above";
static const char* sKeyTrackNmIsBelow = "Track Name Below";
static const char* sKeyTrackNmSize = "Track Name Size";
static const char* sKeyTrackNmFont = "Track Font";
static const char* sKeyMarkerShape = "Marker Shape";
static const char* sKeyMarkerCylinderHeight = "Cylinder Height";
static const char* sKeyMarkerNmSize = "Marker Name Size";
static const char* sKeyMarkerNmFont = "Marker Name Font";
static const char* sKeyMarkerNmColor = "Marker Name Color";
static const char* sKeyMarkerNmSameColor = "Marker Name Color Same as Marker";
static const char* sKeyMarkerSingleColor = "Single Marker Color";
static const char* sKeyMarkerSelected = "Selected Markers";
static const char* sKeyLeftColor = "Left Log Color";
static const char* sKeyLeftSize = "Left Log Size";
static const char* sKeyLeftStyle = "Left Log Style";
static const char* sKeyLeftLeftFill = "Left Fill Left Log";
static const char* sKeyLeftRightFill = "Right Fill Left Log";
static const char* sKeyLeftSingleCol = "Left Single Fill Color";
static const char* sKeyLeftDataRange = "Left Data Range Bool";
static const char* sKeyLeftOverlapp = "Left Log Overlapp";
static const char* sKeyLeftRepeatLog = "Left Log Number";
static const char* sKeyLeftSeisColor = "Left Log Seismic Style Color";
static const char* sKeyLeftName = "Left Log Name";
static const char* sKeyLeftFillName = "Left Filled Log name";
static const char* sKeyLeftCliprate = "Left Cliprate";
static const char* sKeyLeftFillRange = "Left Filled Log Range";
static const char* sKeyLeftRange = "Left Log Range";
static const char* sKeyLeftRevertRange = "Left Revert Range Bool";
static const char* sKeyLeftSeqname = "Left Sequence name";
static const char* sKeyLeftLogWidth = "Left Log Width";
static const char* sKeyLeftColTabFlipped = "Left Log Color Table Flipped";
static const char* sKeyLeftScale = "Left Log scale";
static const char* sKeyRightColor = "Right Log Color";
static const char* sKeyRightSize = "Right Log Size";
static const char* sKeyRightStyle = "Right Log Style";
static const char* sKeyRightLeftFill = "Left Fill Right Log";
static const char* sKeyRightRightFill = "Right Fill Right Log";
static const char* sKeyRightSingleCol = "Right Single Fill Color";
static const char* sKeyRightDataRange = "Right Data Range Bool";
static const char* sKeyRightOverlapp = "Right Log Overlapp";
static const char* sKeyRightRepeatLog = "Right Log Number";
static const char* sKeyRightSeisColor = "Right Log Seismic Style Color";
static const char* sKeyRightName = "Right Log Name";
static const char* sKeyRightFillName = "Right Filled Log name";
static const char* sKeyRightCliprate = "Right Cliprate";
static const char* sKeyRightFillRange = "Right Filled Log Range";
static const char* sKeyRightRange = "Right Log Range";
static const char* sKeyRightRevertRange = "Right Revert Range Bool";
static const char* sKeyRightSeqname = "Right Sequence name";
static const char* sKeyRightLogWidth = "Right Log Width";
static const char* sKeyRightScale = "Right Log scale";
static const char* sKeyRightColTabFlipped = "Right Log Color Table Flipped";
static const char* sKey2DDisplayStrat = "Display Stratigraphy";


Well::DisplayProperties::DisplayProperties( const char* subjname )
    : subjectname_(subjname)
    , displaystrat_(false)  
{
    logs_ += new LogCouple();

    logs_[0]->left_.isrightfill_ = true;
    logs_[0]->right_.isleftfill_ = true;

    Settings& setts = Settings::fetch( "welldisp" );
    markers_.selmarkernms_.erase();
    usePar( setts );
}


Well::DisplayProperties::~DisplayProperties()
{
    deepErase( logs_ );
}


void Well::DisplayProperties::BasicProps::usePar( const IOPar& iop )
{
    iop.get( IOPar::compKey(subjectName(),sKey::Color()), color_ );
    iop.get( IOPar::compKey(subjectName(),sKey::Size()), size_ );
    doUsePar( iop );
}


void Well::DisplayProperties::BasicProps::useLeftPar( const IOPar& iop )
{
    iop.get( IOPar::compKey(subjectName(),sKey::Color()), color_ );
    iop.get( IOPar::compKey(subjectName(),sKey::Size()), size_ );
    doUseLeftPar( iop ); 
}


void Well::DisplayProperties::BasicProps::useRightPar( const IOPar& iop )
{
    iop.get( IOPar::compKey(subjectName(),sKey::Color()), color_ );
    iop.get( IOPar::compKey(subjectName(),sKey::Size()), size_ );
    doUseRightPar( iop ); 
}


void Well::DisplayProperties::BasicProps::fillPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey(subjectName(),sKey::Color()), color_ );
    iop.set( IOPar::compKey(subjectName(),sKey::Size()), size_ );
    doFillPar( iop );
}

void Well::DisplayProperties::BasicProps::fillLeftPar( IOPar& iop ) const
{
    doFillLeftPar( iop );
}


void Well::DisplayProperties::BasicProps::fillRightPar( IOPar& iop ) const
{
    doFillRightPar( iop );
}


void Well::DisplayProperties::Track::doUsePar( const IOPar& par )
{
    par.getYN( IOPar::compKey(subjectName(),sKeyTrackNmIsAbove), dispabove_ );
    par.getYN( IOPar::compKey(subjectName(),sKeyTrackNmIsBelow), dispbelow_ );

    const FixedString fontdata =
	par.find( IOPar::compKey(subjectName(),sKeyTrackNmFont ) );
    if ( fontdata )
	font_.getFrom( fontdata );
    else
    {
	int sz = 0;
	par.get( IOPar::compKey(subjectName(),sKeyTrackNmSize), sz );
	font_.setPointSize( sz );
    }
}


void Well::DisplayProperties::Track::doFillPar( IOPar& par ) const
{
    par.setYN( IOPar::compKey(subjectName(),sKeyTrackNmIsAbove), dispabove_ );
    par.setYN( IOPar::compKey(subjectName(),sKeyTrackNmIsBelow), dispbelow_ );
    BufferString fontdata;
    font_.putTo( fontdata );
    par.set( IOPar::compKey(subjectName(),sKeyTrackNmFont), fontdata );
}


void Well::DisplayProperties::Markers::doUsePar( const IOPar& par )
{
    par.getYN(IOPar::compKey(subjectName(),sKeyMarkerSingleColor),issinglecol_);
    par.get( IOPar::compKey(subjectName(),sKeyMarkerShape), shapeint_ );
    par.get( IOPar::compKey(subjectName(),sKeyMarkerCylinderHeight),
	     cylinderheight_ );
    par.getYN( IOPar::compKey(subjectName(),sKeyMarkerNmSameColor), samenmcol_);
    par.get( IOPar::compKey(subjectName(),sKeyMarkerNmColor), nmcol_ );
    par.get( IOPar::compKey(subjectName(),sKeyMarkerSelected), selmarkernms_ );

    const FixedString fontdata =
	par.find( IOPar::compKey(subjectName(),sKeyMarkerNmFont ) );
    if ( fontdata )
	font_.getFrom( fontdata );
    else
    {
	int sz = 0;
	par.get( IOPar::compKey(subjectName(),sKeyMarkerNmSize), sz );
	font_.setPointSize( sz );
    }
}


void Well::DisplayProperties::Markers::doFillPar( IOPar& par ) const
{
    par.setYN(IOPar::compKey(subjectName(),sKeyMarkerSingleColor),issinglecol_);
    par.set( IOPar::compKey(subjectName(),sKeyMarkerShape), shapeint_ );
    par.set( IOPar::compKey(subjectName(),sKeyMarkerCylinderHeight), cylinderheight_ );
    BufferString fontdata;
    font_.putTo( fontdata );
    par.set( IOPar::compKey(subjectName(),sKeyMarkerNmFont), fontdata );
    par.setYN( IOPar::compKey(subjectName(),sKeyMarkerNmSameColor), samenmcol_);
    par.set( IOPar::compKey(subjectName(),sKeyMarkerNmColor), nmcol_ );
    par.set( IOPar::compKey(subjectName(),sKeyMarkerSelected), selmarkernms_ );
}


void Well::DisplayProperties::Log::doUseLeftPar( const IOPar& iop )
{
    iop.get( IOPar::compKey(subjectName(),sKeyLeftColor), color_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftSize), size_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftName), name_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftRange), range_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftFillName), fillname_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftFillRange), fillrange_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftLeftFill), isleftfill_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftRightFill), isrightfill_ );
    iop.getYN(IOPar::compKey(subjectName(),sKeyLeftRevertRange),islogreverted_);
    iop.get( IOPar::compKey(subjectName(),sKeyLeftCliprate), cliprate_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftStyle), iswelllog_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftSingleCol), issinglecol_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftDataRange), isdatarange_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftRepeatLog), repeat_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftOverlapp), repeatovlap_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftSeisColor), seiscolor_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftSeqname), seqname_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftLogWidth), logwidth_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftScale), islogarithmic_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftColTabFlipped), iscoltabflipped_ );
}


void Well::DisplayProperties::Log::doUseRightPar( const IOPar& iop )
{
    iop.get( IOPar::compKey(subjectName(),sKeyRightColor), color_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightSize), size_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightName), name_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightRange), range_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightFillName), fillname_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightFillRange), fillrange_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightLeftFill), isleftfill_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightRightFill), isrightfill_ );
    iop.getYN(IOPar::compKey(subjectName(),sKeyRightRevertRange),islogreverted_);
    iop.get( IOPar::compKey(subjectName(),sKeyRightCliprate), cliprate_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightStyle), iswelllog_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightSingleCol), issinglecol_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightDataRange), isdatarange_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightRepeatLog), repeat_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightOverlapp), repeatovlap_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightSeisColor), seiscolor_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightSeqname), seqname_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightLogWidth), logwidth_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightScale), islogarithmic_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightColTabFlipped), iscoltabflipped_ );
}


void Well::DisplayProperties::Log::doFillLeftPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey(subjectName(),sKeyLeftColor), color_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftSize), size_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftName), name_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftRange), range_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftFillName), fillname_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftFillRange), fillrange_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftLeftFill), isleftfill_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftRightFill), isrightfill_ );
    iop.setYN(IOPar::compKey(subjectName(),sKeyLeftRevertRange),islogreverted_);
    iop.set( IOPar::compKey(subjectName(),sKeyLeftCliprate), cliprate_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftStyle), iswelllog_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftSingleCol), issinglecol_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftDataRange), isdatarange_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftRepeatLog), repeat_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftOverlapp), repeatovlap_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftSeisColor), seiscolor_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftSeqname), seqname_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftLogWidth), logwidth_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftScale), islogarithmic_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftColTabFlipped), iscoltabflipped_ );
}


void Well::DisplayProperties::Log::doFillRightPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey(subjectName(),sKeyRightColor), color_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightSize), size_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightName), name_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightRange), range_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightFillName), fillname_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightFillRange), fillrange_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightLeftFill), isleftfill_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightRightFill), isrightfill_ );
    iop.setYN(IOPar::compKey(subjectName(),sKeyRightRevertRange),islogreverted_);
    iop.set( IOPar::compKey(subjectName(),sKeyRightCliprate), cliprate_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightStyle), iswelllog_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightSingleCol), issinglecol_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightDataRange), isdatarange_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightRepeatLog), repeat_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightOverlapp), repeatovlap_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightSeisColor), seiscolor_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightSeqname), seqname_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightLogWidth), logwidth_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightScale), islogarithmic_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightColTabFlipped), iscoltabflipped_ );
}


void Well::DisplayProperties::usePar( const IOPar& iop )
{
    IOPar* par = iop.subselect( subjectName() );
    if ( !par ) par = new IOPar( iop );
    track_.usePar( *par );
    markers_.usePar( *par );
    logs_[0]->left_.useLeftPar( *par );
    logs_[0]->right_.useRightPar( *par );
    int widx=1; IOPar* welliop = par->subselect( toString(widx) );
    while ( welliop )
    {
	logs_ += new LogCouple();
	logs_[widx]->left_.useLeftPar( *welliop );
	logs_[widx]->right_.useRightPar( *welliop );
	widx ++;
	delete welliop;
	welliop = par->subselect( toString(widx) );
    }
    par->getYN(IOPar::compKey(subjectName(),sKey2DDisplayStrat),displaystrat_);
    delete par;
}


void Well::DisplayProperties::fillPar( IOPar& iop ) const
{
    IOPar par;
    track_.fillPar( par );
    markers_.fillPar( par );
    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	IOPar tmpiop;
	logs_[idx]->left_.fillLeftPar( tmpiop );
	logs_[idx]->right_.fillRightPar( tmpiop );
	par.mergeComp( tmpiop, idx ? toString( idx ) : "" );
	//keeps compatibility with former versions 
    }
    par.setYN(IOPar::compKey(subjectName(),sKey2DDisplayStrat),displaystrat_);
    iop.mergeComp( par, subjectName() );
}


Well::DisplayProperties& Well::DisplayProperties::defaults()
{
    static Well::DisplayProperties* ret = 0;

    if ( !ret )
    {
	Settings& setts = Settings::fetch( "welldisp" );
	ret = new DisplayProperties;
	ret->usePar( setts );
    }

    return *ret;
}


void Well::DisplayProperties::commitDefaults()
{
    Settings& setts = Settings::fetch( "welldisp" );
    defaults().fillPar( setts );
    setts.write();
}

