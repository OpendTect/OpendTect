/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "welldisp.h"
#include "welldata.h"
#include "welllogset.h"
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
static const char* sKey2DDisplayStrat = "Display Stratigraphy";

static const char* sKeyLeftColor = "Left Log Color";
static const char* sKeyLeftSize = "Left Log Size";
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
static const char* sKeyLeftColTabFlipped = "Left Log Color Table Flipped";
static const char* sKeyLeftScale = "Left Log scale";
static const char* sKeyLeftLogStyle = "Left Log Style";
static const char* sKeyLeftLogWidthXY = "Left Log Width XY";

static const char* sKeyRightColor = "Right Log Color";
static const char* sKeyRightSize = "Right Log Size";
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
static const char* sKeyRightScale = "Right Log scale";
static const char* sKeyRightColTabFlipped = "Right Log Color Table Flipped";
static const char* sKeyRightLogStyle = "Right Log Style";
static const char* sKeyRightLogWidthXY = "Right Log Width XY";

static const char* sKeyCenterColor = "Center Log Color";
static const char* sKeyCenterSize = "Center Log Size";
static const char* sKeyCenterLeftFill = "Left Fill Center Log";
static const char* sKeyCenterRightFill = "Center Fill Right Log";
static const char* sKeyCenterSingleCol = "Center Single Fill Color";
static const char* sKeyCenterDataRange = "Center Data Range Bool";
static const char* sKeyCenterOverlapp = "Center Log Overlapp";
static const char* sKeyCenterRepeatLog = "Center Log Number";
static const char* sKeyCenterSeisColor = "Center Log Seismic Style Color";
static const char* sKeyCenterName = "Center Log Name";
static const char* sKeyCenterFillName = "Center Filled Log name";
static const char* sKeyCenterCliprate = "Center Cliprate";
static const char* sKeyCenterFillRange = "Center Filled Log Range";
static const char* sKeyCenterRange = "Center Log Range";
static const char* sKeyCenterRevertRange = "Center Revert Range Bool";
static const char* sKeyCenterSeqname = "Center Sequence name";
static const char* sKeyCenterScale = "Center Log scale";
static const char* sKeyCenterColTabFlipped = "Center Log Color Table Flipped";
static const char* sKeyCenterLogStyle = "Center Log Style";
static const char* sKeyCenterLogWidthXY = "Center Log Width XY";



Well::DisplayProperties::DisplayProperties( const char* subjname )
    : subjectname_(subjname)
    , displaystrat_(false)
{
    logs_ += new LogCouple();

    logs_[0]->left_.isrightfill_ = true;
    logs_[0]->right_.isleftfill_ = true;
    logs_[0]->center_.isleftfill_ = true;

    logs_[0]->center_.style_ = 2;

    const Settings& setts = Settings::fetch( "welldisp" );
    markers_.selmarkernms_.erase();
    usePar( setts );
}


Well::DisplayProperties::~DisplayProperties()
{
    deepErase( logs_ );
}


Well::DisplayProperties& Well::DisplayProperties::operator = (
					const Well::DisplayProperties& dp )
{
    track_ = dp.track_;
    markers_ = dp.markers_;
    displaystrat_ = dp.displaystrat_;
    if ( logs_.size() != dp.logs_.size() )
	deepCopy( logs_, dp.logs_ );
    else
	for ( int idx=0; idx<logs_.size(); idx++ )
	    *logs_[idx] = *dp.logs_[idx];

    markers_.selmarkernms_ = dp.markers_.selmarkernms_;
    return *this;

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


void Well::DisplayProperties::BasicProps::useCenterPar( const IOPar& iop )
{
    iop.get( IOPar::compKey(subjectName(),sKey::Color()), color_ );
    iop.get( IOPar::compKey(subjectName(),sKey::Size()), size_ );
    doUseCenterPar( iop );
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


void Well::DisplayProperties::BasicProps::fillCenterPar( IOPar& iop ) const
{
    doFillCenterPar( iop );
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
    par.set( IOPar::compKey(subjectName(),sKeyMarkerCylinderHeight),
	     cylinderheight_ );
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
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftSingleCol), issinglecol_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftDataRange), isdatarange_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftRepeatLog), repeat_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftOverlapp), repeatovlap_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftSeisColor), seiscolor_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftSeqname), seqname_ );

    float logwidth = logwidth_==0 ? 250.f : logwidth_;
    iop.get( IOPar::compKey(subjectName(),sKeyLeftLogWidthXY), logwidth );
    if ( SI().xyInFeet() )
	logwidth *= mToFeetFactorF;
    logwidth_ = mNINT32( logwidth );

    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftScale), islogarithmic_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyLeftColTabFlipped),
	       iscoltabflipped_ );
    iop.get( IOPar::compKey(subjectName(),sKeyLeftLogStyle),style_);

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
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightRevertRange),
	       islogreverted_);
    iop.get( IOPar::compKey(subjectName(),sKeyRightCliprate), cliprate_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightSingleCol), issinglecol_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightDataRange), isdatarange_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightRepeatLog), repeat_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightOverlapp), repeatovlap_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightSeisColor), seiscolor_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightSeqname), seqname_ );

    float logwidth = logwidth_==0 ? 250.f : logwidth_;
    iop.get( IOPar::compKey(subjectName(),sKeyRightLogWidthXY), logwidth );
    if ( SI().xyInFeet() )
	logwidth *= mToFeetFactorF;
    logwidth_ = mNINT32( logwidth );

    iop.getYN( IOPar::compKey(subjectName(),sKeyRightScale), islogarithmic_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyRightColTabFlipped),
	       iscoltabflipped_ );
    iop.get( IOPar::compKey(subjectName(),sKeyRightLogStyle),style_);
}


void Well::DisplayProperties::Log::doUseCenterPar( const IOPar& iop )
{
    iop.get( IOPar::compKey(subjectName(),sKeyCenterColor), color_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterSize), size_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterName), name_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterRange), range_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterFillName), fillname_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterFillRange), fillrange_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyCenterLeftFill), isleftfill_ );
    iop.getYN(IOPar::compKey(subjectName(),sKeyCenterRightFill), isrightfill_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyCenterRevertRange),
	       islogreverted_);
    iop.get( IOPar::compKey(subjectName(),sKeyCenterCliprate), cliprate_ );
    iop.getYN(IOPar::compKey(subjectName(),sKeyCenterSingleCol), issinglecol_ );
    iop.getYN(IOPar::compKey(subjectName(),sKeyCenterDataRange), isdatarange_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterRepeatLog), repeat_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterOverlapp), repeatovlap_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterSeisColor), seiscolor_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterSeqname), seqname_ );

    float logwidth = logwidth_==0 ? 250.f : logwidth_;
    iop.get( IOPar::compKey(subjectName(),sKeyCenterLogWidthXY), logwidth );
    if ( SI().xyInFeet() )
	logwidth *= mToFeetFactorF;
    logwidth_ = mNINT32( logwidth );

    iop.getYN( IOPar::compKey(subjectName(),sKeyCenterScale), islogarithmic_ );
    iop.getYN( IOPar::compKey(subjectName(),sKeyCenterColTabFlipped),
	       iscoltabflipped_ );
    iop.get( IOPar::compKey(subjectName(),sKeyCenterLogStyle),style_);
}


void Well::DisplayProperties::Log::doFillLeftPar( IOPar& iop ) const
{
    float logwidth = (float)logwidth_;
    if ( SI().xyInFeet() )
	logwidth *= mFromFeetFactorF;

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
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftSingleCol), issinglecol_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftDataRange), isdatarange_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftRepeatLog), repeat_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftOverlapp), repeatovlap_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftSeisColor), seiscolor_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftSeqname), seqname_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftLogWidthXY), logwidth );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftScale), islogarithmic_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyLeftColTabFlipped),
	       iscoltabflipped_ );
    iop.set( IOPar::compKey(subjectName(),sKeyLeftLogStyle),style_);
}


void Well::DisplayProperties::Log::doFillRightPar( IOPar& iop ) const
{
    float logwidth = (float)logwidth_;
    if ( SI().xyInFeet() )
	logwidth *= mFromFeetFactorF;

    iop.set( IOPar::compKey(subjectName(),sKeyRightColor), color_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightSize), size_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightName), name_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightRange), range_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightFillName), fillname_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightFillRange), fillrange_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightLeftFill), isleftfill_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightRightFill), isrightfill_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightRevertRange),
	       islogreverted_);
    iop.set( IOPar::compKey(subjectName(),sKeyRightCliprate), cliprate_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightSingleCol), issinglecol_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightDataRange), isdatarange_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightRepeatLog), repeat_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightOverlapp), repeatovlap_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightSeisColor), seiscolor_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightSeqname), seqname_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightLogWidthXY), logwidth );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightScale), islogarithmic_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyRightColTabFlipped),
	       iscoltabflipped_ );
    iop.set( IOPar::compKey(subjectName(),sKeyRightLogStyle),style_);

}


void Well::DisplayProperties::Log::doFillCenterPar( IOPar& iop ) const
{
    float logwidth = (float)logwidth_;
    if ( SI().xyInFeet() )
	logwidth *= mFromFeetFactorF;

    iop.set( IOPar::compKey(subjectName(),sKeyCenterColor), color_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterSize), size_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterName), name_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterRange), range_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterFillName), fillname_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterFillRange), fillrange_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyCenterLeftFill), isleftfill_ );
    iop.setYN(IOPar::compKey(subjectName(),sKeyCenterRightFill), isrightfill_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyCenterRevertRange),
	       islogreverted_);
    iop.set( IOPar::compKey(subjectName(),sKeyCenterCliprate), cliprate_ );
    iop.setYN(IOPar::compKey(subjectName(),sKeyCenterSingleCol), issinglecol_ );
    iop.setYN(IOPar::compKey(subjectName(),sKeyCenterDataRange), isdatarange_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterRepeatLog), repeat_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterOverlapp), repeatovlap_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterSeisColor), seiscolor_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterSeqname), seqname_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterLogWidthXY), logwidth );
    iop.setYN( IOPar::compKey(subjectName(),sKeyCenterScale), islogarithmic_ );
    iop.setYN( IOPar::compKey(subjectName(),sKeyCenterColTabFlipped),
	       iscoltabflipped_ );
    iop.set( IOPar::compKey(subjectName(),sKeyCenterLogStyle),style_);

}


void Well::DisplayProperties::Log::setTo( const Well::Data* wd, const Log& oth,
					  bool forceIfMissing  )
{
    if ( forceIfMissing || oth.name_=="None" || wd->logs().getLog( oth.name_ ) )
	*this = oth;
}


void Well::DisplayProperties::usePar( const IOPar& iop )
{
    IOPar* par = iop.subselect( subjectName() );
    if ( !par ) par = new IOPar( iop );
    track_.usePar( *par );
    markers_.usePar( *par );
    logs_[0]->left_.useLeftPar( *par );
    logs_[0]->right_.useRightPar( *par );
    logs_[0]->center_.useCenterPar( *par );
    for ( int idx=logs_.size()-1; idx>0; idx-- )
	delete logs_.removeSingle( idx );

    int widx=1; IOPar* welliop = par->subselect( toString(widx) );
    while ( welliop )
    {
	logs_ += new LogCouple();
	logs_[widx]->left_.useLeftPar( *welliop );
	logs_[widx]->right_.useRightPar( *welliop );
	logs_[widx]->center_.useCenterPar( *welliop );
	widx++;
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
	logs_[idx]->center_.fillCenterPar( tmpiop );
	par.mergeComp( tmpiop, idx ? toString( idx ) : "" );
	//keeps compatibility with former versions
    }
    par.setYN(IOPar::compKey(subjectName(),sKey2DDisplayStrat),displaystrat_);
    iop.mergeComp( par, subjectName() );
}


void Well::DisplayProperties::ensureColorContrastWith( Color bkcol )
{
    TypeSet<Color> usecols = bkcol.complimentaryColors( 2 );
    if ( bkcol.contrast(logs_[0]->left_.color_)<4.5 )
	logs_[0]->left_.color_ = usecols[0];
    if ( bkcol.contrast(logs_[0]->center_.color_)<4.5 )
	logs_[0]->center_.color_ = usecols[0];
    if ( bkcol.contrast(logs_[0]->right_.color_)<4.5 )
	logs_[0]->right_.color_ = usecols[0];
    if ( bkcol.contrast(markers_.color_)<4.5 )
	markers_.color_ = usecols[1];
    if ( bkcol.contrast(track_.color_)<4.5 )
	track_.color_ = usecols[1];
}


Well::DisplayProperties& Well::DisplayProperties::defaults()
{
    mDefineStaticLocalObject( PtrMan<Well::DisplayProperties>, ret, = 0 );

    if ( !ret )
    {
	const Settings& setts = Settings::fetch( "welldisp" );
	Well::DisplayProperties* newret = new DisplayProperties;
	newret->usePar( setts );

	ret.setIfNull(newret,true);
    }

    return *ret;
}


void Well::DisplayProperties::commitDefaults()
{
    Settings& setts = Settings::fetch( "welldisp" );
    defaults().fillPar( setts );
    setts.write();
}
