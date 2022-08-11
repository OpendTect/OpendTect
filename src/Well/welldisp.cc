/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


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
static const char* sKeyMarkerUnselected = "Unselected Markers";

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

static const char* sKey2DDisplayStrat = "Display Stratigraphy";



Well::DisplayProperties::DisplayProperties( const char* subjname )
    : subjectname_(subjname)
{
    logs_ += new LogCouple();

    logs_[0]->left_.isrightfill_ = true;
    logs_[0]->center_.isleftfill_ = true;
    logs_[0]->right_.isleftfill_ = true;
    logs_[0]->center_.style_ = 2;

    const Settings& setts = Settings::fetch( "welldisp" );
    PtrMan<IOPar> par = setts.subselect( subjname );
    usePar( par ? *par.ptr() : (const IOPar&)(setts) );
    setValid( false );
    setModified( false );
}


Well::DisplayProperties::DisplayProperties( const Well::DisplayProperties& dp )
{
    *this = dp;
    setValid( dp.isValid() );
    setModified( false );
}


Well::DisplayProperties::~DisplayProperties()
{
    deepErase( logs_ );
}


Well::DisplayProperties& Well::DisplayProperties::operator = (
					const DisplayProperties& oth )
{
    if ( &oth == this )
	return *this;

    const bool modified = oth != *this;
    subjectname_ = oth.subjectname_;
    track_ = oth.track_;
    markers_ = oth.markers_;
    displaystrat_ = oth.displaystrat_;
    if ( logs_.size() != oth.logs_.size() )
	deepCopy( logs_, oth.logs_ );
    else
	for ( int idx=0; idx<logs_.size(); idx++ )
	    *logs_[idx] = *oth.logs_[idx];

    setModified( modified );

    return *this;
}


bool Well::DisplayProperties::operator ==( const DisplayProperties& oth ) const
{
    if ( &oth == this )
	return true;

    if ( StringView(subjectName()) != oth.subjectName() ||
	 track_ != oth.track_ ||
	 markers_ != oth.markers_ ||
	 displaystrat_ != oth.displaystrat_ ||
	 logs_.size() != oth.logs_.size() )
	return false;

    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	if ( *logs_.get(idx) != *oth.logs_.get(idx) )
	    return false;
    }

    return true;
}


bool Well::DisplayProperties::operator !=( const DisplayProperties& oth ) const
{
    return !(oth == *this);
}


bool Well::DisplayProperties::is2D() const
{
    return StringView(subjectName()) == sKey2DDispProp();
}



Well::DisplayProperties::BasicProps::BasicProps( int sz )
    : size_(sz)
{
}


Well::DisplayProperties::BasicProps::~BasicProps()
{
}


bool Well::DisplayProperties::BasicProps::operator ==(
						const BasicProps& oth ) const
{
    return size_ == oth.size_ && color_ == oth.color_ &&
	   StringView(subjectName()) == oth.subjectName();
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


void Well::DisplayProperties::BasicProps::useCenterPar( const IOPar& iop )
{
    iop.get( IOPar::compKey(subjectName(),sKey::Color()), color_ );
    iop.get( IOPar::compKey(subjectName(),sKey::Size()), size_ );
    doUseCenterPar( iop );
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


void Well::DisplayProperties::BasicProps::fillCenterPar( IOPar& iop ) const
{
    doFillCenterPar( iop );
}


void Well::DisplayProperties::BasicProps::fillRightPar( IOPar& iop ) const
{
    doFillRightPar( iop );
}



Well::DisplayProperties::Track::Track()
  : BasicProps(1)
{}


Well::DisplayProperties::Track::Track( const Track& oth )
  : BasicProps(oth)
{ *this = oth; }


Well::DisplayProperties::Track::~Track()
{
}


bool Well::DisplayProperties::Track::operator ==( const Track& oth ) const
{
    return BasicProps::operator==(oth) &&
	   dispabove_ == oth.dispabove_ && dispbelow_ == oth.dispbelow_ &&
	   nmsizedynamic_ == oth.nmsizedynamic_ &&
	   font_ == oth.font_;
}


bool Well::DisplayProperties::Track::operator !=( const Track& oth ) const
{
    return !(*this == oth );
}


void Well::DisplayProperties::Track::doUsePar( const IOPar& par )
{
    PtrMan<IOPar> trackpar = par.subselect( subjectName() );
    if ( !trackpar )
	return;

    trackpar->getYN( sKeyTrackNmIsAbove, dispabove_ );
    trackpar->getYN( sKeyTrackNmIsBelow, dispbelow_ );
    trackpar->getYN( sKeyTrackDynamicNmSize, nmsizedynamic_ );

    const StringView fontdata = trackpar->find( sKeyTrackNmFont );
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
    BufferString fontdata;
    font_.putTo( fontdata );
    trackpar.set( sKeyTrackNmFont, fontdata );
    trackpar.setYN( sKeyTrackDynamicNmSize, nmsizedynamic_ );
    par.mergeComp( trackpar, subjectName() );
}



Well::DisplayProperties::Markers::Markers()
    : BasicProps(5)
{
}


Well::DisplayProperties::Markers::Markers( const Markers& oth )
    : BasicProps(oth)
{ *this = oth; }


Well::DisplayProperties::Markers::~Markers()
{
}


Well::DisplayProperties::Markers&
    Well::DisplayProperties::Markers::operator=( const Markers& oth )
{
    if ( &oth == this )
	return *this;

    BasicProps::operator=( oth );
    shapeint_ = oth.shapeint_;
    cylinderheight_ = oth.cylinderheight_;
    issinglecol_ = oth.issinglecol_;
    font_ = oth.font_;
    samenmcol_ = oth.samenmcol_;
    nmsizedynamic_ = oth.nmsizedynamic_;
    selmarkernms_ = oth.selmarkernms_;
    unselmarkernms_ = oth.unselmarkernms_;

    return *this;
}


bool Well::DisplayProperties::Markers::operator ==( const Markers& oth ) const
{
    bool nmcoloreq = !samenmcol_ && !oth.samenmcol_ ? nmcol_==oth.nmcol_ : true;
    return BasicProps::operator==(oth) &&
	   shapeint_ == oth.shapeint_ &&
	   cylinderheight_ == oth.cylinderheight_ &&
	   issinglecol_ == oth.issinglecol_ &&
	   font_ == oth.font_ &&
	   nmcoloreq && samenmcol_ == oth.samenmcol_ &&
	   nmsizedynamic_ == oth.nmsizedynamic_ &&
	   selmarkernms_ == oth.selmarkernms_ &&
	   unselmarkernms_ == oth.unselmarkernms_;
}


bool Well::DisplayProperties::Markers::operator !=( const Markers& oth ) const
{
    return !(*this == oth );
}


bool Well::DisplayProperties::Markers::isEmpty() const
{
    return selmarkernms_.isEmpty() && unselmarkernms_.isEmpty();
}


const BufferStringSet& Well::DisplayProperties::Markers::markerNms(
							bool issel ) const
{
    return issel ? selmarkernms_ : unselmarkernms_;
}


void Well::DisplayProperties::Markers::setMarkerNms( const BufferStringSet& nms,
						     bool issel )
{
    BufferStringSet& markernms = issel ? selmarkernms_ : unselmarkernms_;
    markernms = nms;
}


bool Well::DisplayProperties::Markers::isSelected( const char* nm ) const
{
    if ( !selmarkernms_.isEmpty() && selmarkernms_.isPresent(nm) )
	return true;

    if ( !unselmarkernms_.isEmpty() && unselmarkernms_.isPresent(nm) )
	return false;

    return false;
}


void Well::DisplayProperties::Markers::adjustSelection(
					const BufferStringSet& markernms )
{
    for ( int idx=selmarkernms_.size()-1; idx>=0; idx-- )
    {
	if ( !markernms.isPresent(selmarkernms_.get(idx)) )
	    selmarkernms_.removeSingle(idx);
    }

    for ( int idx=unselmarkernms_.size()-1; idx>=0; idx-- )
    {
	if ( !markernms.isPresent(unselmarkernms_.get(idx)) )
	    unselmarkernms_.removeSingle(idx);
    }
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
    BufferStringSet selmarkernms, unselmarkernames;
    if ( mrkspar->get(sKeyMarkerSelected,selmarkernms) )
	selmarkernms_ = selmarkernms;
    if ( mrkspar->get(sKeyMarkerUnselected,unselmarkernames) )
	unselmarkernms_.add( unselmarkernames, false );
    mrkspar->getYN( sKeyMarkerDynamicNmSize, nmsizedynamic_ );

    const StringView fontdata =
	mrkspar->find( sKeyMarkerNmFont );
    if ( fontdata )
	font_.getFrom( fontdata );
    else
    {
	int sz = 0;
	if ( mrkspar->get(sKeyMarkerNmSize,sz) )
	    font_.setPointSize( sz );
    }
}


void Well::DisplayProperties::Markers::doFillPar( IOPar& par ) const
{
    if ( !issinglecol_ )
	par.removeWithKey( IOPar::compKey(subjectName(),sKey::Color()) );

    IOPar mrkspar;
    mrkspar.setYN( sKeyMarkerSingleColor,issinglecol_ );
    mrkspar.set( sKeyMarkerShape, shapeint_ );
    mrkspar.set( sKeyMarkerCylinderHeight, cylinderheight_ );
    BufferString fontdata;
    font_.putTo( fontdata );
    mrkspar.set( sKeyMarkerNmFont, fontdata );
    mrkspar.setYN( sKeyMarkerDynamicNmSize, nmsizedynamic_ );
    mrkspar.setYN( sKeyMarkerNmSameColor, samenmcol_ );
    if ( !samenmcol_ )
	mrkspar.set( sKeyMarkerNmColor, nmcol_ );

    mrkspar.set( sKeyMarkerSelected, selmarkernms_ );
    mrkspar.set( sKeyMarkerUnselected, unselmarkernms_ );
    par.mergeComp( mrkspar, subjectName() );
}



Well::DisplayProperties::Log::Log()
  : BasicProps(1)
  , logwidth_(250 * ((int)(SI().xyInFeet() ? mToFeetFactorF:1)))
{
}


Well::DisplayProperties::Log::Log( const Log& oth )
  : BasicProps(oth)
{ *this = oth; }


Well::DisplayProperties::Log::~Log()
{
}


bool Well::DisplayProperties::Log::operator ==( const Log& oth ) const
{
    return BasicProps::operator==(oth) &&
	   name_ == oth.name_ &&
	   fillname_ == oth.fillname_ && seqname_ == oth.seqname_ &&
	   isleftfill_ == oth.isleftfill_ && isrightfill_ == oth.isrightfill_ &&
	   islogarithmic_ == oth.islogarithmic_ &&
	   islogreverted_ == oth.islogreverted_ &&
	   issinglecol_ == oth.issinglecol_ &&
	   isdatarange_ == oth.isdatarange_ &&
	   iscoltabflipped_ == oth.iscoltabflipped_ &&
	   repeat_ == oth.repeat_ &&
	   linecolor_ == oth.linecolor_ && seiscolor_ == oth.seiscolor_ &&
	   logwidth_ == oth.logwidth_ && style_ == oth.style_ &&
	   repeatovlap_ == oth.repeatovlap_ &&
	   range_ == oth.range_ && fillrange_ == oth.fillrange_;
}


bool Well::DisplayProperties::Log::operator !=( const Log& oth ) const
{
    return !(*this == oth);
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


void Well::DisplayProperties::Log::doFillLeftPar( IOPar& par ) const
{
    if ( name_ == sKey::None() )
	return;

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


void Well::DisplayProperties::Log::doFillCenterPar( IOPar& par ) const
{
    if ( name_ == sKey::None() )
	return;

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


void Well::DisplayProperties::Log::doFillRightPar( IOPar& par ) const
{
    if ( name_ == sKey::None() )
	return;

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



void Well::DisplayProperties::Log::setTo( const Data* wd, const Log& oth,
					  bool forceIfMissing  )
{
    if ( forceIfMissing || oth.name_== sKey::None() ||
	 wd->logs().getLog(oth.name_) )
	*this = oth;
}



Well::DisplayProperties::LogCouple::LogCouple()
{
}


Well::DisplayProperties::LogCouple::LogCouple( const LogCouple& oth )
{
    *this = oth;
}


Well::DisplayProperties::LogCouple::~LogCouple()
{
}


Well::DisplayProperties::LogCouple&
    Well::DisplayProperties::LogCouple::operator=( const LogCouple& oth )
{
    if ( &oth == this )
        return *this;

    left_ = oth.left_;
    center_ = oth.center_;
    right_ = oth.right_;

    return *this;
}


bool Well::DisplayProperties::LogCouple::operator==( const LogCouple& oth) const
{
    const char* nonestr = sKey::None();
    const bool lefteq = (left_.name_==nonestr && oth.left_.name_==nonestr) ||
							    left_==oth.left_;
    const bool righteq = (right_.name_==nonestr && oth.right_.name_==nonestr) ||
							    right_==oth.right_;
    const bool centereq = ( center_.name_==nonestr &&
			oth.center_.name_==nonestr) || center_==oth.center_;

    return lefteq && righteq && centereq;
}


bool Well::DisplayProperties::LogCouple::operator!=( const LogCouple& oth) const
{
    return !(*this == oth);
}


void Well::DisplayProperties::usePar( const IOPar& iop )
{
    if ( iop.isEmpty() )
	return;

    const bool wasvalid = isValid();
    const Well::DisplayProperties snapshot( *this );

    IOPar* par = iop.subselect( subjectName() );
    if ( !par ) par = new IOPar( iop );

    track_.usePar( *par );
    markers_.usePar( *par );
    logs_[0]->left_.useLeftPar( *par );
    logs_[0]->center_.useCenterPar( *par );
    logs_[0]->right_.useRightPar( *par );
    for ( int idx=logs_.size()-1; idx>0; idx-- )
	delete logs_.removeSingle( idx );

    int widx=1; IOPar* welliop = par->subselect( toString(widx) );
    while ( welliop )
    {
	logs_ += new LogCouple();
	logs_[widx]->left_.useLeftPar( *welliop );
	logs_[widx]->center_.useCenterPar( *welliop );
	logs_[widx]->right_.useRightPar( *welliop );
	widx++;
	delete welliop;
	welliop = par->subselect( toString(widx) );
    }

    if ( is2D() )
    {
	if ( !par->getYN(sKey2DDisplayStrat,displaystrat_) )
	    par->getYN(IOPar::compKey(subjectName(),sKey2DDisplayStrat),
		       displaystrat_); //Read legacy data
    }

    delete par;

    const bool samedata = *this == snapshot;
    if ( !wasvalid && !samedata )
	setValid( true );
    setModified( wasvalid && !samedata );
}


void Well::DisplayProperties::fillPar( IOPar& iop ) const
{
    if ( !isValid() && !isModified() )
	return;

    BufferString compkey( IOPar::compKey(subjectName(),track_.subjectName()) );
    iop.removeWithKey( IOPar::compKey(compkey,sKeyTrackNmSize) );
    compkey.set( IOPar::compKey(subjectName(),markers_.subjectName()) );
    iop.removeWithKey( IOPar::compKey(compkey,sKeyMarkerNmSize) );

    IOPar par;
    track_.fillPar( par );
    markers_.fillPar( par );
    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	IOPar tmpiop;
	logs_[idx]->left_.fillLeftPar( tmpiop );
	logs_[idx]->center_.fillCenterPar( tmpiop );
	logs_[idx]->right_.fillRightPar( tmpiop );
	par.mergeComp( tmpiop, idx ? toString( idx ) : "" );
	//keeps compatibility with former versions
    }
    if ( is2D() )
	par.setYN( sKey2DDisplayStrat,displaystrat_ );

    iop.mergeComp( par, subjectName() );
}


void Well::DisplayProperties::ensureColorContrastWith( OD::Color bkcol )
{
    const TypeSet<OD::Color> usecols = bkcol.complimentaryColors( 2 );
    if ( usecols.size() < 2 )
	return;

    bool modified = false;
    if ( bkcol.contrast(track_.getColor())<4.5 )
    {
	modified = modified || track_.getColor() != usecols[1];
	track_.setColor( usecols[1] );
    }
    if ( markers_.issinglecol_ && bkcol.contrast(markers_.getColor())<4.5 )
    {
	modified = modified || markers_.color_ != usecols[1];
	markers_.setColor( usecols[1] );
    }

    for ( auto* logpanel : logs_ )
    {
	ObjectSet<Log> logs( &logpanel->left_, &logpanel->center_,
			     &logpanel->right_ );
	for ( auto* log : logs )
	{
	    if ( log->name_ != sKey::None() &&
		 bkcol.contrast(log->getColor())<4.5)
	    {
		modified = modified || log->getColor() != usecols[0];
		log->setColor( usecols[0] );
	    }
	}
    }

    if ( modified )
	setModified( true );
}


void Well::DisplayProperties::setTrack( const Track& track )
{
    const bool modified = track != track_;
    if ( modified )
    {
	track_ = track;
	setModified( true );
    }
}


void Well::DisplayProperties::setMarkers( const Data* wd,
					  const Markers& markers )
{
    Markers newmarkers( markers );
    if ( wd )
    {
	BufferStringSet markernms;
	Well::Man::getMarkersByID( wd->multiID(), markernms );
	newmarkers.adjustSelection( markernms );
    }

    const bool modified = newmarkers != markers_;
    if ( modified )
    {
	markers_ = newmarkers;
	setModified( true );
    }
}


void Well::DisplayProperties::setMarkerNames( const BufferStringSet& nms,
					     bool issel )
{
    const BufferStringSet markernms = markers_.markerNms( issel );
    if ( nms != markernms )
    {
	markers_.setMarkerNms( nms, issel );
	setModified( true );
    }
}


void Well::DisplayProperties::setLeftLog( const Data* wd, const Log& log,
					  int panelidx, bool forceifmissing )
{
    if ( !logs_.validIdx(panelidx) )
	return;

    const bool modified = logs_[panelidx]->left_ != log;
    if ( modified )
    {
	logs_[panelidx]->left_.setTo( wd, log, forceifmissing );
	setModified( true );
    }
}


void Well::DisplayProperties::setCenterLog( const Data* wd, const Log& log,
					    int panelidx, bool forceifmissing )
{
    if ( !logs_.validIdx(panelidx) )
	return;

    const bool modified = logs_[panelidx]->center_ != log;
    if ( modified )
    {
	logs_[panelidx]->center_.setTo( wd, log, forceifmissing );
	setModified( true );
    }
}


void Well::DisplayProperties::setRightLog( const Data* wd, const Log& log,
					   int panelidx, bool forceifmissing )
{
    if ( !logs_.validIdx(panelidx) )
	return;

    const bool modified = logs_[panelidx]->right_ != log;
    if ( modified )
    {
	logs_[panelidx]->right_.setTo( wd, log, forceifmissing );
	setModified( true );
    }
}


void Well::DisplayProperties::setDisplayStrat( bool yn )
{
    const bool modified = displaystrat_ != yn;
    if ( modified )
    {
	displaystrat_ = yn;
	setModified( true );
    }
}


bool Well::DisplayProperties::isValidLogPanel( int idx ) const
{
    return logs_.validIdx( idx );
}


const Well::DisplayProperties::LogCouple&
    Well::DisplayProperties::getLogs( int panel ) const
{
#ifdef __debug__
    if ( !isValidLogPanel(panel) )
    {
	pErrMsg("Invalid idx for log panel");
	DBG::forceCrash(false);
    }
#endif
    return *logs_[panel];
}


void Well::DisplayProperties::ensureNrPanels( int nrpanels )
{
    if ( getNrLogPanels() >= nrpanels )
	return;

    setModified( true );
    for ( int idx=getNrLogPanels(); idx<nrpanels; idx++ )
	logs_ += new LogCouple();
}


Well::DisplayProperties& Well::DisplayProperties::defaults()
{
    mDefineStaticLocalObject( PtrMan<Well::DisplayProperties>, ret, = nullptr );

    if ( !ret )
    {
	const Settings& setts = Settings::fetch( "welldisp" );
	auto* newret = new DisplayProperties;
	newret->usePar( setts );

	ret.setIfNull(newret,true);
    }

    return *ret;
}


void Well::DisplayProperties::commitDefaults()
{
    Settings& setts = Settings::fetch( "welldisp" );
    const Well::DisplayProperties& defs = defaults();
    defs.fillPar( setts );
    const BufferString key1( defs.subjectName() );
    const BufferString key2( IOPar::compKey( defs.markers_.subjectName(),
					     sKeyMarkerSelected ) );
    const BufferString key3( IOPar::compKey( defs.markers_.subjectName(),
					     sKeyMarkerUnselected ) );
    setts.removeWithKey( IOPar::compKey(key1,key2) );
    setts.removeWithKey( IOPar::compKey(key1,key3) );
    setts.write( false );
}
