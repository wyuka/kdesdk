#!/usr/bin/perl

# (copied from kdesdk/cervisia/misc.cpp)
# These regular expression parts aren't useful to check the validity of the
# CVSROOT specification. They are just used to extract the different parts of it.
$usernamerx = "([a-z0-9_][a-z0-9_-]*)?";
$passwordrx = "(:[^@]+)?";
$hostrx     = "([^:/]+)";
$portrx     = "(:(\\d*))?";
$pathrx     = "(/.*)";

# concat above regexps into a single expression
$regexp = join('', ":pserver:(", $usernamerx, $passwordrx, "@)?", $hostrx, $portrx, $pathrx);

$loginuser = getlogin || getpwuid($<);

while(<>)
{
    # skip empty lines
    next if /^$/;

    # config group for a repository?
    if( /^\[Repository-(.+)\]$/ )
    {
        $oldcvsroot = $1;

        # pserver CVSROOT specification?
        if( $oldcvsroot =~ m/($regexp)/ )
        {
            # extract username, hostname, port and path from CVSROOT
            $username = $3;
            $hostname = $5;
            $port     = $7;
            $path     = $8;

            # replace empty port number
            $port =~ s/^$/2401/;

            # replace empty username
            $username =~ s/^$/$loginuser/;

            # create normalized CVSROOT specification
            $newcvsroot = join('', ":pserver:", $username, "@", $hostname, ":", $port, $path);

            print "# DELETEGROUP [Repository-$oldcvsroot]\n";
            print "[Repository-$newcvsroot]\n";
        }

        next;
    }

    print $_;
}
