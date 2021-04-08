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
static const char* sKeyTrackDynamicNmSize = "Track Dynamic Name Size";
static const char* sKeyTrackNmSize = "Track Name Size";
static const char* sKeyTrackNmFont = "Track Font";
static const char* sKeyMarkerShape = "Marker Shape";
static const char* sKeyMarkerCylinderHeight = "Cylinder Height";
static const char* sKeyMarkerDynamicNmSize = "Marker Dynamic Name Size";
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
static const char* sKeyLeftOverlap = "Left Log Overlapp";
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
static const char* sKeyRightOverlap = "Right Log Overlapp";
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
static const char* sKeyCenterOverlap = "Center Log Overlapp";
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
    PtrMan<IOPar> trackpar = par.subselect( subjectName() );
    if ( !trackpar )
	return;

    trackpar->getYN( sKeyTrackNmIsAbove, dispabove_ );
    trackpar->getYN( sKeyTrackNmIsBelow, dispbelow_ );
    if ( !trackpar->getYN(sKeyTrackDynamicNmSize,nmsizedynamic_) )
	nmsizedynamic_ = true; //Legacy

    const FixedString fontdata = trackpar->find( sKeyTrackNmFont );
    if ( fontdata )
	font_.getFrom( fontdata );
    else
    {
	int sz = 0;
	trackpar->get( sKeyTrackNmSize, sz );
	font_.setPointSize( sz );
    }

    if ( font_.pointSize()==0 )
	font_.setPointSize( FontData::defaultPointSize() );
}


void Well::DisplayProperties::Track::doFillPar( IOPar& par ) const
{
    IOPar trackpar;
    trackpar.setYN( sKeyTrackNmIsAbove, dispabove_ );
    trackpar.setYN( sKeyTrackNmIsBelow, dispbelow_ );
    trackpar.setYN( sKeyTrackDynamicNmSize, nmsizedynamic_ );
    BufferString fontdata;
    font_.putTo( fontdata );
    trackpar.set( sKeyTrackNmFont, fontdata );
    par.mergeComp( trackpar, subjectName() );
}


void Well::DisplayProperties::Markers::doUsePar( const IOPar& par )
{
    PtrMan<IOPar> mrkspar = par.subselect( subjectName() );
    if ( !mrkspar )
	return;

    mrkspar->getYN( sKeyMarkerSingleColor, issinglecol_ );
    mrkspar->get( sKeyMarkerShape, shapeint_ );
    mrkspar->get( sKeyMarkerCylinderHeight, cylinderheight_ );
    mrkspar->getYN( sKeyMarkerNmSameColor, samenmcol_);
    mrkspar->get( sKeyMarkerNmColor, nmcol_ );
    mrkspar->get( sKeyMarkerSelected, selmarkernms_ );
    if ( !mrkspar->getYN(sKeyMarkerDynamicNmSize,nmsizedynamic_) )
	nmsizedynamic_ = true; //Legacy

    const FixedString fontdata =
	mrkspar->find( sKeyMarkerNmFont );
    if ( fontdata )
	font_.getFrom( fontdata );
    else
    {
	int sz = 0;
	mrkspar->get( sKeyMarkerNmSize, sz );
	font_.setPointSize( sz );
    }

    if ( font_.pointSize()==0 )
	font_.setPointSize( FontData::defaultPointSize() );
}


void Well::DisplayProperties::Markers::doFillPar( IOPar& par ) const
{
    IOPar mrkspar;
    mrkspar.setYN( sKeyMarkerSingleColor,issinglecol_);
    mrkspar.set( sKeyMarkerShape, shapeint_ );
    mrkspar.set( sKeyMarkerCylinderHeight, cylinderheight_ );
    mrkspar.setYN( sKeyMarkerDynamicNmSize, nmsizedynamic_ );
    BufferString fontdata;
    font_.putTo( fontdata );
    mrkspar.set( sKeyMarkerNmFont, fontdata );
    mrkspar.setYN( sKeyMarkerNmSameColor, samenmcol_);
    mrkspar.set( sKeyMarkerNmColor, nmcol_ );
    mrkspar.set( sKeyMarkerSelected, selmarkernms_ );
    par.mergeComp( mrkspar, subjectName() );
}


void Well::DisplayProperties::Log::doUseLeftPar( const IOPar& par )
{
    PtrMan<IOPar> leftlogpar = par.subselect( subjectName() );
    if ( !leftlogpar )
	return;

    leftlogpar->get( sKeyLeftColor, color_ );
    leftlogpar->get( sKeyLeftSize, size_ );
    leftlogpar->get( sKeyLeftName, name_ );
    leftlogpar->get( sKeyLeftRange, range_ );
    leftlogpar->get( sKeyLeftFillName, fillname_ );
    leftlogpar->get( sKeyLeftFillRange, fillrange_ );
    leftlogpar->getYN( sKeyLeftLeftFill, isleftfill_ );
    leftlogpar->getYN( sKeyLeftRightFill, isrightfill_ );
    leftlogpar->getYN( sKeyLeftRevertRange,islogreverted_ );
    leftlogpar->get( sKeyLeftCliprate, cliprate_ );
    leftlogpar->getYN( sKeyLeftSingleCol, issinglecol_ );
    leftlogpar->getYN( sKeyLeftDataRange, isdatarange_ );
    leftlogpar->get( sKeyLeftRepeatLog, repeat_ );
    leftlogpar->get( sKeyLeftOverlap, repeatovlap_ );
    leftlogpar->get( sKeyLeftSeisColor, seiscolor_ );
    leftlogpar->get( sKeyLeftSeqname, seqname_ );

    float logwidth = logwidth_==0 ? 250.f : logwidth_;
    leftlogpar->get( sKeyLeftLogWidthXY, logwidth );
    if ( SI().xyInFeet() )
	logwidth *= mToFeetFactorF;
    logwidth_ = mNINT32( logwidth );

    leftlogpar->getYN( sKeyLeftScale, islogarithmic_ );
    leftlogpar->getYN( sKeyLeftColTabFlipped, iscoltabflipped_ );
    leftlogpar->get( sKeyLeftLogStyle, style_ );
}


void Well::DisplayProperties::Log::doUseRightPar( const IOPar& par )
{
    PtrMan<IOPar> rightlogpar = par.subselect( subjectName() );
    if ( !rightlogpar )
	return;

    rightlogpar->get( sKeyRightColor, color_ );
    rightlogpar->get( sKeyRightSize, size_ );
    rightlogpar->get( sKeyRightName, name_ );
    rightlogpar->get( sKeyRightRange, range_ );
    rightlogpar->get( sKeyRightFillName, fillname_ );
    rightlogpar->get( sKeyRightFillRange, fillrange_ );
    rightlogpar->getYN( sKeyRightLeftFill, isleftfill_ );
    rightlogpar->getYN( sKeyRightRightFill, isrightfill_ );
    rightlogpar->getYN( sKeyRightRevertRange, islogreverted_ );
    rightlogpar->get( sKeyRightCliprate, cliprate_ );
    rightlogpar->getYN( sKeyRightSingleCol, issinglecol_ );
    rightlogpar->getYN( sKeyRightDataRange, isdatarange_ );
    rightlogpar->get( sKeyRightRepeatLog, repeat_ );
    rightlogpar->get( sKeyRightOverlap, repeatovlap_ );
    rightlogpar->get( sKeyRightSeisColor, seiscolor_ );
    rightlogpar->get( sKeyRightSeqname, seqname_ );

    float logwidth = logwidth_==0 ? 250.f : logwidth_;
    rightlogpar->get( sKeyRightLogWidthXY, logwidth );
    if ( SI().xyInFeet() )
	logwidth *= mToFeetFactorF;
    logwidth_ = mNINT32( logwidth );

    rightlogpar->getYN( sKeyRightScale, islogarithmic_ );
    rightlogpar->getYN( sKeyRightColTabFlipped, iscoltabflipped_ );
    rightlogpar->get( sKeyRightLogStyle, style_ );
}


void Well::DisplayProperties::Log::doUseCenterPar( const IOPar& par )
{
    PtrMan<IOPar> centerlogpar = par.subselect( subjectName() );
    if ( !centerlogpar )
	return;

    centerlogpar->get( sKeyCenterColor, color_ );
    centerlogpar->get( sKeyCenterSize, size_ );
    centerlogpar->get( sKeyCenterName, name_ );
    centerlogpar->get( sKeyCenterRange, range_ );
    centerlogpar->get( sKeyCenterFillName, fillname_ );
    centerlogpar->get( sKeyCenterFillRange, fillrange_ );
    centerlogpar->getYN( sKeyCenterLeftFill, isleftfill_ );
    centerlogpar->getYN( sKeyCenterRightFill, isrightfill_ );
    centerlogpar->getYN( sKeyCenterRevertRange, islogreverted_ );
    centerlogpar->get( sKeyCenterCliprate, cliprate_ );
    centerlogpar->getYN( sKeyCenterSingleCol, issinglecol_ );
    centerlogpar->getYN( sKeyCenterDataRange, isdatarange_ );
    centerlogpar->get( sKeyCenterRepeatLog, repeat_ );
    centerlogpar->get( sKeyCenterOverlap, repeatovlap_ );
    centerlogpar->get( sKeyCenterSeisColor, seiscolor_ );
    centerlogpar->get( sKeyCenterSeqname, seqname_ );

    float logwidth = logwidth_==0 ? 250.f : logwidth_;
    centerlogpar->get( sKeyCenterLogWidthXY, logwidth );
    if ( SI().xyInFeet() )
	logwidth *= mToFeetFactorF;
    logwidth_ = mNINT32( logwidth );

    centerlogpar->getYN( sKeyCenterScale, islogarithmic_ );
    centerlogpar->getYN( sKeyCenterColTabFlipped, iscoltabflipped_ );
    centerlogpar->get( sKeyCenterLogStyle, style_ );
}


void Well::DisplayProperties::Log::doFillLeftPar( IOPar& par ) const
{
    float logwidth = float(logwidth_);
    if ( SI().xyInFeet() )
	logwidth *= mFromFeetFactorF;

    IOPar leftlogpar;
    leftlogpar.set( sKeyLeftColor, color_ );
    leftlogpar.set( sKeyLeftSize, size_ );
    leftlogpar.set( sKeyLeftName, name_ );
    leftlogpar.set( sKeyLeftRange, range_ );
    leftlogpar.set( sKeyLeftFillName, fillname_ );
    leftlogpar.set( sKeyLeftFillRange, fillrange_ );
    leftlogpar.setYN( sKeyLeftLeftFill, isleftfill_ );
    leftlogpar.setYN( sKeyLeftRightFill, isrightfill_ );
    leftlogpar.setYN( sKeyLeftRevertRange, islogreverted_ );
    leftlogpar.set( sKeyLeftCliprate, cliprate_ );
    leftlogpar.setYN( sKeyLeftSingleCol, issinglecol_ );
    leftlogpar.setYN( sKeyLeftDataRange, isdatarange_ );
    leftlogpar.set( sKeyLeftRepeatLog, repeat_ );
    leftlogpar.set( sKeyLeftOverlap, repeatovlap_ );
    leftlogpar.set( sKeyLeftSeisColor, seiscolor_ );
    leftlogpar.set( sKeyLeftSeqname, seqname_ );
    leftlogpar.set( sKeyLeftLogWidthXY, logwidth );
    leftlogpar.setYN( sKeyLeftScale, islogarithmic_ );
    leftlogpar.setYN( sKeyLeftColTabFlipped, iscoltabflipped_ );
    leftlogpar.set( sKeyLeftLogStyle, style_ );
    par.mergeComp( leftlogpar, subjectName() );
}


void Well::DisplayProperties::Log::doFillRightPar( IOPar& par ) const
{
    float logwidth = float(logwidth_);
    if ( SI().xyInFeet() )
	logwidth *= mFromFeetFactorF;

    IOPar rightlogpar;
    rightlogpar.set( sKeyRightColor, color_ );
    rightlogpar.set( sKeyRightSize, size_ );
    rightlogpar.set( sKeyRightName, name_ );
    rightlogpar.set( sKeyRightRange, range_ );
    rightlogpar.set( sKeyRightFillName, fillname_ );
    rightlogpar.set( sKeyRightFillRange, fillrange_ );
    rightlogpar.setYN( sKeyRightLeftFill, isleftfill_ );
    rightlogpar.setYN( sKeyRightRightFill, isrightfill_ );
    rightlogpar.setYN( sKeyRightRevertRange, islogreverted_ );
    rightlogpar.set( sKeyRightCliprate, cliprate_ );
    rightlogpar.setYN( sKeyRightSingleCol, issinglecol_ );
    rightlogpar.setYN( sKeyRightDataRange, isdatarange_ );
    rightlogpar.set( sKeyRightRepeatLog, repeat_ );
    rightlogpar.set( sKeyRightOverlap, repeatovlap_ );
    rightlogpar.set( sKeyRightSeisColor, seiscolor_ );
    rightlogpar.set( sKeyRightSeqname, seqname_ );
    rightlogpar.set( sKeyRightLogWidthXY, logwidth );
    rightlogpar.setYN( sKeyRightScale, islogarithmic_ );
    rightlogpar.setYN( sKeyRightColTabFlipped, iscoltabflipped_ );
    rightlogpar.set( sKeyRightLogStyle, style_ );
    par.mergeComp( rightlogpar, subjectName() );
}


void Well::DisplayProperties::Log::doFillCenterPar( IOPar& par ) const
{
    float logwidth = float(logwidth_);
    if ( SI().xyInFeet() )
	logwidth *= mFromFeetFactorF;

    IOPar centerlogpar;
    centerlogpar.set( sKeyCenterColor, color_ );
    centerlogpar.set( sKeyCenterSize, size_ );
    centerlogpar.set( sKeyCenterName, name_ );
    centerlogpar.set( sKeyCenterRange, range_ );
    centerlogpar.set( sKeyCenterFillName, fillname_ );
    centerlogpar.set( sKeyCenterFillRange, fillrange_ );
    centerlogpar.setYN( sKeyCenterLeftFill, isleftfill_ );
    centerlogpar.setYN( sKeyCenterRightFill, isrightfill_ );
    centerlogpar.setYN( sKeyCenterRevertRange, islogreverted_ );
    centerlogpar.set( sKeyCenterCliprate, cliprate_ );
    centerlogpar.setYN( sKeyCenterSingleCol, issinglecol_ );
    centerlogpar.setYN( sKeyCenterDataRange, isdatarange_ );
    centerlogpar.set( sKeyCenterRepeatLog, repeat_ );
    centerlogpar.set( sKeyCenterOverlap, repeatovlap_ );
    centerlogpar.set( sKeyCenterSeisColor, seiscolor_ );
    centerlogpar.set( sKeyCenterSeqname, seqname_ );
    centerlogpar.set( sKeyCenterLogWidthXY, logwidth );
    centerlogpar.setYN( sKeyCenterScale, islogarithmic_ );
    centerlogpar.setYN( sKeyCenterColTabFlipped, iscoltabflipped_ );
    centerlogpar.set( sKeyCenterLogStyle, style_ );
    par.mergeComp( centerlogpar, subjectName() );
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


void Well::DisplayProperties::ensureColorContrastWith( OD::Color bkcol )
{
    TypeSet<OD::Color> usecols = bkcol.complimentaryColors( 2 );
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
