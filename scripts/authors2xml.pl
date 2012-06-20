#!/usr/bin/perl -w
# Extract author information from C++ files
# and print it out in DocBook format as a list
# Daniel Naber <daniel.naber@t-online.de>
# $Id: authors2xml.pl 439841 2005-07-29 02:48:45Z mueller $

my $file = $ARGV[0];
if( ! $file ) {
	print "Usage: $0 <file.cpp>\n";
	exit;
}

open(IN, $file) || die "Cannot open '$file': $!\n";
undef $/;
my $str = (<IN>);
close(IN);

print "<itemizedlist>\n";
while( $str =~ m/addAuthor\s*\(\s*"(.*?)",\s*.*?,\s*"(.*?)"/gs ) {
	my ($name, $email) = ($1, $2);
	print "<listitem><para>$name <email>$email</email></para></listitem>\n";
	#print "$name, $email\n";
}
print "</itemizedlist>\n";

print STDERR "Warning: maybe you need to fix umlauts manually...\n";
exit;
