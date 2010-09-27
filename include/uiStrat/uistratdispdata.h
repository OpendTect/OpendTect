#ifndef uistratdispdata_h
#define uistratdispdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdispdata.h,v 1.9 2010-09-27 11:05:19 cvsbruno Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "callback.h"
#include "color.h"
#include "ranges.h"
#include "bufstring.h"


namespace Strat{ class UnitRef; class NodeUnitRef; 
    		 class RefTree; class UnitRepository; }
class uiStratRefTree;
class uiListViewItem;

mClass StratDispData
{
public:
			StratDispData() {};
			~StratDispData() { eraseData(); }

    mStruct Unit
    {
			Unit(const char* nm,const Color& col)
				: name_(nm)
				, color_(col)
				, isdisplayed_(true)		 
				{}	 

	Interval<float>	zrg_;

	const BufferString name_;
	const Color	color_;
	bool		isdisplayed_;
	int		colidx_;
    };


    mStruct Column 
    {
			Column( const char* nm )
			    : name_(nm)
			    , isdisplayed_(true)
			    {}

	const BufferString name_;
	ObjectSet<Unit>	units_;

	bool		isdisplayed_;
    };

    void		eraseData() 
			{ 
			    for ( int idx=0; idx<cols_.size(); idx++ )
			    {
				cols_[idx]->units_.erase();
			    }
			    cols_.erase();
			}

    void		addCol( Column* col )
			    { cols_ += col; }
    void		addUnit( int colidx, Unit* un )
				    { cols_[colidx]->units_ += un; }

    int 		nrCols() const 
			    { return cols_.size(); }
    int			nrUnits( int colidx ) const 
			    { return cols_[colidx]->units_.size(); }
    const Unit*		getUnit( int colidx, int uidx ) const 
			    { return gtUnit( colidx, uidx ); }
    Unit*		getUnit( int colidx, int uidx ) 
			    { return gtUnit( colidx, uidx ); }
    const Column*	getCol( int idx ) const 
			    { return cols_[idx]; }
    Column*		getCol( int idx ) 
			    { return cols_[idx]; }

    int 		nrDisplayedCols() const				
			{
			    int nr = 0;
			    for ( int idx=0; idx<cols_.size(); idx++)
				{ if ( cols_[idx]->isdisplayed_ ) nr++; }
			    return nr;
			}
protected :

    Unit*		gtUnit( int colidx, int uidx ) const 
			    { return const_cast<Unit*>( 
					    cols_[colidx]->units_[uidx] ); }

    ObjectSet<Column> 	cols_;
};



/*!\brief used to gather all units and tied levels from a tree for display*/

mClass uiStratTreeToDispTransl : public CallBacker
{
public:
    			uiStratTreeToDispTransl(StratDispData&,Strat::RefTree&);
			~uiStratTreeToDispTransl();

    Notifier<uiStratTreeToDispTransl> newtreeRead;

protected:

    StratDispData& 	data_;
    Strat::RefTree&	tree_;

    bool 		withauxs_;
    bool 		withlevels_;

    void		addUnits(const Strat::NodeUnitRef&,int);
    void		addUnit(const Strat::UnitRef&,int);
    void		addAnnot(const char*,Interval<float>& posrg,bool);
    void		addBoundary(int,int,float);
    void		addBottomBoundary();
    void 		readFromTree();				

    void		triggerDataChange(CallBacker*);
};


/*!brief used to write directly in the listView of the uiStratRefTree as if we were handling the uiTree directly*/

mClass uiStratDispToTreeTransl : public CallBacker
{
public:
    			uiStratDispToTreeTransl(uiStratRefTree&);
	                ~uiStratDispToTreeTransl(){};

    uiListViewItem*	getItemFromTree(const char*);

    void		handleUnitMenu(const char*);
    void		handleUnitLvlMenu(const char*);

protected:

    uiStratRefTree&     uitree_;
};

#endif
