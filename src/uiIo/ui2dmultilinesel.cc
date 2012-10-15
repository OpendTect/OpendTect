/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Sep 2012
-*/

static const char* rcsID mUsedVar = "$Id: ui2dmultilinesel.cc,v 1.24 2012/09/12 10:59:03 cvsraman Exp $";

#include "ui2dmultilinesel.h"

#include "callback.h"
#include "uibutton.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "uicombobox.h"
#include "uilistbox.h"
#include "uiselsurvranges.h"


ui2DMultiLineSel::ui2DMultiLineSel( uiParent* p, Setup su )
    : uiCompoundParSel( p, "Lineset", "Select" )
    , setup_(su)
{
    dlg_ = new  ui2DMultiLineSelDlg( this, setup_ );
    butPush.notify( mCB(this,ui2DMultiLineSel,openDlg) );
}


ui2DMultiLineSel::~ui2DMultiLineSel()
{ delete dlg_; }


void ui2DMultiLineSel::openDlg( CallBacker* )
{
    dlg_->fillDlg();
    if ( !dlg_->go() )
		return;

    lineinfo_ = dlg_->getLineInfo();
    geomids_ = dlg_->getSelLinesGeomIds();

}


BufferString ui2DMultiLineSel::getSummary() const
{
    BufferString ret;
    int alllines = 0;
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	alllines++;
	if ( (idx+1) != geomids_.size() ? 
	    geomids_[idx].lsid_ != geomids_[idx+1].lsid_ : true )
	{
	    if ( ret.size() != 0) 
			ret += ", ";

	    ret += S2DPOS().getLineSet( geomids_[idx].lsid_ );
	    BufferStringSet lines;
	    S2DPOS().getLines( lines, geomids_[idx].lsid_ );
	    ret += " (";
	    if ( alllines == 1 )
		ret += " (1 Line)";
	    else
	    {
		if ( alllines == lines.size() )
		    ret += "all";
		else
		    ret += alllines;

		ret += " Lines)";
	    }

	    alllines = 0;
	}
    }

    if ( geomids_.size() == 0 )
    {
		BufferStringSet lsnms;
		S2DPOS().getLineSets( lsnms );
		ret = lsnms.get(0);
		ret += " ( 0 Line)";
    }

    return ret;
}

const char* ui2DMultiLineSel::getLineSet()  // TODO return multi linesets
{
    if (geomids_.size() < 1)
		return 0;

    return S2DPOS().getLineSet( geomids_[0].lsid_ );
}


BufferStringSet ui2DMultiLineSel::getSelLines() const
{
    int lineidx;
    BufferStringSet sellines;
    S2DPOS().setCurLineSet( geomids_[0].lsid_ );
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
		sellines.add( S2DPOS().getLineName(geomids_[idx].lineid_) );
		if ( (idx+1) != geomids_.size() )
			if ( geomids_[idx].lsid_ != geomids_[idx+1].lsid_ )
				S2DPOS().setCurLineSet( geomids_[idx+1].lsid_ );

    }

    return sellines;
}


bool ui2DMultiLineSel::fillPar(IOPar& iop) const
{
    return dlg_->fillPar( iop );
}


void ui2DMultiLineSel::usePar( const IOPar& iop )
{
    dlg_->usePar( iop );
}


ui2DMultiLineSelDlg::ui2DMultiLineSelDlg( uiParent* p, 
				const ui2DMultiLineSel::Setup& su )
    : uiDialog( p, uiDialog::Setup("Select 2D Lines",mNoDlgTitle,"103.1.13") )
    , setup_(su)
{
    createDlg();
}


ui2DMultiLineSelDlg::~ui2DMultiLineSelDlg()
{
    deepErase ( lineinfo_ );
}


void ui2DMultiLineSelDlg::createDlg()
{
    if ( setup_.allowmultilineset_ )
    {
        llb_ = new uiLabeledListBox( this, "Line Sets", false );
		lsnmsfld_ = llb_->box();
		lsnmsfld_->selectionChanged.notify
							( mCB(this,ui2DMultiLineSelDlg,multiLineSetSel) );
		lsnmsfld_->itemChecked.notify
							( mCB(this,ui2DMultiLineSelDlg,multiLineSetCheck) );
    }
    else
    {
		lsnmfld_ = new uiComboBox( this, "Line Set" );
		lsnmfld_->selectionChanged.notify( 
									mCB(this,ui2DMultiLineSelDlg,lineSetSel) );
    }

    if ( !setup_.allowmultiline_ )
    {
		lnmfld_ = new uiComboBox(this, "Line Name" );
		lnmfld_->attach( alignedBelow, lsnmfld_ );
		lnmfld_->selectionChanged.notify( 
										mCB(this,ui2DMultiLineSelDlg,lineSel) );
	
    }
    else
    {
		llb2_ = new uiLabeledListBox( this, "Line names", false);
		lnmsfld_ = llb2_->box();
		if( setup_.allowmultilineset_ )
			llb2_->attach( alignedBelow, llb_ );
		else
			llb2_->attach( alignedBelow, lsnmfld_ );

		lnmsfld_->setItemsCheckable( true );
		lnmsfld_->selectionChanged.notify(
								mCB(this,ui2DMultiLineSelDlg,multiLineSel) );
		lnmsfld_->itemChecked.notify
								( mCB(this,ui2DMultiLineSelDlg,multiLineSel) );
    }

    if ( setup_.withtrcrg_ )
    {
		trcrgfld_ = new uiSelNrRange(this, StepInterval<int>(), true, "Trace");
		if ( setup_.allowmultiline_ )
			trcrgfld_->attach( alignedBelow, llb2_ );
		else
			trcrgfld_->attach( alignedBelow, lnmfld_ );
    }

    if ( setup_.withz_ )
    {
		zrgfld_ = new uiSelZRange(this, StepInterval<float>(), true, "Z Range");
		zrange_ = new uiCheckBox( this, "For all lines" );
		if ( setup_.withtrcrg_ )
			zrgfld_->attach( alignedBelow, trcrgfld_ );
		else if ( setup_.allowmultiline_ )
			zrgfld_->attach( alignedBelow, llb2_ );
		else
			zrgfld_->attach( alignedBelow, lnmfld_ );

		zrange_->attach( rightOf, zrgfld_ );
    }

}


bool ui2DMultiLineSelDlg::fillDlg()
{
    if ( selgeomids_.size() <= 0 )
        fillLineInfo();

    if ( setup_.allowmultilineset_ )
    {
		fillUIForMultiLineSet();
		fillUIForMultiLine();
    }
    else
    {
		fillUIForSingleLineSet();
		if ( setup_.allowmultiline_ )
			fillUIForMultiLine();
		else
			fillUIForSingleLine();
    }

    if ( setup_.withtrcrg_ )
		fillUIFortraceRange();

    if( setup_.withz_ )
		fillUIForzRange();

    return true;
}


void ui2DMultiLineSelDlg::getSelLines( TypeSet<PosInfo::GeomID>& gid ) const
{
    gid.erase();
    for( int idx=0; idx<lineinfo_.size(); idx++ )
		if ( lineinfo_[idx]->issel_ == true )
			gid += lineinfo_[idx]->geomid_;
}


void ui2DMultiLineSelDlg::fillLineInfo()
{
    lineinfo_.erase();
    BufferStringSet lsnms;
    S2DPOS().getLineSets( lsnms );
    for( int idx=0; idx<lsnms.size(); idx++ )
    {
		BufferStringSet lnms;
		S2DPOS().getLines( lnms, lsnms.get(idx).buf() );
		for ( int id=0; id<lnms.size(); id++ )
		{
			S2DPOS().setCurLineSet( lsnms.get(idx).buf() );
			PosInfo::Line2DData l2d( lnms.get(id).buf() );
			S2DPOS().getGeometry( l2d );
			PosInfo::GeomID gid = S2DPOS().getGeomID( lsnms.get(idx).buf(),
								  lnms.get(id).buf() );
			LineInfo* li = new LineInfo( gid, l2d.trcNrRange(), l2d.zRange() );
			lineinfo_ += li;
		}
    }

}


void ui2DMultiLineSelDlg::fillUIForMultiLineSet()
{
    BufferStringSet lsnms;
    S2DPOS().getLineSets( lsnms );
    NotifyStopper stop( lsnmsfld_->selectionChanged );
    NotifyStopper stopper( lsnmsfld_->itemChecked );
    lsnmsfld_->setEmpty();
    lsnmsfld_->addItems( lsnms );
    lsnmsfld_->setItemsCheckable(true);
    if ( selgeomids_.size() > 0 )
    {
		BufferString lsnm;
		lsnm = S2DPOS().getLineSet( selgeomids_[0].lsid_ );
		lsnmsfld_->setItemChecked( lsnm.buf(), true );
		previouslsnm_ = lsnm;
		for ( int idx=1; idx<selgeomids_.size(); idx++ )
		{
			if ( selgeomids_[idx].lsid_ != selgeomids_[idx-1].lsid_ )
			{
				lsnm = S2DPOS().getLineSet( selgeomids_[idx].lsid_ );
				lsnmsfld_->setItemChecked( lsnm.buf(), true );
			}

		}

		lsnmsfld_->setCurrentItem( previouslsnm_.buf() );
    }
    else
    {
		lsnmsfld_->setSelected( 0 );
		previouslsnm_ = lsnms.get( 0 );
    }

}


void ui2DMultiLineSelDlg::fillUIForSingleLineSet()
{
    BufferStringSet lsnms;
    S2DPOS().getLineSets( lsnms );
    NotifyStopper stop( lsnmfld_->selectionChanged );
    lsnmfld_->setEmpty();
    lsnmfld_->addItems( lsnms );
    if ( selgeomids_.size() > 0 )
    {
		lsnmfld_->setCurrentItem( S2DPOS().getLineSet(selgeomids_[0].lsid_));
		previouslsnm_ = S2DPOS().getLineSet(selgeomids_[0].lsid_);
    }
    else
    {
		lsnmfld_->setCurrentItem( 0 );
		previouslsnm_ = lsnms.get( 0 );
    }

    if ( !setup_.withlinesetsel_ )
    {
		lsnmfld_->setReadOnly( true );
		return;
    }

}


void ui2DMultiLineSelDlg::fillUIForMultiLine()
{
    BufferStringSet lnms;
    NotifyStopper stop( lnmsfld_->selectionChanged );
    NotifyStopper stopper( lnmsfld_->itemChecked );
    lnmsfld_->setEmpty();
    if ( selgeomids_.size() > 0 )
    {
		BufferString lnm;
		int test = selgeomids_[0].lsid_; ////debug
		S2DPOS().getLines( lnms, selgeomids_[0].lsid_ );
		lnmsfld_->addItems( lnms );
		S2DPOS().setCurLineSet( S2DPOS().getLineSet(selgeomids_[0].lsid_) );
		lnm = S2DPOS().getLineName( selgeomids_[0].lineid_ );
		lnmsfld_->setCurrentItem( lnm.buf() );
		lnmsfld_->setItemChecked( lnm.buf(), true );
		previouslnm_ = lnm;
		for ( int idx=1; idx<selgeomids_.size(); idx++ )
		{
			if ( selgeomids_[idx].lsid_ == selgeomids_[idx-1].lsid_ )
			{
			lnm = S2DPOS().getLineName( selgeomids_[idx].lineid_ );
			lnmsfld_->setItemChecked( lnm, true );
			}
			else
				break;
		}
    }
    else
    {
		S2DPOS().getLines( lnms, lineinfo_[0]->geomid_.lsid_ );
		lnmsfld_->addItems( lnms );
		lnmsfld_->setSelected( 0 );
		previouslnm_ = lnms.get( 0 );
    }
}


void ui2DMultiLineSelDlg::fillUIForSingleLine()
{
    BufferStringSet lnms;
    BufferString lnm;
    NotifyStopper stop( lnmfld_->selectionChanged );
    lnmfld_->setEmpty();
    if ( selgeomids_.size() > 0 )
    {
		S2DPOS().getLines( lnms, selgeomids_[0].lsid_ );
		lnmfld_->addItems( lnms );
		S2DPOS().setCurLineSet( selgeomids_[0].lsid_ );
		lnm = S2DPOS().getLineName( selgeomids_[0].lineid_ );
		lnmfld_->setCurrentItem( lnm.buf() );
		previouslnm_ = lnm.buf();
    }
    else
    {
		S2DPOS().getLines( lnms, lineinfo_[0]->geomid_.lsid_ );
		lnmfld_->addItems( lnms );
		lnmfld_->setCurrentItem(lnms.get(0));
		previouslnm_ = lnms.get(0);
    }
}


void ui2DMultiLineSelDlg::fillUIFortraceRange()
{
    BufferString lnm;
    if( selgeomids_.size() > 0 )
    {
		int index = lineID( selgeomids_[0] );
		if ( !lineinfo_.validIdx(index) )
			return;

		trcrgfld_->setLimitRange( lineinfo_[index]->trcrange_ );
		trcrgfld_->setRange( lineinfo_[index]->seltrcrange_ );
		return;
    }
    else
    {
		trcrgfld_->setLimitRange( lineinfo_[0]->trcrange_ );
		trcrgfld_->setRange( lineinfo_[0]->trcrange_ );
    }

}


void ui2DMultiLineSelDlg::fillUIForzRange()
{
    if( selgeomids_.size() > 0 )
    {
		int index = lineID( selgeomids_[0] );
		if ( !lineinfo_.validIdx(index) )
			return;

		zrgfld_->setRangeLimits( lineinfo_[index]->zrange_ );
		zrgfld_->setRange( lineinfo_[index]->selzrange_ );
		return;
    }
    else
		zrgfld_->setRangeLimits( lineinfo_[0]->zrange_ );
		zrgfld_->setRange( lineinfo_[0]->zrange_ );
}


void ui2DMultiLineSelDlg::multiLineSetCheck( CallBacker* )
{
    BufferString lsnm;
    BufferStringSet lnms;
    lsnm = lsnmsfld_->getText();
    if ( lsnmsfld_->isItemChecked(lsnm.buf()) )
		return;

    S2DPOS().getLines( lnms, lsnmsfld_->getText() );
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
		PosInfo::GeomID gid = S2DPOS().getGeomID
											( lsnm.buf(), lnms.get(idx).buf() );
		int index = lineID( gid );
		lineinfo_[index]->issel_ = false;
		lnmsfld_->setItemChecked( lnms.get(idx), false );
    }
}



void ui2DMultiLineSelDlg::multiLineSetSel( CallBacker* )
{
    BufferString lnm;
    BufferStringSet lnms;
    S2DPOS().getLines( lnms, lsnmsfld_->getText() );
    trcRgZrgChanged();
    previouslsnm_ = lsnmsfld_->getText();
    getSelLines( selgeomids_ );
    NotifyStopper stop( lnmsfld_->selectionChanged );
    NotifyStopper stopper( lnmsfld_->itemChecked );
    NotifyStopper stopper2( trcrgfld_->rangeChanged );
    lnmsfld_->setEmpty();
    lnmsfld_->addItems( lnms );
    if ( lnms.size() == 0 )
		return;

    PosInfo::GeomID gid = S2DPOS().getGeomID
				    ( lsnmsfld_->getText(), lnms.get(0).buf() );
    S2DPOS().setCurLineSet( gid.lsid_ );
    for ( int idx=0; idx<selgeomids_.size(); idx++ )
    {
		if ( selgeomids_[idx].lsid_ == gid.lsid_ )
			lnmsfld_->setItemChecked( S2DPOS().getLineName
											(selgeomids_[idx].lineid_), true );
    }

    lnmsfld_->setSelected( 0 );
    previouslnm_ = lnms.get( 0 );
    int index = lineID( gid );
    if ( setup_.withtrcrg_ )
    {
		trcrgfld_->setLimitRange( lineinfo_[index]->trcrange_ );
		if ( lineinfo_[index]->issel_ == true )
			trcrgfld_->setRange( lineinfo_[index]->seltrcrange_ );
		else
			trcrgfld_->setRange( lineinfo_[index]->trcrange_ );
    }

    if ( setup_.withz_ ? !isZRangeForAll() : false )
    {
		if ( lineinfo_[index]->issel_ == true )
			zrgfld_->setRange( lineinfo_[index]->selzrange_ );
		else
			zrgfld_->setRange( lineinfo_[index]->zrange_ );
    }

}


void ui2DMultiLineSelDlg::lineSetSel( CallBacker* )
{
    BufferStringSet lnms;
    previouslsnm_ = lsnmfld_->textOfItem( lsnmfld_->currentItem() );
    S2DPOS().getLines( lnms, lsnmfld_->textOfItem(lsnmfld_->currentItem()) );
    if ( setup_.allowmultiline_ )
    {
		NotifyStopper stop( lnmsfld_->selectionChanged );
		NotifyStopper stopper( lnmsfld_->itemChecked );
		lnmsfld_->setEmpty();
		lnmsfld_->addItems( lnms );
		lnmsfld_->setSelected(0);
    }
    else
    {
		NotifyStopper stop( lnmfld_->selectionChanged );
		lnmfld_->setEmpty();
		lnmfld_->addItems( lnms );
		lnmfld_->setCurrentItem(0);
    }

    if ( lnms.size() == 0 )
		return;

    previouslnm_ = lnms.get( 0 );
    S2DPOS().setCurLineSet( previouslsnm_.buf() );
    PosInfo::Line2DData l2d( lnms.get(0).buf() );
    S2DPOS().getGeometry( l2d );
    if ( setup_.withtrcrg_ )
    {
		trcrgfld_->setLimitRange( l2d.trcNrRange() );
		trcrgfld_->setRange( l2d.trcNrRange() );
    }

    if ( setup_.withz_ ? !isZRangeForAll() : false )
		zrgfld_->setRange( l2d.zRange() );

}


void ui2DMultiLineSelDlg::multiLineSel( CallBacker* )
{
    BufferString lnm, lsnm;
    trcRgZrgChanged();
    if ( setup_.allowmultilineset_ )
		lsnm = lsnmsfld_->getText();
    else
		lsnm = lsnmfld_->textOfItem(lsnmfld_->currentItem());

    previouslnm_ = lnmsfld_->getText();
    lnm = lnmsfld_->getText();
    PosInfo::GeomID gid = S2DPOS().getGeomID( lsnm.buf(), lnm.buf() );
    int index = lineID( gid );
    if ( !lineinfo_.validIdx(index) )
		return;

    if( lnmsfld_->isItemChecked(lnm) )
    {
		if ( setup_.allowmultilineset_ )
		{
			NotifyStopper stop( lsnmsfld_->itemChecked );
			lsnmsfld_->setItemChecked( lsnm.buf(), true );
		}

		lineinfo_[index]->issel_ = true;
    }
    else
		lineinfo_[index]->issel_ = false;

    StepInterval<int> si( 0, 0, 1 );
    if ( setup_.withtrcrg_ )
    {
		trcrgfld_->setLimitRange( lineinfo_[index]->trcrange_ );
		if ( lineinfo_[index]->seltrcrange_ == si )
			trcrgfld_->setRange( lineinfo_[index]->trcrange_ );
		else
			trcrgfld_->setRange( lineinfo_[index]->seltrcrange_ );
    }

    if ( setup_.withz_ ? !isZRangeForAll() : false )
    {
		zrgfld_->setRangeLimits( lineinfo_[index]->zrange_ );
		if ( lineinfo_[index]->seltrcrange_ == si )
			zrgfld_->setRange( lineinfo_[index]->zrange_ );
		else
			zrgfld_->setRange( lineinfo_[index]->selzrange_ );
    }

    return;
}


void ui2DMultiLineSelDlg::lineSel( CallBacker* )
{
    BufferString lnm = lnmfld_->textOfItem( lnmfld_->currentItem() );
    previouslnm_ = lnm;
    S2DPOS().setCurLineSet( lsnmfld_->textOfItem(lsnmfld_->currentItem()) );
    PosInfo::Line2DData l2d( lnm.buf() );
    S2DPOS().getGeometry( l2d );
    if ( setup_.withtrcrg_ )
    {
		trcrgfld_->setLimitRange( l2d.trcNrRange() );
		trcrgfld_->setRange( l2d.trcNrRange() );
    }

    if ( setup_.withz_ ? !isZRangeForAll() : false )
		zrgfld_->setRange( l2d.zRange() );

}


void ui2DMultiLineSelDlg::trcRgZrgChanged()
{
    PosInfo::GeomID gid  = S2DPOS().getGeomID
				    ( previouslsnm_.buf(), previouslnm_.buf() );
    int index = lineID( gid );
    if ( !lineinfo_.validIdx(index) )
		return;

    if ( setup_.withtrcrg_ )
		lineinfo_[index]->seltrcrange_ = trcrgfld_->getRange();

    if ( setup_.withz_ )
		lineinfo_[index]->selzrange_ = zrgfld_->getRange();
}


bool ui2DMultiLineSelDlg::acceptOK( CallBacker* )
{
    trcRgZrgChanged();
    getSelLines( selgeomids_ );
    if ( setup_.allowmultiline_ && !setup_.allowmultilineset_)
    {
		selgeomids_.erase();
		BufferStringSet lnms;
		lnmsfld_->getCheckedItems( lnms );
		for( int idx=0; idx<lnms.size(); idx++ )
		{
		   selgeomids_ += S2DPOS().getGeomID
					   ( previouslsnm_.buf(), lnms.get(idx).buf() );
		}
    }

    if ( !setup_.allowmultiline_ )
    {
		selgeomids_.erase();
		selgeomids_ += S2DPOS().getGeomID
					  ( previouslsnm_.buf(), previouslnm_.buf() );
    }

    if ( isZRangeForAll() )
    {
		int lineidx;
		StepInterval<float> zrg = zrgfld_->getRange();
		for ( int idx=0; idx<selgeomids_.size(); idx++ )
		{
			lineidx = lineID( selgeomids_[idx] );
			if ( lineinfo_.validIdx(lineidx) )
			lineinfo_[lineidx]->selzrange_ = zrg;
		}
    }

    return true;
}


bool ui2DMultiLineSelDlg::fillPar( IOPar& iop ) const
{
    if ( selgeomids_.size() == 0 )
		return false;

    int lineidx;
    iop.set( "Nr Lines", selgeomids_.size() );
    for ( int idx=0; idx<selgeomids_.size(); idx++ )
    {
		lineidx = lineID( selgeomids_[idx] );
		iop.set( IOPar::compKey(sKey::Line(),idx), 
			 IOPar::compKey(sKey::ID(),selgeomids_[idx].toString()) );
		iop.set( IOPar::compKey(IOPar::compKey(sKey::Line(),idx),
				sKey::TrcRange()), lineinfo_[lineidx]->seltrcrange_ );
		iop.set( IOPar::compKey(IOPar::compKey(sKey::Line(),idx),
				sKey::ZRange()), lineinfo_[lineidx]->selzrange_ );
    }

    return true;
}


void ui2DMultiLineSelDlg::usePar(const IOPar& par)
{
    fillLineInfo();
    int totallines;
    if ( !par.get( "Nr Lines", totallines ) )
		return;

    PtrMan<IOPar> lspar = par.subselect( sKey::Line() );
    for ( int idx=0; idx<totallines; idx++ )
    {
		PtrMan<IOPar> linepar = lspar->subselect( idx );
		if ( !linepar )
			break;

		StepInterval< float > zrg;
		StepInterval< int > trcrg;
		BufferString geomid;
		if ( !linepar->get(sKey::ID(),geomid) || 
			 !linepar->get(sKey::TrcRange(),trcrg) ||
			 !linepar->get(sKey::ZRange(),zrg) )
			 continue;

		PosInfo::GeomID gid;
		gid.fromString( geomid.buf() );
		int lineidx = lineID( gid );
		lineinfo_[lineidx]->seltrcrange_ = trcrg;
		lineinfo_[lineidx]->selzrange_ = zrg;
    }

}


int ui2DMultiLineSelDlg::lineID( const PosInfo::GeomID& gid ) const
{
    for ( int idx=0; idx<lineinfo_.size(); idx++ )
    {
		if ( lineinfo_[idx]->geomid_ == gid )
			return idx;
    }
    
    return -1;
}


bool ui2DMultiLineSelDlg::isZRangeForAll() const
{ return zrange_->isChecked(); }