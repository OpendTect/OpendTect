/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Nov 2010
________________________________________________________________________

-*/

static const char* rcsID = "";

#include "uiimpexpselgrp.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeom.h"
#include "uilabel.h"
#include "uimsg.h"

#include "ascstream.h"
#include "bufstring.h"
#include "color.h"
#include "odver.h"
#include "separstr.h"
#include "strmprov.h"
#include "strmoper.h"
#include "timefun.h"

static const char* filefilter = "Text (*.txt *.dat)";
static const char* sKeyFileType = "CrossPlot Selection";
static const char* sKeyNrSelGrps = "Nr of Selection Groups";

uiReadSelGrp::uiReadSelGrp( uiParent* p, uiDataPointSetCrossPlotter& plotter )
    : uiDialog(p,uiDialog::Setup("Import Crossplot Selection", "","112.0.0"))
    , plotter_(plotter)
    , selgrpset_(plotter.selectionGrps())
    , y2selfld_(0)
{
    setCtrlStyle( DoAndStay );
    bool hasy2 = plotter.axisHandler(2);
    BufferStringSet nms;
    nms.add( plotter.axisHandler(0)->name() );
    nms.add( plotter.axisHandler(1)->name() );
    if ( hasy2 )
	nms.add( plotter.axisHandler(2)->name() );

    inpfld_ = new uiFileInput( this, "Input ASCII File",
	    uiFileInput::Setup(uiFileDialog::Gen)
	    .withexamine(true).forread(true).filter(filefilter) );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    inpfld_->setExamine( mCB(this,uiReadSelGrp,examineCB) );

    uiLabeledComboBox* xselfld =
	new uiLabeledComboBox( this, plotter.axisHandler(0)->name() );
    xselfld_ = xselfld->box();
    xselfld->attach( alignedBelow, inpfld_ );
    xselfld_->display( false, false );
   
    uiLabeledComboBox* yselfld =
	new uiLabeledComboBox( this, plotter.axisHandler(1)->name() );
    yselfld_ = yselfld->box();
    yselfld->attach( alignedBelow, xselfld );
    yselfld_->display( false, false );
    
    ychkfld_ = new uiCheckBox( this, "Import Y1",
	    		       mCB(this,uiReadSelGrp,fldCheckedCB) );
    ychkfld_->attach( rightTo, yselfld );
    ychkfld_->display( false, false );
    
    if ( hasy2 )
    {
	uiLabeledComboBox* y2selfld =
	    new uiLabeledComboBox( this,plotter.axisHandler(2)->name() );
	y2selfld_ = y2selfld->box();
	y2selfld->attach( alignedBelow, yselfld );
	y2selfld_->display( false, false );
	y2chkfld_ = new uiCheckBox( this, "Import Y2",
				    mCB(this,uiReadSelGrp,fldCheckedCB) );
	y2chkfld_->attach( rightTo, y2selfld );
	y2chkfld_->display( false, false );
    }
}


void uiReadSelGrp::fldCheckedCB( CallBacker* cb )
{
    mDynamicCastGet(uiCheckBox*,chkbox,cb);
    if ( ychkfld_ == chkbox )
	yselfld_->setSensitive( ychkfld_->isChecked() );
    else if ( y2chkfld_ == chkbox )
	y2selfld_->setSensitive( y2chkfld_->isChecked() );
}


class SelGrpImporter
{
public:

SelGrpImporter( const char* fnm )
{
    sd_ = StreamProvider( fnm ).makeIStream();
    if ( !sd_.usable() )
	{ errmsg_ = "Cannot open input file"; return; }
}

~SelGrpImporter()
{
    sd_.close();
}

ObjectSet<SelectionGrp> getSelections()
{
    ObjectSet<SelectionGrp> selgrpset;
    if ( !sd_.usable() )
    {
	errmsg_ = "Stream not usable";
	return selgrpset;
    }

    ascistream astrm( *sd_.istrm, true );

    if ( !astrm.isOfFileType(sKeyFileType) )
    {
	errmsg_ = "File type does not match with Crossplot Selection";
	return selgrpset;
    }

    int nrselgrps = 0;
    IOPar par( astrm );
    
    if ( par.hasKey(sKeyNrSelGrps) )
	par.get( sKeyNrSelGrps, nrselgrps );
    if ( par.hasKey(IOPar::compKey(sKey::Attribute,"X")) )
	par.get( IOPar::compKey(sKey::Attribute,"X"), xname_ );
    if ( par.hasKey(IOPar::compKey(sKey::Attribute,"Y")) )
	par.get( IOPar::compKey(sKey::Attribute,"Y"), yname_ );
    if ( par.hasKey(IOPar::compKey(sKey::Attribute,"Y2")) )
	par.get( IOPar::compKey(sKey::Attribute,"Y2"), y2name_ );

    for ( int selidx=0; selidx < nrselgrps; selidx++ )
    {
	PtrMan<IOPar> selgrppar = par.subselect( selidx );
	if ( !selgrppar ) continue;

	SelectionGrp* selgrp = new SelectionGrp();
	selgrp->usePar( *selgrppar );
	selgrpset += selgrp;
    }

    sd_.close();

    return selgrpset;
}

    BufferString	errmsg_;
    BufferString	xname_;
    BufferString	yname_;
    BufferString	y2name_;

    StreamData		sd_;
};


void uiReadSelGrp::examineCB( CallBacker* )
{
    if ( !inpfld_->fileName() )
    {
	uiMSG().error( "Please select an input file" );
	return;
    }

    SelGrpImporter imp( inpfld_->fileName() );
    ObjectSet<SelectionGrp> selgrpset = imp.getSelections();
    if ( !imp.errmsg_.isEmpty() )
	{ uiMSG().error(imp.errmsg_); return; }
    

    xname_ = imp.xname_;
    yname_ = imp.yname_;
    y2name_ = imp.y2name_;

    BufferString msg;
    msg += "Selection Group details "; msg += "\n\n";

    getInfo( selgrpset, msg );

    msg += "Do you want this selection to be imported?";

    if ( !uiMSG().askGoOn(msg.buf()) )
	return;
    
    deepCopy( selgrpset_, selgrpset );

    BufferStringSet nms;
    nms.add( xname_ );
    nms.add( yname_ );
    if ( !y2name_.isEmpty() )
	nms.add( y2name_ );
    xselfld_->addItems( nms );
    xselfld_->display( true );
    yselfld_->addItems( nms );
    yselfld_->display( true );
    yselfld_->setSensitive( true );
    
    if ( y2selfld_ )
    {
	ychkfld_->display( true );
	yselfld_->setSensitive( false );
	y2selfld_->addItems( nms );
	y2selfld_->display( true );
	y2selfld_->setSensitive( false );
	y2chkfld_->display( true );
    }
}


BufferStringSet uiReadSelGrp::getAvailableAxisNames() const
{
    BufferStringSet axisnm;
    axisnm.add( xselfld_->textOfItem(xselfld_->currentItem()) );
    if ( ychkfld_->isChecked() || !ychkfld_->isDisplayed() )
	axisnm.add( yselfld_->textOfItem(yselfld_->currentItem()) );
    if ( y2selfld_ && y2chkfld_->isChecked() )
	axisnm.add( y2selfld_->textOfItem(y2selfld_->currentItem()) );
    return axisnm;
}

#define mGetAxisVals \
    int xaxis = xselfld_->currentItem(); \
    int yaxis=-1; \
    int y2axis=-2; \
    if ( !ychkfld_->isDisplayed() || ychkfld_->isChecked() ) \
	yaxis = yselfld_->currentItem(); \
    if ( y2selfld_ ) \
    { \
	if ( y2chkfld_->isChecked() ) \
	    y2axis = y2selfld_->currentItem(); \
    }



bool uiReadSelGrp::checkSelectionArea( SelectionArea& actselarea,
				       const BufferStringSet& selaxisnm,
				       const BufferStringSet& axisnms,
				       bool hasalt )
{
    mGetAxisVals;

    if ( !selaxisnm.isPresent(axisnms.get(0)) )
	return false;

    if ( yaxis<0 && y2axis>=0 )
    {
	if ( (axisnms.validIdx(2) && !selaxisnm.isPresent(axisnms.get(2))) ||
   	     !selaxisnm.isPresent(axisnms.get(1))  )
	    return false;

	actselarea.axistype_ = SelectionArea::Y2;
    }
    else if ( yaxis>=0 && y2axis<0 )
    {
	if ( !selaxisnm.isPresent(axisnms.get(1)) )
	    return false;

	actselarea.axistype_ = SelectionArea::Y1;
    }
    else if ( yaxis>=0 && y2axis>=0 )
    {
	actselarea.axistype_ = SelectionArea::Both;
	if ( !hasalt )
	{
	    if ( !selaxisnm.isPresent(axisnms.get(1)) )
		actselarea.axistype_ = SelectionArea::Y2;
	    else if ( !selaxisnm.isPresent(axisnms.get(2)) )
		actselarea.axistype_ = SelectionArea::Y1;
	}
    }

    return true;
}


void uiReadSelGrp::fillRectangle( const SelectionArea& selarea,
				  SelectionArea& actselarea )
{
    mGetAxisVals;
    bool hasalt = !selarea.altyaxisnm_.isEmpty();
    if ( xaxis == 0 )
    {
	actselarea.worldrect_ =
	    ((yaxis == 2) && hasalt) ? selarea.altworldrect_
			 : selarea.worldrect_;
	actselarea.altworldrect_ =
	    ((yaxis == 2) && hasalt) ? selarea.worldrect_
			 : selarea.altworldrect_;
    }
    else 
    {
	const bool xis1 = xaxis == 1;
	const bool yis0 = yaxis == 0;
       uiWorldRect rect = selarea.worldrect_;
       uiWorldRect altrect = hasalt ? selarea.altworldrect_
				    : selarea.worldrect_;
       TypeSet<double> ltptval;
       ltptval += rect.topLeft().x;
       ltptval += rect.topLeft().y;
       ltptval += altrect.topLeft().y;
       
       TypeSet<double> rbptval;
       rbptval += rect.bottomRight().x;
       rbptval += rect.bottomRight().y;
       rbptval += altrect.bottomRight().y;

       const bool onlyy2 = actselarea.axistype_ == SelectionArea::Y2;
       const int yaxisnr = (yaxis<0 || onlyy2) ? y2axis : yaxis;

       actselarea.worldrect_ =
	   uiWorldRect( ltptval[xaxis], ltptval[yaxisnr],
		        rbptval[xaxis], rbptval[yaxisnr] );
       actselarea.worldrect_.checkCorners( true, false );
       if (hasalt && actselarea.axistype_==SelectionArea::Both)
       {
	   actselarea.altworldrect_ =
	       uiWorldRect( ltptval[xaxis], ltptval[y2axis],
			    rbptval[xaxis], rbptval[y2axis] );
	   actselarea.altworldrect_.checkCorners( true, false );
       }
    }
}


void uiReadSelGrp::fillPolygon( const SelectionArea& selarea,
			        SelectionArea& actselarea )
{
    mGetAxisVals;
    bool hasalt = !selarea.altyaxisnm_.isEmpty();
 
    if ( xaxis == 0 )
    {
	actselarea.worldpoly_ = ((yaxis) == 2 && hasalt)
	    ? selarea.altworldpoly_ : selarea.worldpoly_;
	actselarea.altworldpoly_ = ((yaxis == 2) && hasalt)
	    ? selarea.worldpoly_ : selarea.altworldpoly_;
    }
    else 
    {
	const bool xis1 = xaxis == 1;
	const bool yis0 = yaxis == 0;
       ODPolygon<double> worldpoly,altworldpoly;
       TypeSet< Geom::Point2D<double> > pts = selarea.worldpoly_.data();
       TypeSet< Geom::Point2D<double> > altpts =
				   hasalt ? selarea.altworldpoly_.data()
					  : selarea.worldpoly_.data();
       for ( int idx=0; idx<pts.size(); idx++ )
       {
	   TypeSet<double> ptval;
	   ptval += pts[idx].x; ptval += pts[idx].y;
	   ptval += altpts[idx].y;
	   const bool onlyy2 = actselarea.axistype_ == SelectionArea::Y2;
	   const int yaxisnr = (yaxis<0 || onlyy2) ? y2axis : yaxis;
	   
	   worldpoly.add( Geom::Point2D<double>(ptval[xaxis], ptval[yaxisnr]) );
	   
	   if (hasalt && actselarea.axistype_==SelectionArea::Both)
	       altworldpoly.add( Geom::Point2D<double>(ptval[xaxis],
				 ptval[y2axis]) );
       }

       actselarea.worldpoly_ = worldpoly;
       actselarea.altworldpoly_ = altworldpoly;
    }
}


bool uiReadSelGrp::adjustSelectionGrps()
{
    mGetAxisVals;
    if ( xaxis<0 || (yaxis<0 && y2axis<0) )
    {
	uiMSG().error( "Can't import selection group" );
	return false;
    }

    if ( xaxis==yaxis || yaxis==y2axis || y2axis==xaxis )
    {
	uiMSG().error( "Choose separate axis for different Axis" );
	return false;
    }

    BufferStringSet axisnms = getAvailableAxisNames();

    int selareaid = 0;
    bool selimportfailed = false;
    for ( int selidx=0; selidx<selgrpset_.size(); selidx++ )
    {
	SelectionGrp* selgrp = selgrpset_[selidx];

	SelectionGrp* newselgrp =
	    new SelectionGrp( selgrp->name(), selgrp->col_ );
	for ( int idx=0; idx<selgrp->size(); idx++ )
	{
	    const SelectionArea& selarea = selgrp->getSelectionArea( idx );
	    SelectionArea actselarea = SelectionArea( selarea.isrectangle_ );
	    bool hasalt = !selarea.altyaxisnm_.isEmpty();

	    if ( !checkSelectionArea(actselarea,selarea.getAxisNames(),
				     axisnms,hasalt) )
	    {
		selimportfailed = true;
		continue;
	    }
	   
	    if ( selarea.isrectangle_ )
		fillRectangle( selarea, actselarea );
	    else
		fillPolygon( selarea, actselarea );

	    if ( plotter_.checkSelArea(actselarea) )
	    {
		actselarea.id_ = selareaid;
		newselgrp->addSelection( actselarea );
		selareaid++;
	    }
	    else
		selimportfailed = true;
	}

	delete selgrpset_.replace( selgrpset_.indexOf(selgrp), newselgrp );
    }

    if ( selimportfailed )
	uiMSG().message( "Some selectionareas could not be imported \n"
			 "as they fall outside the value ranges of the plot" );

    return true;
}


void uiReadSelGrp::getInfo( const ObjectSet<SelectionGrp>& selgrps,
			    BufferString& msg )
{
    for ( int selgrpidx=0; selgrpidx<selgrps.size(); selgrpidx++ )
    {
	const SelectionGrp* selgrp = selgrps[selgrpidx];
	msg += "Selection Group Name :";
	msg += selgrp->name();
	msg += "\n";

	Interval<double> range( mUdf(double), -mUdf(double) );
	
	for ( int idx=0; idx<selgrp->size(); idx++ )
	{
	    const SelectionArea& selarea = selgrp->getSelectionArea( idx );
	    BufferStringSet axisnms = selarea.getAxisNames();
	 
	    msg += "Area Nr "; msg.add( idx ); msg += "\n";
	    
	    msg += selarea.xaxisnm_; msg += " (range) :";
	    range = selarea.getValueRange( true );
	    msg .add( range.start ); msg += ", "; msg.add( range.stop );
	    msg += "\n";

	    msg += selarea.yaxisnm_; msg += " (range) :";
	    range = selarea.getValueRange(false);
	    msg .add( range.start ); msg += ", "; msg.add( range.stop );
	    msg += "\n";
	    
	    if ( !selarea.altyaxisnm_.isEmpty() )
	    {
		msg += selarea.altyaxisnm_; msg += " (range) :";
		range = selarea.getValueRange( false, true );
		msg .add( range.start ); msg += ", "; msg.add( range.stop);
		msg += "\n";
	    }
	    
	    msg += "\n";
	}

	msg += "\n";
    }
}


bool uiReadSelGrp::acceptOK( CallBacker* )
{
    if ( !adjustSelectionGrps() )
	return false;
    plotter_.reDrawSelections();
    return true;
}


uiExpSelectionArea::uiExpSelectionArea( uiParent* p,
					const ObjectSet<SelectionGrp>& selgrps,
					uiExpSelectionArea::Setup setup )
    : uiDialog(p,uiDialog::Setup("Export Selection Area",
				 "Specify parameters",mTODOHelpID))
    , selgrps_(selgrps)
    , setup_(setup)
{
    setCtrlStyle( DoAndStay );

    outfld_ = new uiFileInput( this, "Output File",
			uiFileInput::Setup(uiFileDialog::Gen)
			.withexamine(false).forread(false).filter(filefilter) );
    outfld_->setSelectMode( uiFileDialog::AnyFile );
}


class SelGrpExporter 
{
public:

SelGrpExporter( const char* fnm )
{
    sd_ = StreamProvider( fnm ).makeOStream();
    if ( !sd_.usable() )
	{ errmsg_ = "Cannot open output file"; return; }
}

~SelGrpExporter()
{
    sd_.close();
}

bool putSelections( const ObjectSet<SelectionGrp>& selgrps, const char* xname,
		    const char* yname, const char* y2name )
{
    if ( !sd_.usable() ) return false;

    ascostream ostrm( *sd_.ostrm ); 
    std::ostream& strm = ostrm.stream();

    if ( !selgrps.size() )
    {
	errmsg_ = "No selections found";
	return false;
    }

    IOPar selectionpar;
    selectionpar.set( IOPar::compKey(sKey::Attribute,"X"), xname );
    selectionpar.set( IOPar::compKey(sKey::Attribute,"Y"), yname );
    if ( y2name )
	selectionpar.set( IOPar::compKey(sKey::Attribute,"Y2"), y2name );
    selectionpar.set( sKeyNrSelGrps, selgrps.size() );

    for ( int selidx=0; selidx<selgrps.size(); selidx++ )
    {
	const SelectionGrp* selgrp = selgrps[selidx];
	IOPar par,selgrppar;
	BufferString selstr;
	selstr.add( selidx );
	selgrp->fillPar( par );
	selgrppar.mergeComp( par, selstr.buf() );
	selectionpar.merge( selgrppar );
    }

    selectionpar.write( ostrm.stream(), sKeyFileType );
    const bool ret = strm.good();
    if ( !ret )
	errmsg_ = "Error during write";
    sd_.close();
    return ret;
}

    BufferString	errmsg_;
    StreamData		sd_;

};


bool uiExpSelectionArea::acceptOK( CallBacker* )
{
    SelGrpExporter exp( outfld_->fileName() );
    BufferString yaxisnm;
    if ( !exp.putSelections(selgrps_,setup_.xname_,
			    setup_.yname_,setup_.y2name_) )
	{ uiMSG().error(exp.errmsg_); return false; }

    uiMSG().message( "Output file created" );
    return false;
}
