#!/usr/bin/perl
#
# AuthDriver.pm:
# External authenticator for use with tpop3d's auth-other.
#
# Copyright (c) 2001 Chris Lightfoot. All rights reserved.
#
# $Id$
#

# POD begins

=head1 NAME

TPOP3D::AuthDriver - library to interface to tpop3d's auth-other

=head1 SYNOPSIS

  package FredAuthDriver;
  
  use TPOP3D::AuthDriver;
  @ISA = (TPOP3D::AuthDriver);

  sub start($) {
    # startup code
  }

  sub finish($) {
    # shutdown code
  }
  
  sub pass($$) {
    my ($self, $req) = @_;
    if ($req->{'user'} eq "fred" and $req->{pass} eq "secret") {
      # Success.
      return { 'result' => 'YES',
               'logmsg' => 'authenticated Fred',
               'uid' => 'fred',   # Fred's UID
               'gid' => 'mail',
               'mboxtype' => 'bsd',
               'mailbox' => '/var/spool/mail/fred' };
    } else {
      # Failure.
      return { 'result' => 'NO', 'logmsg' => 'authentication failed' };
    }
  }

  1;

  package Main;

  $auth = new FredAuthDriver;
  $auth->run();

=head1 DESCRIPTION

The TPOP3D::AuthDriver object presents a generic interface to authentication
for tpop3d. Objects derived from it can implement specific authentication
strategies to customise the server. This is designed mainly for use with
virtual-domains configurations.

=head1 PUBLIC INTERFACE

=over 4

=cut

package TPOP3D::AuthDriver;

use MD5;

$VERSION = '0.1';

=item new

I<Class method.>
Creates a new authentication driver object.

=cut
sub new ($) {
    my ($proto) = @_;
    my $class = ref($proto) || $proto;
    my $self = { foad => 0 };
    bless($self, $class);

    return $self;
}

=item run

I<Instance method.>
Start processing requests from tpop3d. This will install a handler for
SIGTERM; on receiving this signal, the method will return.

=cut
sub run ($) {
    my $self = shift;
    my ($buffer, $offset);
    local %SIG;
    $SIG{TERM} = sub { $self->{foad} = 1; };

    $self->start();

    $buffer = '';
    $offset = 0;
    do {
        my $readfds = '';
        vec($readfds, fileno(STDIN), 1) = 1;
        if (select($readfds, undef, undef, 0.1) == 1) {
            my $i = sysread(STDIN, $buffer, 4096, $offset);
            if ($i > 0) {
                $offset += $i;
            } else {
                $self->{foad} = 1;
            }

            # Now see whether the first part of the buffer has the right
            # structure:
            if ($buffer =~ /((?:(?:[^\0]+\0){2})+)\0/) {
                my $packet = $1;
                $buffer = substr($buffer, length($packet));
                last unless ($self->process_packet($packet));
            }
        }
    } while (!$self->{foad});

    $self->finish();
}

# process_packet:
# Process a formatted packet of data, and dispatch it to the appropriate
# handler.
sub process_packet ($$) {
    my ($self, $packet) = @_;
    my %hash = split("\0", $packet);
    
    $res = $self->apop(\%hash) if ($hash{method} eq 'APOP');
    $res = $self->pass(\%hash) if ($hash{method} eq 'USER');

    if (!defined($res)) {
        # OK, we didn't handle it; perhaps we are chained to another handler?
        $res = $self->{next}->process_request(\%hash) if (defined($self->{next}));
    }
    
    return $self->send_packet($res);
}

# send_packet:
# Return the results to the server, appropriately formatted.
sub send_packet ($$) {
    my ($self, $res) = @_;
    my $packet = join("\0", map { "$_\0$res->{$_}"; } keys %{$res}) . "\0\0";
    return syswrite(STDOUT, "$packet", length($packet)) == length($packet);
}

=item start

I<Instance method.>
This method is called by run() before it starts processing requests. You
should override this to perform any initialisation steps your authentication
driver needs.

=cut
sub start ($) {
    my $self = shift;
    # do nothing
}

=item finish

I<Instance method.>
This method is called by run() when it ceases to process requests after
receipt of SIGTERM. You should override this to perform any shutdown steps
your authentication driver needs.

=cut
sub finish ($) {
    my $self = shift;
    # do nothing
}

=item apop REQUEST

I<Instance method.>
This method is called by run() when a request for APOP authentication is
received. REQUEST is a reference to a hash of the parameters supplied by the
server, including

=over 4

=item timestamp

server's RFC1939 timestamp

=item user

client's supplied username

=item digest

client's supplied digest, in hex

=back


It should return a reference to a hash containing the following keys:

=over 4

=item result

if authentication was successful, `YES'; otherwise `NO'

=item uid

username/uid with which to access mailspool

=item gid

groupname/gid with which to access mailspool

=item domain

(optional) domain in which the user has been authenticated

=item mailbox

(optional) location of mailbox

=item mboxtype

(optional) name of mailbox driver

=item logmsg

(optional) message to log

=back 4

Note that if you do not supply a value for C<mailspool>, then the mailspool
name will be determined from the `mailbox:' and `auth-other-mailbox:'
tpop3d configuration directives; if you supply a value for C<mailbox>, but
not for C<mboxtype>, then the `default' mailbox type will be used; but this is
dependent on your installed version of tpop3d, and should not be relied upon.

You should override this to perform APOP authentication, if you want to use
it.

=cut
sub apop ($$) {
    my ($self, $req) = @_;
    return { 'result' => 'NO' };    # default
}

=item apopauth DIGEST, TIMESTAMP, PASSWORD

I<Class method.>
Returns true if the given plaintext PASSWORD and TIMESTAMP correspond to the
given DIGEST. This is a utility method designed to make it easier to write
APOP authenticators.

=cut
sub apopauth ($$$) {
    my ($digest, $timestamp, $password) = @_;
    if (lc($digest) eq lc(Digest::MD5::md5_hex($timestamp . $password))) {
        return 1;
    } else {
        return 0;
    }
}

=item pass REQUEST

I<Instance method.>
This method is called by run() when a request for USER/PASS authentication is
received. REQUEST is a reference to a hash of the parameters supplied by the
server, including

=over

=item user

client's supplied username

=item pass

client's supplied password

=back

It should return a reference to a hash, as described for the C<apop> method
above. You should override this to perform USER/PASS authentication, if you
want to use it.

=cut

sub pass ($$) {
    my ($self, $req) = @_;
    return { 'result' => 'NO' };    # default
}

1;

__END__

=head1 COPYING

Copyright (c) 2001 Chris Lightfoot, <chris@ex-parrot.com>
F<http://www.ex-parrot.com/~chris/tpop3d>

This program is free software; you can redistribute and/or modify it under the
same terms as Perl itself.

=head1 BUGS

None yet; please send me information when you find them.

=head1 VERSION

$Id$

=head1 SEE ALSO

F<tpop3d>, F<RFC1939>, F<http://www.ex-parrot.com/~chris/tpop3d/>
