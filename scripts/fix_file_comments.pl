#!/usr/bin/perl


use File::Find;
use File::Basename;

find(
    {
        wanted => \&findfiles,
    },
    'Source'
);

sub findfiles
{

    $header = <<EOH;
/*
 * OB-Xf - a continuation of the last open source version
 * of OB-Xd.
 *
 * OB-Xd was originally written by Filatov Vadim, and
 * then a version was released under the GPL3
 * at https://github.com/reales/OB-Xd. Subsequently
 * the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more
 * see "HISTORY.md" in the root of this repo.
 *
 * This repository is a successor to the last open source
 * release, a version marked as 2.11. Copyright
 * 2013-2025 by the authors as indicated in the original
 * release, and subsequent authors per the github transaction
 * log.
 *
 * OB-Xf is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source for OB-Xf is available at
 * https://github.com/surge-synthesizer/OB-Xf
 */

EOH

    $f = $File::Find::name;
    if ($f =~ m/\.h$/ or $f =~ m/.cpp$/)
    {
        #To search the files inside the directories
        print "Processing $f\n";

        $q = basename($f);
        print "$q\n";
        open(IN, "<$q") || die "Cant open IN $!";
        open(OUT, "> ${q}.bak") || die "Cant open BAK $!";

        $nonBlank = 0;
        $inComment = 0;
        while(<IN>)
        {
            if ($nonBlank)
            {
                print OUT
            }
            else
            {
                if (m:^\s*/\*:) {
                    $inComment = 1;
                }
                elsif (m:\s*\*/:)
                {
                    print OUT $header;
                    $nonBlank = true;
                    $inComment = false;
                }
                elsif ($inComment)
                {

                }
                elsif (m:^//:)
                {

                }
                else
                {
                    print OUT $header;
                    $nonBlank = true;
                    print OUT;

                }
            }
        }
        close(IN);
        close(OUT);
        system("mv ${q}.bak ${q}");
        system("clang-format -i ${q}");
        system("dos2unix ${q}");
    }
}
