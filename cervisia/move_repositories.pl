#!/usr/bin/perl

$DEBUG = 0;

while(<>)
{
    if( /\[(Repository-.*)\]/ )
    {
        # remember group section
        $section = $1;
        print "[$section]\n";
        next;
    }
    if( /\[(.*)\]/ )
    {
        # clear section variable for other groups
        $section = "";
        next;
    }
    if( /^Compression=(.*)$/ )
    {
        # skip if not in repository group
        if( $section eq "" )
        {
            next;
        }

        print STDERR "\n[$section]Compression=$1\n" if ( $DEBUG );
        print "Compression=$1\n";
    }
    if( /^rsh=(.*)$/ )
    {
        # skip if not in repository group
        if( $section eq "" )
        {
            next;
        }

        print STDERR "\n[$section]rsh=$1\n" if ( $DEBUG );
        print "rsh=$1\n";
    }
}
