######################################################################
# SPDX-License-Identifier: EPICS
# EPICS BASE is distributed subject to a Software License Agreement
# found in file LICENSE that is included with this distribution.
######################################################################

package DBD::Menu;
use DBD::Base;
our @ISA = qw(DBD::Base);

use strict;

sub init {
    my ($this, $name) = @_;
    $this->SUPER::init($name, "menu");
    $this->{CHOICE_LIST} = [];
    $this->{CHOICE_INDEX} = {};
    $this->{COMMENTS} = [];
    return $this;
}

sub add_choice {
    my ($this, $name, $value) = @_;
    $name = $this->identifier($name, "Choice name");
    foreach my $pair ($this->choices) {
        dieContext("Duplicate menu choice name '$name'")
            if ($pair->[0] eq $name);
        dieContext("Duplicate menu choice string '$value'")
            if ($pair->[1] eq $value);
    }
    push @{$this->{CHOICE_LIST}}, [$name, $value];
    $this->{CHOICE_INDEX}->{$value} = $name;
}

sub choices {
    return @{shift->{CHOICE_LIST}};
}

sub choice {
    my ($this, $idx) = @_;
    return $this->{CHOICE_LIST}[$idx];
}

sub legal_choice {
    my ($this, $value) = @_;
    return exists $this->{CHOICE_INDEX}->{$value};
}

sub add_comment {
    my $this = shift;
    push @{$this->{COMMENTS}}, @_;
}

sub comments {
    return @{shift->{COMMENTS}};
}

sub equals {
    my ($a, $b) = @_;
    return $a->SUPER::equals($b)
        && join(',', map "$_->[0]:$_->[1]", @{$a->{CHOICE_LIST}})
        eq join(',', map "$_->[0]:$_->[1]", @{$b->{CHOICE_LIST}});
}

sub toDeclaration {
    my $this = shift;
    my $name = $this->name;
    my $macro_name = "${name}_NUM_CHOICES";
    my @choices = map {
        sprintf "    %-31s /**< \@brief State string \"%s\" */",
            @{$_}[0], escapeCcomment(@{$_}[1]);
    } $this->choices;
    my $num = scalar @choices;
    return "#ifndef $macro_name\n" .
           "/** \@brief Enumerated type from menu $name */\n" .
           "typedef enum {\n" .
               join(",\n", @choices) .
           "\n} $name;\n" .
           "/** \@brief Number of states defined for menu $name */\n" .
           "#define $macro_name $num\n" .
           "#endif\n\n";
}

sub toDefinition {
    my $this = shift;
    my $name = $this->name;
    my @strings = map {
        "\t\"" . escapeCstring(@{$_}[1]) . "\""
    } $this->choices;
    return "static const char * const ${name}ChoiceStrings[] = {\n" .
               join(",\n", @strings) . "\n};\n" .
           "const dbMenu ${name}MenuMetaData = {\n" .
           "\t\"" . escapeCstring($name) . "\",\n" .
           "\t${name}_NUM_CHOICES,\n" .
           "\t${name}ChoiceStrings\n};\n\n";
}

1;
