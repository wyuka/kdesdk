#!/usr/bin/perl
#
# Code adapted from Pod::HtmlEasy for kio_perldoc.  See CPAN for the real deal.
# Integrated and crap^H^H^H^H unnecessary dependencies removed by Michael Pyne
# <michael.pyne@kdemail.net>
#
# Copyright 2007, 2008 Michael Pyne.
# Original source is licensed to be distributed under the same terms as Perl
# itself.  All changes by Michael Pyne retain this license.
#
# Based on Pod::HtmlEasy by M. P. Graciliano and Geoffrey Leach.
# http://search.cpan.org/~gleach/Pod-HtmlEasy/

#############################################################################
## Name:        TieHandler.pm
## Purpose:     Pod::HtmlEasy::TieHandler
## Author:      Graciliano M. P.
## Modified by: Geoffrey Leach
## Created:     2/14/2007
## Copyright:   (c) 2004 Graciliano M. P.
## Licence:     This program is free software; you can redistribute it and/or
##              modify it under the same terms as Perl itself
#############################################################################

# The function of this package is to provide a print function that is
# tied to a filehandle which is then passed as the output file to
# Pod::Parser. Note that only PRINT() and CLOSE() are defined.
# PRINT() accumulates output in an anon array, which is then referenced
# by the defining function.

package Pod::HtmlEasy::TieHandler;

use strict;
use warnings;

our $VERSION = 0.02;

sub TIEHANDLE {
    my $class  = shift;
    my $scalar = shift;

    return bless $scalar, $class;
}

sub PRINT {
    my $this = shift;

    push @{$this}, @_;
    return 1;
}

sub FILENO { return 1; }
sub CLOSE  { return 1; }

#######
# END #
#######

1;

#############################################################################
## Name:        Parser.pm
## Purpose:     Pod::HtmlEasy::Parser
## Author:      Graciliano M. P.
## Modified by: Geoffrey Leach
## Created:     11/01/2004
## Updated:	    2007-02-25
## Copyright:   (c) 2004 Graciliano M. P.
## Licence:     This program is free software; you can redistribute it and/or
##              modify it under the same terms as Perl itself
#############################################################################

package Pod::HtmlEasy::Parser;

use base qw{ Pod::Parser };

use Carp;
use English qw{ -no_match_vars };
use feature "switch";

use strict;
use warnings;

our $VERSION = 0.03;

our $EMPTY = q{};
our $NL    = qq{\n};
our $NUL   = qq{\0};
our $SPACE = q{ };

# Set to 1 for URIs like /functions/foo to fixup links in evt_on_L
our $fnPage = 0;

########
# VARS #
########

my $MAIL_RE = qr{
         (         # grab all of this
         [\w-]+    # some word chars with '-' included   foo
         \0?       # possible NUL escape
         \@        # literal '@'                         @
         [\w\\-]+  # another word                        bar
         (?:       # non-grabbing pattern
          \.       # literal '.'                        .
          [\w\-\.]+# that word stuff                    stuff
          \.       # another literal '.'                .
          [\w\-]+  # another word                       and
          |        # or
          \.       # literal '.'                        .   
          [\w\-]+  # word                               nonsense
          |        # or empty?
         )        # end of non-grab
         )        # end of grab
        }smx;    # [6062]

# Treatment of embedded HTML-significant characters and embedded URIs.

# There are some characters (%html_entities below) which may in some
# circumstances be interpreted by a browser, and you probably don't want that
# Consequently, they are replaced by names defined by the W3C UNICODE spec,
# http://www.w3.org/TR/MathML2/bycodes.html, bracketed by '&' and ';'
# Thus, '>' becomes '&lt;' This is handled by _encode_entities()
# There's a "gotchya" in this process. As we are generating HTML,
# the encoding needs to take place _before_ any HTML is generated.

# If the HTML appears garbled, and UNICODE entities appear where they
# shouldn't, this encoding has happened to late at some point.

# This is all further complicated by the fact that the POD formatting
# codes syntax uses some of the same characters, as in "L<...>", for example,
# and we can't expand those first, because some of them generate
# HTML. This is resolved by tagging the characters that we want
# to distinguish from HTML with ASCII NUL ('\0', $NUL). Thus, '$lt;' becomes
# '\0&amp;' in _encode_entities().  Generated HTML is also handled
# this way by _nul_escape(). After all processing of the  POD formatting
# codes are processed, this is reversed by _remove _nul_escapes().

# Then there's the issue of embedded URIs. URIs are also generated
# by the processing of L<...>, and can show up _inside L<...>, we
# delay processing of embedded URIs until after all of the POD
# formatting codes is complete. URIs that result from that processing
# are tagged (you guessed it!) with a NUL character, but not preceeding
# the generated URI, but after the first character. These NULs are removed
# by _remove _nul_escapes()

my %html_entities = (
    q{&} => q{amp},
    q{>} => q{gt},
    q{<} => q{lt},
    q{"} => q{quot},
);

my $HTML_ENTITIES_RE = '[' . join ('', keys %html_entities) . ']';
$HTML_ENTITIES_RE = qr{$HTML_ENTITIES_RE};

#################
# _NUL_ESCAPE   #
#################

# Escape HTML-significant characters with ASCII NUL to differentiate them
# from the same characters that get converted to entity names

sub _nul_escape {
    my $txt_ref = shift;

    ${$txt_ref} =~ s{($HTML_ENTITIES_RE)}{$NUL$1}gsm;
    return;
}

#######################
# _REMOVE_NUL_ESCAPSE #
#######################

sub _remove_nul_escapes {
    my $txt_ref = shift;

    ${$txt_ref} =~ s{$NUL}{}gsm;
    return;
}

####################
# _ENCODE_ENTITIES #
####################

sub _encode_entities {
    my ( $txt_ref ) = @_;

    return unless $$txt_ref;

    foreach my $chr ( keys %html_entities ) {

        my $re = qr{(?<!$NUL)$chr};
        my $replacement = '&' . $html_entities{$chr} . ';';
        ${$txt_ref} =~ s{$re}{$NUL$replacement}gsm;
    }

    return;
}

sub setFunctionPage {
    my $this = shift;
    our $fnPage = shift;
}

###########
# COMMAND #
###########

# Overrides command() provided by base class in Pod::Parser
sub command {
    my ( $parser, $command, $paragraph, $line_num, $pod ) = @_;

    if ( defined $parser->{POD_HTMLEASY}->{VERBATIM_BUFFER} ) {
        _verbatim($parser);
    }    # [6062]

    my $expansion = $parser->interpolate( $paragraph, $line_num );

    $expansion =~ s{^\s*}{}gsm;    # delete surrounding whitespace
    $expansion =~ s{\s*$}{}gsm;    # delete surrounding whitespace

    # Encoding puts in a NUL; we're finished with the text, so remove them
    _encode_entities( \$expansion );
    _remove_nul_escapes( \$expansion );

# Create the index tag
# a_name has the text of the expansion _without_ anything between '<' and '>',
# which amounts to the HTML formatting codes, which are not processed by
# the name directive.
    my $a_name = $expansion;
    $a_name =~ s{<.*?>}{}gsm;

    $a_name =~ /</g;
    my $pos = pos($a_name);
    my $start = $pos;
    my $count = 0;
    $count++ if defined $pos;

    while(defined $pos and $count != 0)
    {
        my $match = $a_name =~ /\G[^<>]*([<>])/g;

        last unless $match;
        $pos = pos($a_name);

        $count++ if $1 eq '<';
        $count-- if $1 eq '>';
    }

    if (defined $pos and defined $start) {
        $pos = $pos - $start + 1;
        $start = $start - 1;
        $a_name =~ s/^(.{$start}).{$pos}/$1/;
    }

    my $html;
    given ($command) {
        when ("head1") {
            _add_tree_point( $parser, $expansion, 1 );
            $html = $parser->{POD_HTMLEASY}
                ->{ON_HEAD1}( $parser->{POD_HTMLEASY}, $expansion, $a_name );
        }
        when ("head2") {
            _add_tree_point( $parser, $expansion, 2 );
            $html = $parser->{POD_HTMLEASY}
                ->{ON_HEAD2}( $parser->{POD_HTMLEASY}, $expansion, $a_name );
        }
        when ("head3") {
            _add_tree_point( $parser, $expansion, 3 );
            $html = $parser->{POD_HTMLEASY}
                ->{ON_HEAD3}( $parser->{POD_HTMLEASY}, $expansion, $a_name );
        }
        when ("head4") {
            _add_tree_point( $parser, $expansion, 4 );
            $html = $parser->{POD_HTMLEASY}
                ->{ON_HEAD4}( $parser->{POD_HTMLEASY}, $expansion, $a_name );
        }
        when ("begin") {
            _add_tree_point( $parser, $expansion, 4 );
            $html = $parser->{POD_HTMLEASY}
                ->{ON_BEGIN}( $parser->{POD_HTMLEASY}, $expansion, $a_name );
        }
        when ("end") {
            $html = $parser->{POD_HTMLEASY}
                ->{ON_END}( $parser->{POD_HTMLEASY}, $expansion, $a_name );
        }
        when ("over") {
            if ( $parser->{INDEX_ITEM} ) {
                $parser->{INDEX_ITEM_LEVEL}++;
            }
            $html = $parser->{POD_HTMLEASY}
                ->{ON_OVER}( $parser->{POD_HTMLEASY}, $expansion );
        }
        when ("item") {
            if ( $parser->{INDEX_ITEM} ) {
                _add_tree_point( $parser, $expansion,
                    ( 3 + ( $parser->{INDEX_ITEM_LEVEL} || 1 ) ) );
            }
            $html = $parser->{POD_HTMLEASY}
                ->{ON_ITEM}( $parser->{POD_HTMLEASY}, $expansion, $a_name );
        }
        when ("back") {
            if ( $parser->{INDEX_ITEM} ) {
                $parser->{INDEX_ITEM_LEVEL}--;
            }
            $html = $parser->{POD_HTMLEASY}
                ->{ON_BACK}( $parser->{POD_HTMLEASY}, $expansion );
        }
        when ("for") {
            $html = $parser->{POD_HTMLEASY}
                ->{ON_FOR}( $parser->{POD_HTMLEASY}, $expansion, $a_name );
        }
        when ("include") {
            my $file = $parser->{POD_HTMLEASY}
                ->{ON_INCLUDE}( $parser->{POD_HTMLEASY}, $expansion );
            if (   -e $file
                && -r _ ) # _ is the last checked filehandle.
            {
                $parser->{POD_HTMLEASY}->parse_include($file);
            }
        }
        default {
            if ( defined $parser->{POD_HTMLEASY}->{qq{ON_\U$command\E}} ) {
                $html = $parser->{POD_HTMLEASY}
                    ->{qq{ON_\U$command\E}}( $parser->{POD_HTMLEASY},
                    $expansion );
            }
            elsif ( $command !~ /^(?:pod|cut)$/imx ) {
                $html = qq{<pre>=$command $expansion</pre>};
            }
            else { $html = $EMPTY; }
        }
    };

    if ( $html ne $EMPTY ) {
        print { $parser->output_handle() } $html;
    }    # [6062]

    return;
}

############
# VERBATIM #
############

# Overrides verbatim() provided by base class in Pod::Parser
sub verbatim {
    my ( $parser, $paragraph, $line_num ) = @_;

    if ( exists $parser->{POD_HTMLEASY}->{IN_BEGIN} ) { return; }
    $parser->{POD_HTMLEASY}->{VERBATIM_BUFFER} .= $paragraph;

    return;
}

sub _verbatim {
    my ($parser) = @_;

    if ( exists $parser->{POD_HTMLEASY}->{IN_BEGIN} ) { return; }
    my $expansion = $parser->{POD_HTMLEASY}->{VERBATIM_BUFFER};
    $parser->{POD_HTMLEASY}->{VERBATIM_BUFFER} = $EMPTY;

    _encode_entities( \$expansion );

    my $html = $parser->{POD_HTMLEASY}
        ->{ON_VERBATIM}( $parser->{POD_HTMLEASY}, $expansion );

    # And remove any NUL escapes
    _remove_nul_escapes( \$html );

    if ( $html ne $EMPTY ) {
        print { $parser->output_handle() } $html;
    }    # [6062]

    return;
}

#############
# TEXTBLOCK #
#############

# Overrides textblock() provided by base class in Pod::Parser
sub textblock {
    my ( $parser, $paragraph, $line_num ) = @_;

    if ( exists $parser->{POD_HTMLEASY}->{IN_BEGIN} ) { return; }
    if ( defined $parser->{POD_HTMLEASY}->{VERBATIM_BUFFER} ) {
        _verbatim($parser);
    }    # [6062]

    my $expansion = $parser->interpolate( $paragraph, $line_num );

    $expansion =~ s{^\s+}{}gsmx;
    $expansion =~ s{\s+$}{}gsmx;

    # Encode HTML-specific characters before adding any HTML (eg <p>)
    _encode_entities( \$expansion );

    my $html = $parser->{POD_HTMLEASY}
        ->{ON_TEXTBLOCK}( $parser->{POD_HTMLEASY}, $expansion );

    # And remove any NUL escapes
    _remove_nul_escapes( \$html );

    if ( $html ne $EMPTY ) { print { $parser->output_handle() } $html; }

    return;
}

#####################
# INTERIOR_SEQUENCE #
#####################

# Overrides interior_sequence() provided by base class in Pod::Parser
sub interior_sequence {
    my ( $parser, $seq_command, $seq_argument, $pod_seq ) = @_;

    my $ret;

    # If we're in the middle of a link then escaping now could break some of
    # the link uncracking code.
    _encode_entities(\$seq_argument) unless $seq_command eq 'L';

    # Not sure how these get in here but HTML doesn't support &sol; (which is
    # simply forward slash
    $seq_argument =~ s/&sol;/\//g;

    if ($pod_seq->nested() and $pod_seq->nested()->cmd_name() eq 'L'
        and $seq_command ne 'E')
    {
        # Interpolating into a hyperlink, ignore formatting, unless we are
        # processing an escape code
        return $seq_argument;
    }

    given ($seq_command) {
        when ("B") {
            $ret = $parser->{POD_HTMLEASY}
                ->{ON_B}( $parser->{POD_HTMLEASY}, $seq_argument );
        }
        when ("C") {
            $ret = $parser->{POD_HTMLEASY}
                ->{ON_C}( $parser->{POD_HTMLEASY}, $seq_argument );
        }
        when ("E") {
            $ret = $parser->{POD_HTMLEASY}
                ->{ON_E}( $parser->{POD_HTMLEASY}, $seq_argument );
        }
        when ("F") {
            $ret = $parser->{POD_HTMLEASY}
                ->{ON_F}( $parser->{POD_HTMLEASY}, $seq_argument );
        }
        when ("I") {
            $ret = $parser->{POD_HTMLEASY}
                ->{ON_I}( $parser->{POD_HTMLEASY}, $seq_argument );
        }
        when ("L") {
            my ( $text, $name, $section, $type ) = _parselink($seq_argument);

            # Held off on escaping these earlier, take care of it now.
            _encode_entities(\$text);
            _encode_entities(\$section) if $section;
            _encode_entities(\$name) if $name;

            $ret = $parser->{POD_HTMLEASY}->{ON_L}(
                $parser->{POD_HTMLEASY},
                $seq_argument, $text, $name, $section, $type, $fnPage,
            );
        }
        when ("S") {
            $ret = $parser->{POD_HTMLEASY}
                ->{ON_S}( $parser->{POD_HTMLEASY}, $seq_argument );
        }
        when ("Z") {
            $ret = $parser->{POD_HTMLEASY}
                ->{ON_Z}( $parser->{POD_HTMLEASY}, $seq_argument );
        }
        default {
            if ( defined $parser->{POD_HTMLEASY}->{qq{ON_\U$seq_command\E}} )
            {
                $ret = $parser->{POD_HTMLEASY}
                    ->{qq{ON_\U$seq_command\E}}( $parser->{POD_HTMLEASY},
                    $seq_argument );
            }
            else {
                $ret = qq{$seq_command<$seq_argument>};
            }
        }
    }

    # Escape HTML-significant characters to prevent them from being escaped
    # later.
    _nul_escape( \$ret );

    return $ret;
}

########################
# PREPROCESS_PARAGRAPH #
########################

# Overrides preprocess_paragraph() provided by base class in Pod::Parser
# NB: the text is _not altered.
sub preprocess_paragraph {
    my $parser = shift;
    my ( $text, $line_num ) = @_;

    if ( $parser->{POD_HTMLEASY}{INFO_COUNT} == 3 ) {
        return $text;
    }

    if ( not exists $parser->{POD_HTMLEASY}{PACKAGE} ) {
        if ( $text =~ m{package}smx ) {
            my ($pack) = $text =~ m{(\w+(?:::\w+)*)}smx;
            if ( defined $pack ) {
                $parser->{POD_HTMLEASY}{PACKAGE} = $pack;
                $parser->{POD_HTMLEASY}{INFO_COUNT}++;
            }
        }
    }

    if ( not exists $parser->{POD_HTMLEASY}{VERSION} ) {
        if ( $text =~ m{VERSION}smx ) {
            my ($ver) = $text =~ m{(\d)(?:\.\d*)?}smx;
            if ( defined $ver ) {
                $parser->{POD_HTMLEASY}{VERSION} = $ver;
                $parser->{POD_HTMLEASY}{INFO_COUNT}++;
            }
        }
    }

    # This situation is created by evt_on_head1()
    if (    ( exists $parser->{POD_HTMLEASY}{TITLE} )
        and ( not defined $parser->{POD_HTMLEASY}{TITLE} ) )
    {
        my @lines = split m{\n}smx, $text;
        my $tmp_text = shift @lines;
        if ( not defined $tmp_text ) { return $text; }
        $tmp_text =~ s{^\s*}{}gsmx;   # delete surrounding whitespace
        $tmp_text =~ s{\s*$}{}gsmx;   # delete surrounding whitespace
        $parser->{POD_HTMLEASY}{TITLE} = $tmp_text;
        $parser->{POD_HTMLEASY}{INFO_COUNT}++;
    }

    return $text;
}

##################
# _PARSE_SECTION #
##################

# Parse a link that is not a URL to get the name and/or section
# Algorithm may be found in perlpodspec. "About L<...> Codes"

sub _parse_section {
    my $link = shift;
    $link =~ s{^\s*}{}sm;    # delete surrounding whitespace
    $link =~ s{\s*$}{}sm;    # delete surrounding whitespace

    # L<"FooBar"> is a the way to specify a section without a name.
    # However, L<Foo Bar> is possible, though deprecated. See below.
    if ($link =~ m/^"/) {
        $link =~ s{^"+\s*}{}sm;  # strip the "s
        $link =~ s{\s*"+$}{}sm;
        return ( undef, $link );
    }

    # So now we have either a name by itself, or name/section
    my ( $name, $section ) = split m[\s*/\s*]sm, $link, 2;

    # Trim leading and trailing whitespace and quotes from section
    if ($section) {
        $section =~ s{"}{}gsm;                # quotes
        $section =~ s{^\s*}{}sm;    # delete surrounding whitespace
        $section =~ s{\s*$}{}sm;    # delete surrounding whitespace
    }    # new leading/trailing

# Perlpodspec observes that and acceptable way to distinguish between L<name> and
# L<section> is that if the link contains any whitespace, then it is a section.
# The construct L<section> is deprecated.
    if ( $name && $name =~ m{\s}sm && !defined $section ) {
        $section = $name;
        $name    = undef;
    }

    return ( $name, $section );
}

###############
# _INFER_TEXT #
###############

# Infer the text content of a L<...> with no text| part (ie a text|-less link)
# By definition (?) either name or section is nonempty, Algorithm from perlpodspec

sub _infer_text {
    my ( $name, $section ) = @_;

    if ($name) {
        return $section
            ? "\"$section\" in $name"
            : $name;
    }

    return $section;
}

##############
# _PARSELINK #
##############

# Parse the content of L<...> and return
#   The text label
#   The name or URL
#   The section (if relevant)
#   The type of link discovered: url, man or pod

sub _parselink {
    my $link = shift;
    my $text;

    # Squeeze out multiple spaces
    $link =~ s{\s+}{$SPACE}gsm;

    if ( $link =~ m{\|}smx ) {

        # Link is in the form "L<Foo|Foo::Bar>"
        ( $text, $link ) = split m{\|}sm, $link, 2;
    }

# Check for a generalized URL. The regex is defined in perlpodspec.
# Quoting perlpodspec: "Authors wanting to link to a particular (absolute) URL, must do so
# only with "L<scheme:...>" codes and must not attempt "L<Some Site Name|scheme:...>"
# Consequently, although $text might be nonempty, we ignore it.
    if ($link =~ m{
                    \A      # The beginning of the string
                    \w+     # followed by some alphanumerics, which would be the protocol (or scheme)
                    :       # literal ":"
                    [^:\s]  # one char that is neither a ":" or whitespace
                    \S*     # maybe some non-whitespace
                    \z      # the end of the string
                   }smx
        )
    {
        return ( $link, $link, undef, q{url} );
    }

    # OK, we've eliminated URLs, so we must be dealing with something else

    my ( $name, $section ) = _parse_section($link);
    if ( not defined $text ) { $text = _infer_text( $name, $section ); }

# A link with parenthesized non-whitespace is assumed to be a manpage reference
# (per perlpodspec))
    my $type =
        ( $name && $name =~ m{\(\S*\)}smx )
        ? q{man}
        : q{pod};

    return ( $text, $name, $section, $type );
}

###################
# _ADD_TREE_POINT #
###################

sub _add_tree_point {
    my ( $parser, $name, $level ) = @_;
    $level ||= 1;

    if ( $level == 1 ) {
        $parser->{POD_HTMLEASY}->{INDEX}{p}
            = $parser->{POD_HTMLEASY}->{INDEX}{tree};
    }
    else {
        if ( exists $parser->{POD_HTMLEASY}->{INDEX}{p} ) {
            while ( $parser->{POD_HTMLEASY}
                ->{INDEX}{l}{ $parser->{POD_HTMLEASY}->{INDEX}{p} }
                > ( $level - 1 ) )
            {
                last
                    if !$parser->{POD_HTMLEASY}
                    ->{INDEX}{b}{ $parser->{POD_HTMLEASY}->{INDEX}{p} };
                $parser->{POD_HTMLEASY}->{INDEX}{p} = $parser->{POD_HTMLEASY}
                    ->{INDEX}{b}{ $parser->{POD_HTMLEASY}->{INDEX}{p} };
            }
        }
    }

    my $array = [];

    $parser->{POD_HTMLEASY}->{INDEX}{l}{$array} = $level;
    $parser->{POD_HTMLEASY}->{INDEX}{b}{$array}
        = $parser->{POD_HTMLEASY}->{INDEX}{p};

    push @{ $parser->{POD_HTMLEASY}->{INDEX}{p} }, $name, $array;
    $parser->{POD_HTMLEASY}->{INDEX}{p} = $array;

    return;

}

#############
# BEGIN_POD #
#############

# Overrides begin_pod() provided by base class in Pod::Parser
sub begin_pod {
    my ($parser) = @_;

    if ( $parser->{POD_HTMLEASY_INCLUDE} ) { return; }

    delete $parser->{POD_HTMLEASY}->{INDEX};
    $parser->{POD_HTMLEASY}->{INDEX} = { tree => [] };

    return 1;
}

###########
# END_POD #
###########

# Overrides end_pod() provided by base class in Pod::Parser
sub end_pod {
    my ($parser) = @_;

    if ( $parser->{POD_HTMLEASY_INCLUDE} ) { return; }

    if ( defined $parser->{POD_HTMLEASY}->{VERBATIM_BUFFER} ) {
        _verbatim($parser);
    }

    my $tree = $parser->{POD_HTMLEASY}->{INDEX}{tree};

    delete $parser->{POD_HTMLEASY}->{INDEX};

    $parser->{POD_HTMLEASY}->{INDEX} = $tree;

    return 1;
}

###########
# _ERRORS #
###########

sub _errors {
    my ( $parser, $error ) = @_;

    carp "$error";
    $error =~ s{^\s*\**\s*errors?:?\s*}{}ismx;
    $error =~ s{\s+$}{}smx;

    my $html = $parser->{POD_HTMLEASY}
        ->{ON_ERROR}( $parser->{POD_HTMLEASY}, $error );
    if ( $html ne $EMPTY ) {
        print { $parser->output_handle() } $html, $NL;
    }

    return 1;
}

###########
# DESTROY #
###########

sub DESTROY { }

#######
# END #
#######

1;

#############################################################################
## Name:        HtmlEasy.pm
## Purpose:     Pod::HtmlEasy
## Author:      Graciliano M. P.
## Modified by: Geoffrey Leach
## Created:     2004-01-11
## Updated:	    2007-02-28
## Copyright:   (c) 2004 Graciliano M. P.
## Licence:     This program is free software; you can redistribute it and/or
##              modify it under the same terms as Perl itself
#############################################################################

package Pod::HtmlEasy;
use 5.008;

use Carp;
use English qw{ -no_match_vars };

use strict;
use warnings;

our $VERSION = 0.09;    # Also appears in "=head1 VERSION" in the POD below

our $EMPTY = q{};
our $NL    = qq{\n};
our $NUL   = qq{\0};
our $SPACE = q{ };

########
# VARS #
########

my %BODY_DEF = (
    bgcolor => '#FFFFFF',
    text    => '#000000',
    link    => '#000000',
    vlink   => '#000066',
    alink   => '#FF0000',
);

# This keeps track of valid options
my %OPTS = (
    basic_entities  => 1,
    body            => 1,
    common_entities => 1,
    css             => 1,
    faq_page        => 0,
    function_page   => 0,
    index           => 1,
    index_item      => 1,
    no_css          => 1,
    no_generator    => 1,
    no_index        => 1,
    only_content    => 1,
    parserwarn      => 1,
    title           => 1,
    top             => 1,
);

my $output_file;

#######
# CSS #
#######

my $CSS_DEF = q`
/*
** HTML elements
*/

body {
    margin: 10px;
    padding: 0;
    text-align: center;
    font-size: 0.8em;
    font-family: "Bitstream Vera Sans", "Lucida Grande", "Trebuchet MS", sans-serif;
    color: #535353;
    background: #ffffff;
}


/*
** HTML Tags
*/

h1, h2, h3, h4
{
    padding: 0;
    text-align: left;
    font-weight: bold;
    color: #f7800a;
    background: transparent;
}

h1 {
    margin: 0 0 0.3em 0;
    font-size: 1.7em;
}

h1.name + p {
    font-size: larger;
    font-style: oblique;
}

h2, h3, h4 {
    margin: 1.3em 0 0 0.3em
}

h2 {
    font-size: 1.5em;
}

h3 {
    font-size: 1.4em;
}

h4 {
    font-size: 1.3em;
}

h5 {
    font-size: 1.2em;
}

a:link {
    padding-bottom: 0;
    text-decoration: none;
    color: #0057ae;
}

a:visited {
    padding-bottom: 0;
    text-decoration: none;
    color: #644A9B;
}


a[href]:hover {
    text-decoration: underline;
}

hr {
    margin: 0.3em 1em 0.3em 1em;
    height: 1px;
    border: #bcbcbc dashed;
    border-width: 0 0 1px 0;
}

pre {
    display: block;
    margin: 0.3em;
    padding: 0.3em;
    font-size: 1em;
    color: #000000;
    text-align: left;
    background: #f9f9f9;
    border: #2f6fab dashed;
    border-width: 1px;
    overflow: auto;
    line-height: 1.1em;
}

input, textarea, select {
    margin: 0.2em;
    padding: 0.1em;
    color: #888888;
    background: #ffffff;
    border: 1px solid;
}

blockquote {
    margin: 0.3em;
    padding-left: 2.5em;
    background: transparent;
}

del {
    color: #800000;
    text-decoration: line-through;
}

dt {
    font-weight: bold;
    font-size: 1.05em;
    color: #0057ae;
}

dd {
    margin-left: 1em;
}

p {
    margin-top: 0.5em;
    margin-bottom: 0.9em;
    text-align: justify;
}
fieldset {
    border: #cccccc 1px solid;
}

li {
    text-align: left;
}

fieldset {
    margin-bottom: 1em;
    padding: .5em;
}

form {
    margin: 0;
    padding: 0;
}

hr {
    height: 1px;
    border: #888888 1px solid;
    background: #888888;
    margin: 0.5em 0 0.5em 0 ;
}

.toc a {
    text-decoration: none;
}

.toc li {
    list-style-type: none;
    line-height: larger;
}

img {
    border: 0;
}

table {
    border-collapse: collapse;
    font-size: 1em;
}

th {
    text-align: left;
    padding-right: 1em;
    border: #cccccc solid;
    border-width: 0 0 3px 0;
}
`;

###############
# DEFAULT_CSS #
###############

sub default_css {
    return $CSS_DEF;
}

#######################
# _ORGANIZE_CALLBACKS #
#######################

sub _organize_callbacks {
    my $this = shift;

    $this->{ON_B} = \&evt_on_B;
    $this->{ON_C} = \&evt_on_C;
    $this->{ON_E} = \&evt_on_E;
    $this->{ON_F} = \&evt_on_F;
    $this->{ON_I} = \&evt_on_I;
    $this->{ON_L} = \&evt_on_L;
    $this->{ON_S} = \&evt_on_S;
    $this->{ON_X} = \&evt_on_X;    # [20078]
    $this->{ON_Z} = \&evt_on_Z;

    $this->{ON_HEAD1} = \&evt_on_head1;
    $this->{ON_HEAD2} = \&evt_on_head2;
    $this->{ON_HEAD3} = \&evt_on_head3;
    $this->{ON_HEAD4} = \&evt_on_head4;

    $this->{ON_VERBATIM}  = \&evt_on_verbatim;
    $this->{ON_TEXTBLOCK} = \&evt_on_textblock;

    $this->{ON_OVER} = \&evt_on_over;
    $this->{ON_ITEM} = \&evt_on_item;
    $this->{ON_BACK} = \&evt_on_back;

    $this->{ON_FOR}   = \&evt_on_for;
    $this->{ON_BEGIN} = \&evt_on_begin;
    $this->{ON_END}   = \&evt_on_end;

    $this->{ON_INDEX_NODE_START} = \&evt_on_index_node_start;
    $this->{ON_INDEX_NODE_END}   = \&evt_on_index_node_end;

    $this->{ON_INCLUDE} = \&evt_on_include;
    $this->{ON_URI}     = \&evt_on_uri;

    $this->{ON_ERROR} = \&evt_on_error;

    return;
}

#######
# NEW #
#######

sub new {
    my $this = shift;
    return $this if ref $this;
    my $class = $this || __PACKAGE__;
    $this = bless {}, $class;

    my (%args) = @_;
    _organize_callbacks($this);

    # Backwards compatibility
    if ( exists $args{on_verbatin} ) {
        $this->{ON_VERBATIM} = $args{on_verbatin};
    }

    foreach my $key ( keys %args ) {

        # Add in any ON_ callbacks
        if ( $key =~ m{^on_(\w+)$}ismx ) {
            my $cmd = uc $1;
            $this->{qq{ON_$cmd}} = $args{$key};
        }
        elsif ( $key =~ m{^(?:=(\w+)|(\w)<>)$}smx ) {
            my $cmd = uc $1 || $2;
            $this->{$cmd} = $args{$key};
        }
    }

    return $this;
}

############
# POD2HTML #
############

sub pod2html {
    my $this = shift;
    my $file = shift;

    # Assume a non-option second arg is a file name
    my $save = (exists $OPTS{ $_[0] } ? undef: shift) if defined $_[0];
    my %args = @_;

    # Check options for validity
    foreach my $key ( keys %args ) {
        if ( not exists $OPTS{$key} ) {
            carp qq{option $key is not supported};
        }
    }

    # No /x please
    if ( defined $save && $save =~ m{$NL}sm ) {

        # Is this a M$ way of saying "nothing there"?
        $save = undef;
    }

    # This will fall through to Pod::Parser::new
    # which is the base for Pod::HtmlEasy::Parser
    # and Pod::HtmlEasy::Parser does not implement new()
    my $parser = Pod::HtmlEasy::Parser->new();

    $parser->errorsub( sub { Pod::HtmlEasy::Parser::errors( $parser, @_ ); }
    );

    # Pod::Parser wiii complain about multiple blank lines in the input
    # which is moderately annoying
    if ( exists $args{parserwarn} ) { $parser->parseopts( -warnings => 1 ); }

    # This allows us to search for non-POD stuff is preprocess_paragraph
    $parser->parseopts( -want_nonPODs => 1 );

    if (exists $args{'function_page'}) { $parser->setFunctionPage($args{'function_page'}); }

    # This puts a subsection in the $parser hash that will record data
    # that is "local" to this code.  Throughout, $parser will refer to
    # Pod::Parser and $this to Pod::HtmlEasy
    $parser->{POD_HTMLEASY} = $this;

    if ( exists $args{index_item} ) { $parser->{INDEX_ITEM} = 1; }
    if ( exists $args{basic_entities} ) {
        carp q{"basic_entities" is deprecated.};
    }
    if ( exists $args{common_entities} ) {
        carp q{"common_entities" is deprecated.};
    }

    # *HTML supplies a PRINT method that's used by the parser to do output
    # It gets accumulated into HTML, which is tied to $output.
    # You'll also see calls to  print {$parser->output_handle()} ...
    # which accomplishes the same thing. When all is said and done, the output
    # of the parse winds up in $output declared below, and used in the construction
    # of @html.

    my $output = [];
    local *HTML;
    tie *HTML => 'Pod::HtmlEasy::TieHandler', $output;
    my $html = \*HTML;
    $this->{TIEDOUTPUT} = $html;

    my $title = $args{title};
    if ( ref $file eq q{GLOB} ) {    # $file is an open filehandle
        if ( not defined $title ) { $title = q{<DATA>}; }
    }
    else {
        if ( !-e $file ) {
            carp qq{No file $file};
            return;
        }
        if ( not defined $title ) { $title = $file; }
    }

    # Build the header to the HTML file
    my @html;
    push @html,
        qq{<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">$NL};
    push @html, qq{<html><head>$NL};
    push @html,
        qq{<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">$NL};

    if ( not exists $args{no_generator} ) {
        push @html,
            qq{<meta name="GENERATOR" content="kio_perldoc Pod::HtmlEasy/$VERSION Pod::Parser/$Pod::Parser::VERSION Perl/$] [$^O]">$NL};
    }
    push @html, qq{<title>$title</title>$NL};
    my $title_line_ref = \$html[-1];
    push @html, _organize_css( \%args );
    push @html, qq{</head>$NL};
    if ( not exists $args{only_content} ) {
        push @html, _organize_body( \%args );
    }

    delete $this->{UPARROW};
    delete $this->{UPARROW_FILE};
    push @html, qq{<div id="header"><div id="header_top"><div><div>$NL};
    push @html, qq{<img src="help:/common/top-kde.jpg"> $title - KDE Perldoc Viewer</div></div></div></div>};
    push @html, q{<div class="pod">};

    if ( exists $args{top} ) {
        push @html, qq{$NL<a name='_top'></a>$NL};
        if ( -e $args{top} ) {
            $this->{UPARROW_FILE} = $args{top};
        }
        else {
            $this->{UPARROW} = $args{top};
        }
    }

    if ($args{'faq_page'}) {
        push @html, qq{$NL<p>The following FAQ results were found:</p>$NL};
    }

    # Avoid carry-over on multiple files
    delete $this->{IN_BEGIN};
    delete $this->{PACKAGE};
    delete $this->{TITLE};
    delete $this->{VERSION};
    $this->{INFO_COUNT} = 0;

    # A filehandle as both args is not documented, but is supported
    # Everything that Pod::Parser prints winds up in $output
    $parser->parse_from_file( $file, $html );

    # If there's a head1 NAME, we've picked this up during processing
    if ( defined $this->{TITLE} && length $this->{TITLE} > 0 ) {
        ${$title_line_ref} = qq{<title>$this->{TITLE}</title>$NL};
    }

  # Note conflict here: user can specify an index, and no_index; no_index wins
    if ( not exists $args{index} ) { $args{index} = $this->build_index(); }
    if ( exists $args{no_index} )  { $args{index} = $EMPTY; }

    if ( $args{'faq_page'} && not @{$output} ) {
        @{$output} = ("<b>None</b>");
    }

    my @kde_version_output = `kde4-config --version`;
    my $kde_version;
    for (@kde_version_output) {
        ($kde_version) = m/^KDE[^:]*:\s*(.*)$/;
        last if defined $kde_version;
    }

    $kde_version = "Unknown" unless $kde_version;

    push @html, qq{$args{index}$NL};
    push @html, @{$output};                      # The pod converted to HTML
    push @html, qq{</div><div id="footer"><div id="footer_text">Generated by kio_perldoc, version $kde_version</div></div>};
    push @html, qq{</body></html>$NL};

    delete $this->{TIEDOUTPUT};
    close $html or carp q{Could not close html};
    untie $html or carp q{Could not untie html};

    if ( defined $save ) {
        open my $out, q{>}, $save or croak qq{Unable to open $save - $!};
        print {$out} @html;
        close $out;
    }

    return wantarray ? @html : join $EMPTY, @html;
}

#################
# PARSE_INCLUDE #
#################

sub parse_include {
    my $this = shift;
    my $file = shift;

    my $parser = Pod::HtmlEasy::Parser->new();
    $parser->errorsub( sub { Pod::HtmlEasy::Parser::errors( $parser, @_ ); }
    );
    $parser->{POD_HTMLEASY}         = $this;
    $parser->{POD_HTMLEASY_INCLUDE} = 1;

    $parser->parse_from_file( $file, $this->{TIEDOUTPUT} );

    return 1;
}

##############
# WALK_INDEX #
##############

sub walk_index {
    my ( $this, $tree, $on_open, $on_close, $output ) = @_;

    my $i = 0;
    while ( $i < @{$tree} ) {
        my $nk =
            ref( ${$tree}[ $i + 1 ] ) eq q{ARRAY}
            ? @{ ${$tree}[ $i + 1 ] }
            : undef;
        $nk = $nk >= 1 ? 1 : undef;

        my $a_name = ${$tree}[$i];
        $a_name =~ s{<.*?>}{}gsmx;

        #$a_name =~ s{&\w+;}{}gsmx;
        #$a_name =~ s{\W+}{-}gsmx;

        if ($on_open) {
            my $ret = $on_open->( $this, ${$tree}[$i], $a_name, $nk );
            if ( $output and defined $ret ) {
                ${$output} .= $ret;
            }    # [6062]
        }

        if ($nk) {
            walk_index( $this, ${$tree}[ $i + 1 ],
                $on_open, $on_close, $output );
        }

        if ($on_close) {
            my $ret = $on_close->( $this, ${$tree}[$i], $a_name, $nk );
            if ( $output and defined $ret ) {
                ${$output} .= $ret;
            }    # [6062]
        }
        $i += 2;
    }
    return;
}

###############
# BUILD_INDEX #
###############

sub build_index {
    my $this = shift;

    my $index = $EMPTY;    # [6062]
    $this->walk_index(
        $this->{INDEX},
        $this->{ON_INDEX_NODE_START},
        $this->{ON_INDEX_NODE_END}, \$index
    );

    return qq{<div class="toc">$NL<ul>$NL$index</ul>$NL</div>$NL};
}

#################
# _ORGANIZE_BODY #
#################

sub _organize_body {
    my $args_ref = shift;

    my ( $body, %body );

    $body = $EMPTY;
    if ( ref $args_ref->{body} eq q{HASH} ) {
        %body = %BODY_DEF;
        my %body_attr = %{ $args_ref->{body} };
        foreach my $key ( keys %body_attr ) {
            $body{$key} = $body_attr{$key};
        }
    }
    elsif ( !exists $args_ref->{body} ) { %body = %BODY_DEF; }

    if (%body) {
        foreach my $key ( sort keys %body ) {
            if ( $body{$key} !~ m{\#}smx && defined $BODY_DEF{$key} ) {
                $body{$key} = qq{#$body{$key}};
            }
            my $value =
                $body{$key} !~ m{"}smx
                ? qq{"$body{$key}"}
                : qq{'$body{$key}'};
            $body .= qq{ $key=$value};
        }
    }
    else { $body = $args_ref->{body}; }

    return qq{<body $body>};
}

################
# ORGANIZE_CSS #
################

sub _organize_css {
    my $perldoc_css = `kde4-config --path data --locate kio_docfilter/kio_docfilter.css`;
    chomp $perldoc_css;

    return <<"EOL";
<link rel="stylesheet" href="help:/common/kde-default.css" type="text/css">
<link rel="stylesheet" href="file://$perldoc_css" type="text/css">
<style>
    #header_top {
        background-image: url("help:/common/top.jpg");
    }
    #header_top div {
        background-image: url("help:/common/top-left.jpg");
    }
    #header_top div div {
        background-image: url("help:/common/top-right.jpg");
    }
    div.pod {
        margin-left: 5em;
        margin-right: 5em;
    }
</style>
EOL
}

##################
# EVENT SUPPORT  #
##################

sub do_title {
    my $this = shift;
    my ( $txt, $a_name ) = @_;

    # This happens only on the _first_ head1 NAME
    if ( ( not exists $this->{TITLE} ) and ( $txt =~ m{\ANAME}smx ) ) {
        my ($title) = $txt =~ m{\ANAME\s+(.*)}smx;
        if ( defined $title ) {

            # Oh, goody
            $title =~ s{^\s*}{}gsmx;  # delete surrounding whitespace
            $title =~ s{\s*$}{}gsmx;  # delete surrounding whitespace
            $this->{TITLE} = $title;
        }
        else {

# If we don't get anything off of NAME, it will be filled in by preprocess_paragraph()
            $this->{TITLE} = undef;
        }
    }
    return;
}

##################
# DEFAULT EVENTS #
##################

sub evt_on_head1 {
    my $this = shift;
    my ( $txt, $a_name ) = @_;

    if ( not defined $txt ) { $txt = $EMPTY; }

    do_title( $this, $txt, $a_name );

    if ( exists $this->{UPARROW_FILE} ) {
        return "<h1><a href='#_top'
                 title='click to go to top of document' 
                 name='$a_name'>$txt<img src='$this->{UPARROW_FILE}'
                 alt=&uArr;></a></h1>$NL";
    }
    elsif ( exists $this->{UPARROW} ) {
        return qq{<h1><a href='#_top'
                  title='click to go to top of document' 
                  name='$a_name'>$txt&$this->{UPARROW};</a></h1>$NL};
    }

    my $style = '';
    $style = 'class="name"' if $txt =~ m{\ANAME};

    return qq{<a name='$a_name'></a><h1 $style>$txt</h1>$NL};
}

sub evt_on_head2 {
    my $this = shift;
    my ( $txt, $a_name ) = @_;
    return qq{<a name='$a_name'></a><h2>$txt</h2>$NL$NL};
}

sub evt_on_head3 {
    my $this = shift;
    my ( $txt, $a_name ) = @_;
    return qq{<a name='$a_name'></a><h3>$txt</h3>$NL$NL};
}

sub evt_on_head4 {
    my $this = shift;
    my ( $txt, $a_name ) = @_;
    return qq{<a name='$a_name'></a><h4>$txt</h4>$NL$NL};
}

sub evt_on_begin {
    my $this = shift;
    my ( $txt, $a_name ) = @_;
    $this->{IN_BEGIN} = 1;
    return $EMPTY;
}

sub evt_on_end {
    my $this = shift;
    my ( $txt, $a_name ) = @_;
    delete $this->{IN_BEGIN};
    return $EMPTY;
}

sub evt_on_L {
    my ($this, $L, $text, $page, $section, $type, $fnPage) = @_;

    Pod::HtmlEasy::Parser::_encode_entities(\$page);

    if($type eq 'pod') {
        $page = '' unless defined $page;

        if($fnPage and not $page) {
            $page = "functions/";
            $section = "" unless defined $section;
        }
        else {
            $section = defined $section ? "#$section" : "";
        }

        # Keep later escaping functions from incorrectly messing this up.
        $section =~ s/^(.)/$1$NUL/;

        # The browser knows what to do for simple links in the same page,
        # and knows how to do it better than we do.
        return "<a href=\"$section\">$text</a>" unless $page;

        $page =~ s/^(.)/$1$NUL/;
        return "<a href=\"perldoc:$page$section\">$text</a>";
    }
    elsif($type eq 'man') {
        return "<a href=\"man:/$page\">$text</a>";
    }
    elsif($type eq 'url') {
        # Keep later escaping functions from incorrectly messing this up.
        $page =~ s/^(.)/$1$NUL/;
        $text =~ s/^(.)/$1$NUL/;

        return "<a href=\"$page\" target=\"_blank\">$text</a>";
    }
}

sub evt_on_B {
    my $this = shift;
    my $txt  = shift;
    return qq{<b>$txt</b>};
}

sub evt_on_I {
    my $this = shift;
    my $txt  = shift;
    return qq{<i>$txt</i>};
}

sub evt_on_C {
    my $this = shift;
    my $txt  = shift;

    # Who would put IMG tags in this?
    return $EMPTY if $txt =~ m{<IMG}i;
    return "<tt>$txt</tt>";
}

sub evt_on_E {
    my $this = shift;
    my $txt  = shift;

    # Hey guess what, not all &foo; entities are part of HTML!  So don't go
    # throwing them out without checking.
    return "/" if $txt eq '&sol;';

    $txt =~ s{^&}{}smx;
    $txt =~ s{;$}{}smx;
    if ( $txt =~ m{^\d+$}smx ) { $txt = qq{#$txt}; }
    return qq{$NUL&$txt;};
}

sub evt_on_F {
    my $this = shift;
    my $txt  = shift;

    # Some pod encoders put hyperlinks in F<>??
    return qq{<a href="$txt">$txt</a>} if $txt =~ m'^http://';
    return qq{<b><i>$txt</i></b>};
}

sub evt_on_S {
    my $this = shift;
    my $txt  = shift;
    $txt =~ s{$SPACE}{&nbsp;}gs;
    return $txt;
}

sub evt_on_X { return $EMPTY; }    # [20078]

sub evt_on_Z { return $EMPTY; }

sub evt_on_verbatim {
    my $this = shift;
    my $txt  = shift;

    return if exists $this->{IN_BEGIN};

    # Multiple empty lines are parsed as verbatim text by Pod::Parser
    # And will show up as empty <pre> blocks, which is mucho messy
    {
        local $RS = $EMPTY;
        chomp $txt;
    }

    if ( not length $txt ) { return $EMPTY; }
    return qq{<pre>$txt</pre>$NL};
}

sub evt_on_textblock {
    my $this = shift;
    my $txt  = shift;
    return if exists $this->{IN_BEGIN};
    return qq{<p>$txt</p>$NL};
}

sub evt_on_over {
    my $this  = shift;
    my $level = shift;
    return qq{<ul>$NL};
}

sub evt_on_item {
    my $this = shift;
    my ( $txt, $a_name ) = @_;

    # POD has no list item tag, so most authors simulate it, breaking the HTML.
    # So, strip out most bullet characters.
    $txt =~ s/^\s*[\*oO\+\-\.](?!\S)//;

    # If POD authors are going so far as to number the list, it's already too
    # late to fix it by using <ol> tag, but we can change the CSS style.
    my $style = '';
    if ($txt =~ m/^\d+\.?/) {
        $style = qq{ style="list-style-type:decimal;"};
        $txt =~ s/^\d+\.?\s*//;
    }

    return qq{<li$style>$NL} if $txt =~ /^\s*$/;
    return qq{<li$style><a name='$a_name'></a><b>$txt</b></li>$NL};
}

sub evt_on_back {
    my $this = shift;
    return qq{</ul>$NL};
}

sub evt_on_for { return $EMPTY; }

sub evt_on_error {
    my $this = shift;
    my $txt  = shift;
    return qq{<!-- POD_ERROR: $txt -->};
}

sub evt_on_include {
    my $this = shift;
    my $file = shift;
    return $file;
}

sub evt_on_uri {
    my $this = shift;
    my $uri  = shift;
    my $target =
        $uri !~ m{^(?:mailto|telnet|ssh|irc):}ismx
        ? q{ target='_blank'}
        : $EMPTY;    # [6062]
    my $txt = $uri;
    $txt =~ s{^mailto:}{}ismx;
    return qq{<a href='$uri'$target>$txt</a>};
}

sub evt_on_index_node_start {
    my $this = shift;
    my ( $txt, $a_name, $has_children ) = @_;

    my $ret = qq{<li><a href='#$a_name'>$txt</a>$NL};
    if ($has_children) {
        $ret .= qq{$NL<ul>$NL};
    }
    return $ret;
}

sub evt_on_index_node_end {
    my $this = shift;
    my ( $txt, $a_name, $has_children ) = @_;

    my $ret = $has_children ? q{</ul>} : undef;
    return $ret;
}

##############
# PM_VERSION #
##############

sub pm_version {
    my $this = ref( $_[0] ) ? shift: undef;
    if ( not defined $this ) {
        carp q{pm_version must be referenced through Pod::HtmlEasy};
        return;
    }

    return $this->{VERSION};
}

##############
# PM_PACKAGE #
##############

sub pm_package {
    my $this = ref( $_[0] ) ? shift: undef;
    if ( not defined $this ) {
        carp q{pm_package must be referenced through Pod::HtmlEasy};
        return;
    }

    return $this->{PACKAGE};
}

###########
# PM_NAME #
###########

sub pm_name {
    my $this = ref( $_[0] ) ? shift: undef;
    if ( not defined $this ) {
        carp q{pm_name must be referenced through Pod::HtmlEasy};
        return;
    }
    return $this->{TITLE};
}

###########################
# PM_PACKAGE_VERSION_NAME #
###########################

sub pm_package_version_name {
    my $this = ref( $_[0] ) ? shift: undef;
    if ( not defined $this ) {
        carp
            q{pm_package_version_name must be referenced through Pod::HtmlEasy};
        return;
    }

    return ( $this->pm_package(), $this->pm_version(), $this->pm_name() );
}

#######
# END #
#######

1;

package main;

# Pass argv to perldoc, which we will redirect the output of perldoc to this
# process.  This syntax only works in Perl 5.8 and higher, but that's OK, the
# rest of the code also requires Perl 5.8.
open my $perlDocInput, "-|", qw/perldoc -u -T/, @ARGV
     or die "Unable to open pipe: $!";

my $usingFunctionPage = 0;
if(exists $ARGV[0] and $ARGV[0] eq '-f')
{
    # Looking for a function, rewrite some URLs that refer to other functions
    # on the same page.
    $usingFunctionPage = 1;
}

my $podhtml = Pod::HtmlEasy->new();

print $podhtml->pod2html($perlDocInput,
                         'function_page'=>$usingFunctionPage,
                         'faq_page' => exists $ARGV[0] && $ARGV[0] eq '-q',
                         title=>"$ARGV[-1]"), "\n";
