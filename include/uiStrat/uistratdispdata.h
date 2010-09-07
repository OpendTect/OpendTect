#ifndef uistratdispdata_h
#define uistratdispdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdispdata.h,v 1.8 2010-09-07 16:03:06 cvsbruno Exp $
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

mClass AnnotData
{
public:
			AnnotData() {};
			~AnnotData() { eraseData(); };

    mStruct Annot
    {
			Annot( const char* nm, float pos )
			    : zpos_(pos)
			    , name_(nm)
			    , colidx_(0)
			    , draw_(true)	
			    {}

	float 		zpos_;
	Color 		col_;
	Color 		nmcol_;
	BufferString	name_;
	int		id_;
	int 		colidx_;	
	bool		draw_;
    };

    mStruct Marker : public Annot
    {
			Marker( const char* nm, float pos )
			    : Annot(nm,pos)
			    , isdotted_(false)	
			    {}

	bool		isdotted_;
    };

    mStruct Unit : public Annot
    {
			Unit(const char* nm,float zpostop,float zposbot)
			    : Annot(nm,zpostop)
			    , zposbot_(zposbot)
			    {}

	float 		zposbot_;
	BufferStringSet annots_;
    };
    
    mStruct Column 
    {
			Column( const char* nm )
			    : name_(nm)
			    , iseditable_(true) 
			    , isdisplayed_(true)	       	
			    , isaux_(false)	       
			    {}

	BufferString	name_;
	bool		iseditable_;
	bool		isdisplayed_;
	bool		isaux_;
	ObjectSet<Marker> markers_;  
	ObjectSet<Unit>	units_; 
    };

    void		eraseData() 
			{ 
			    for ( int idx=0; idx<columns_.size(); idx++ )
			    {
				deepErase( columns_[idx]->markers_ );
				deepErase( columns_[idx]->units_ );
			    }
			    deepErase( columns_ );
			}
    const Column*	getCol( int idx ) const { return columns_[idx]; }
    Column*		getCol( int idx ) 	{ return columns_[idx]; }
    int			nrCols() const 		{ return columns_.size(); }
    int			nrDisplayedCols() const
			{
			    int nrcols = 0;
			    for ( int idx=0; idx<columns_.size(); idx++ )
				nrcols += columns_[idx]->isdisplayed_ ? 1 : 0;
			    return nrcols;
			}    
    void		addCol( Column* col) 	{ columns_ += col; }
    
protected:

    ObjectSet<Column> 	columns_;  
};



/*!\brief used to gather all units and tied levels from a tree for display*/

mClass uiStratTreeToDispTransl : public CallBacker
{
public:
	                        uiStratTreeToDispTransl(AnnotData&);
	                        ~uiStratTreeToDispTransl();

    Notifier<uiStratTreeToDispTransl> newtreeRead;

    float		botzpos_;
    int			botlvlid_;

protected:

    const Strat::UnitRepository& unitrepos_;
    AnnotData& 		data_;

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

    uiListViewItem*		getItemFromTree(const char*);
    void			handleUnitMenu(const char*);
    void			handleUnitLvlMenu(int);
    void 			fillUndef(CallBacker*);

protected:

    uiStratRefTree&     	uitree_;
};

#endif
