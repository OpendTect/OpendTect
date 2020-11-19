<?php
include_once( "dtect/arcanist/dgbarcanist/linter/TabTools.php");

$quiet = false;

if ( count( $argv ) >1 && $argv[1]=="--quiet" )
    $quiet = true;

function testExpand( $input, $expected )
{
    global $quiet;
    $desc = "Expansion of \"$input\" - ";
    if ( !$quiet )
	echo $desc;

    if ( tab_expand( $input ) != $expected )
    {
	if ( $quiet )
	    echo $desc;
	echo "FAILED\n";

	print_r( $expected );
	print_r( tab_expand( $input ) );

	return false;
    }
    else
    {
	if ( !$quiet ) echo "SUCCESS\n";
    }

    $desc = "Collapse of \"$expected\" - ";
    if ( !$quiet )
	echo $desc;

    if ( tab_collapse( $expected,8) != $input )
    {
	if ( $quiet )
	    echo $desc;
	echo "FAILED\n";

	return false;
    }
    else
    {
	if ( !$quiet ) echo "SUCCESS\n";
    }

    return true;
}


//000000011111111112222222222333333333344444444445
//345678901234567890123456789012345678901234567890
//	|	|	|	|	|	|        
if ( !testExpand(      "abc\tdef",
		       "abc     def" ) )
    exit( 1 );
//	|	|	|	|	|	|        
if ( !testExpand(      "\tdef",
		       "        def" ) )
    exit( 1 );
//	|	|	|	|	|	|        
if ( !testExpand(      "12345678\tdef",
		       "12345678        def" ) )
    exit( 1 );

if ( !testExpand(      "12345678\n".
		       "\tdef",
		       "12345678\n".
		       "        def" ) )
    exit( 1 );

exit( 0 );

?>

