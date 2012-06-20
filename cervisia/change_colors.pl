#!/usr/bin/perl


while(<>)
{
    ($key)   = ($_ =~ /([^=]*)=(.*)$/);
    ($value) = ($_ =~ /^[^=]*=(.*)$/);

    if( $key eq "Conflict" and $value eq "255,100,100" )
    {
        print "# DELETE " . $key . "\n";
        print $key . "=255,130,130\n";
        next;
    }

    if( $key eq "LocalChange" and $value eq "190,190,237" )
    {
        print "# DELETE " . $key . "\n";
        print $key . "=130,130,255\n";
        next;
    }

    if( $key eq "RemoteChange" and $value eq "255,240,190" )
    {
        print "# DELETE " . $key . "\n";
        print $key . "=70,210,70\n";
        next;
    }

    print $_;
}
