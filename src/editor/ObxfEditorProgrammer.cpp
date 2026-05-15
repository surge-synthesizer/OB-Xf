/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#include "../ObxfEditor.h"

void ObxfAudioProcessorEditor::updateSelectButtonStates()
{
    auto *groupSelectButton = getWidget<ToggleButton>("groupSelectButton");
    auto lsp = processor.lastLoadedPatchNode.lock();

    auto selectButton = [&](int i) {
        return getWidget<ToggleButton>(fmt::format("select{}Button", i + 1));
    };
    auto selectLabel = [&](int i) { return getWidget<Label>(fmt::format("select{}Label", i + 1)); };

    if (lsp)
    {
        auto parentCount{MAX_PROGRAMS};
        const auto idx = lsp->indexInParent;

        if (auto lp = lsp->parent.lock())
        {
            parentCount = lp->nonFolderChildIndices.size();
        }

        const uint8_t curGroup = idx / NUM_PATCHES_PER_GROUP;
        const uint8_t curPatchInGroup = idx % NUM_PATCHES_PER_GROUP;

        currProgrammerGroup = curGroup;
        currProgrammerPatch = curPatchInGroup;

        for (int i = 0; i < NUM_PATCHES_PER_GROUP; i++)
        {
            bool enabled{true};

            if (groupSelectButton)
            {
                if (groupSelectButton->getToggleState())
                {
                    enabled = i <= (parentCount / NUM_PATCHES_PER_GROUP);
                }
                else if (i + curGroup * NUM_PATCHES_PER_GROUP >= parentCount)
                {
                    enabled = false;
                }
            }

            uint8_t offset = 0;

            if (enabled && selectButton(i) && selectButton(i)->isDown())
            {
                offset += 1;
            }

            if (i == curGroup)
            {
                offset += 2;
            }
            if (i == curPatchInGroup)
            {
                offset += 4;
            }

            if (auto *lbl = selectLabel(i))
            {
                bool doRepaint = false;

                if (lbl->getCurrentFrame() != offset)
                {
                    lbl->setCurrentFrame(offset);
                    doRepaint = true;
                }
                if (lbl->isEnabled() != enabled)
                {
                    lbl->setEnabled(enabled);
                    doRepaint = true;
                }
                if (doRepaint)
                {
                    lbl->repaint();
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < NUM_PATCHES_PER_GROUP; i++)
        {
            if (auto *lbl = selectLabel(i))
            {
                if (lbl->getCurrentFrame() != 0)
                {
                    lbl->setCurrentFrame(0);
                    lbl->setEnabled(true);
                    lbl->repaint();
                }
            }
        }
    }
}

void ObxfAudioProcessorEditor::loadPatchFromProgrammer(int whichButton)
{
    int newIdx = whichButton;
    const auto lsp = processor.lastLoadedPatchNode.lock();
    const auto *gsb = getWidget<ToggleButton>("groupSelectButton");
    const bool gsOn = gsb && gsb->getToggleState();

    if (!lsp)
    {
        newIdx *= gsOn ? NUM_PATCHES_PER_GROUP : 1;

        if (newIdx >= 0 && newIdx < (int)utils.patchesAsLinearList.size())
        {
            utils.loadPatch(utils.patchesAsLinearList[newIdx]);
        }

        return;
    }

    const auto lspParent = lsp->parent.lock();

    if (!lspParent)
    {
        return;
    }

    newIdx = gsOn ? whichButton * NUM_PATCHES_PER_GROUP + currProgrammerPatch
                  : currProgrammerGroup * NUM_PATCHES_PER_GROUP + whichButton;

    if ((size_t)newIdx < lspParent->nonFolderChildIndices.size())
    {
        utils.loadPatch(utils.patchesAsLinearList[lspParent->nonFolderChildIndices[newIdx]]);
    }
}

void ObxfAudioProcessorEditor::updatePatchNumberIfNeeded()
{
    auto *patchNumberMenu = getWidget<DisplayDigits>("patchNumberMenu");

    if (!patchNumberMenu)
    {
        return;
    }

    auto fr = patchNumberMenu->getFrame();
    auto ppl = processor.lastLoadedPatchNode.lock();
    auto nextIndex = fr;

    if (ppl)
    {
        if (ppl->indexInParent + 1 != fr)
        {
            nextIndex = ppl->indexInParent + 1;
        }
    }
    else
    {
        nextIndex = 0;
    }

    if (nextIndex != fr)
    {
        patchNumberMenu->setFrame(nextIndex);
    }
}

void ObxfAudioProcessorEditor::nextProgram()
{
    auto llp = processor.lastLoadedProgram;
    auto nlp = llp + 1;
    if ((size_t)nlp >= utils.patchesAsLinearList.size())
    {
        nlp = -1;
    }
    if (nlp < 0)
    {
        utils.initializePatch();
        processor.processActiveProgramChanged();
    }
    else
    {
        utils.loadPatch(utils.patchesAsLinearList[nlp]);
    }
}

void ObxfAudioProcessorEditor::prevProgram()
{
    auto llp = processor.lastLoadedProgram;
    auto nlp = llp - 1;
    if (nlp < -1)
    {
        nlp = utils.patchesAsLinearList.size() - 1;
    }
    if (nlp == -1)
    {
        utils.initializePatch();
        processor.processActiveProgramChanged();
    }
    else
    {
        utils.loadPatch(utils.patchesAsLinearList[nlp]);
    }
}
