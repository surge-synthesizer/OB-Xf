**OB-Xf Nightly Build**

This Nightly build contains a copy of OB-Xf built at the tip of the code. We are working on things like installers and asset inclusion and the like, but until then, you need to use the ZIP or DMG here to install on your OS.

Once that is done you must also grab the -assets.zip file here, which you must install into your Documents folder

Windows: `%DOCUMENTS%/Surge Synth Team/OB-Xf`

macOS: `~/Documents/Surge Synth Team/OB-Xf`

Linux: The juce algorithm appears to be

 * if you have `~/.cofnig/user-dirs.dirs` with an entry for `XDG_DOCUMENTS_DIR`,
 * otherwise `~/Documents`
 * Whatever that directory is plus `Surge Synth Team/OB-Xf`

If you cant figure it out, dont install assets, run the synth, and it will tell you where it looked.

Assets can change with installs so please update both regularly.

Improving this installer and asset location experience is on our list, including the unsatisfying linux location and the lack of a windows and linux 'portable' install.
