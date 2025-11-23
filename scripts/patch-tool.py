#!/usr/bin/env python3
#
# Modify OB-XF fxp files to adjust the XML

import sys
import struct
import xml.dom.minidom
import argparse
import os

# Parse command line arguments
parser = argparse.ArgumentParser(description='Modify OB-XF fxp files to adjust the XML')
parser.add_argument('patch_file', help='Path to the patch file to modify')
parser.add_argument('--rename', help='New program name for the patch', default=None)
parser.add_argument('--author', help='Author name for the patch', default=None)
parser.add_argument('--outdir', help='Output directory for the modified patch', default=None)

args = parser.parse_args()

patch = args.patch_file
print("Patch: {0}".format(patch))

with open(patch, mode='rb') as patchFile:
    patchContent = patchFile.read()

fxpHeader = struct.unpack(">4si4siiii28si", patchContent[:60])
(chunkmagic, byteSize, fxMagic, version, fxId, fxVersion, numPrograms, prgName, chunkSize) = fxpHeader


# Juce XML in an FXP uses a magic number (60:64) then the string length so the XML starts at
# 68
xmlct = patchContent[68:(68 + chunkSize -8 - 1)].decode('utf-8')
dom = xml.dom.minidom.parseString(xmlct)

# Get the top-level OB-XF node
obxf_node = dom.getElementsByTagName('OB-Xf')[0]

# Change the programName and author properties if provided
if args.rename:
    obxf_node.setAttribute("programName", args.rename)
if args.author:
    obxf_node.setAttribute("author", args.author)

# Print the modified XML
pretty_xml_as_string = dom.toxml()

# Convert the modified XML to bytes and add terminal zero
new_xml_bytes = pretty_xml_as_string.encode('utf-8') + b'\0'

# Calculate the new chunk size (8 bytes for patchHeader + XML bytes)
new_chunk_size = 8 + len(new_xml_bytes)

# Rebuild the patch header with the new chunk size
new_fxp_header = patchContent[:56] + struct.pack(">i", new_chunk_size)

# and the juce header with the magic number and string length
new_patch_header = patchContent[60:64] + struct.pack("<i", len(new_xml_bytes))

# Expand patchContent array by reconstructing it with all components
patchContent = new_fxp_header + new_patch_header + new_xml_bytes

fdir = os.path.dirname(patch)
if (args.outdir):
    fdir = args.outdir

fname = os.path.basename(patch).replace('.fxp', '_modified.fxp')
if (args.rename):
    fname = args.rename + '.fxp'

# Write patchContent to a binary file
output_file = os.path.join(fdir, fname)

print("\nWriting modified patch to: {0}".format(output_file))
with open(output_file, mode='wb') as output:
    output.write(patchContent)
print("\nSuccess.");

