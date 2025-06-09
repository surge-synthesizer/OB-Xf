---
name: Release Checklist
about: Clone this template to make a release checklist for each release
title: Release checklist for OB-Xf x.y.z
labels: 'Release Plan'
assignees: ''
---

Release label: OB-Xf x.y.z
Link to milestone: https://github.com/surge-synthesizer/OB-Xf/milestone/xx

## Pre-Install

- [ ] Address all open issues for the milestone
- [ ] Update the changelog
- [ ] Update the manual
- [ ] Make sure the `CMakeLists.txt` version matches the version you are about to install

## Executing the Install

- [ ] Update `releases/azure-pipelines` so we don't get a tag conflict when the release happens even if it is just a bump commit
- [ ] Create git markers in the `OB-Xf` repository
  - [ ] Unlock your GPG key just in case, by running `gpg --output x.sig --sign CMakeLists.txt`
  - [ ] Create a branch named `git checkout -b release/x.y.z`
  - [ ] Sign a tag with `git tag -s release_x.y.z -m "Create signed tag"`,
  - [ ] Push both the branch and tag to upstream `git push --atomic upstream-write release/x.y.z release_x.y.z`

## Post-Install

- [ ] Update and announce
   - [ ] Update Homebrew [doc](https://github.com/surge-synthesizer/surge/blob/main/doc/How%20to%20Update%20Homebrew.md)
   - [ ] Update [Winget manifest](https://github.com/microsoft/winget-pkgs) (`wingetcreate update --submit --urls https://github.com/surge-synthesizer/OB-Xf/releases/download/x.y.z/obxf-win64-x.y.z-setup.exe --version x.y.z SurgeSynthTeam.OBXf`)
   - [ ] Ping @luzpaz in https://github.com/surge-synthesizer/surge/issues/7132
   - [ ] Post to KvR thread, Facebook, Discord, etc.
