/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

#include "testprog.h"
#include "uigraphicsviewbase.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uidialog.h"
#include "uimain.h"
#include "uistrings.h"


class uiCanvasDrawTester : public uiGraphicsViewBase
{
public:

uiCanvasDrawTester( uiParent* p )
    : uiGraphicsViewBase(p,"uiGraphicsViewBase tester")
    , grp_(*new uiGraphicsItemGroup)
{
    scene().addItem( &grp_ );
    reSize.notify( mCB(this,uiCanvasDrawTester,reSizeCB) );
}

void reSizeCB( CallBacker* )
{
    reDraw();
}

uiLineItem* mkLine( int ix0, int iy0, int ix1, int iy1 )
{
    uiLineItem* li = new uiLineItem( ix0, iy0, ix1, iy1 );
    li->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,1) );
    li->setPenColor( Color(0,255,255) );
    return li;
}

uiTextItem* mkText( const char* txt, int x, int y )
{
    uiTextItem* ti = new uiTextItem;
    ti->setText( toUiString(txt) );
    ti->setTextColor( Color(0,0,0) );
    ti->setPos( uiPoint(x,y) );
    return ti;
}

void reDraw()
{
    grp_.removeAll( true );

    // rect at 4 pix from outside
    const int distfrom = 4;
    const int xmax = scene().nrPixX() - 1;
    const int ymax = scene().nrPixY() - 1;
    const int x0 = distfrom;
    const int x1 = xmax - distfrom;
    const int y0 = distfrom;
    const int y1 = ymax - distfrom;
    grp_.add( mkLine( x0, y0, x0, y1 ) );
    grp_.add( mkLine( x0, y1, x1, y1 ) );
    grp_.add( mkLine( x1, y1, x1, y0 ) );
    grp_.add( mkLine( x1, y0, x0, y0 ) );

    // vertical line at 100 from right
    uiLineItem* li = mkLine( xmax-100, 1, xmax-100, ymax-1 );
    li->setPenColor( Color(0,255,0) );
    grp_.add( li );

    // Text item:
    uiTextItem* ti = mkText( "Text at (100,110), Point at (90,95)", 100, 110 );
    grp_.add( ti );

    uiMarkerItem* mi = new uiMarkerItem( false );
    mi->setPos( uiPoint(90,95) );
    grp_.add( mi );
}

    uiGraphicsItemGroup&    grp_;

};

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    uiMain app;

    PtrMan<uiDialog> dlg =
	    new uiDialog( 0, uiDialog::Setup(uiStrings::sInformation(),
			mNoDlgTitle,mNoHelpKey) );
    PtrMan<uiCanvasDrawTester> tstr = new uiCanvasDrawTester( dlg );
    tstr->setViewSize( 600, 400 );
    app.setTopLevel( dlg );
    dlg->go();

    return app.exec();
}
