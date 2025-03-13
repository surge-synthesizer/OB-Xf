 #include "StateManager.h"
 #include "PluginProcessor.h"

 #define S(T) (juce::String(T))

StateManager::~StateManager() = default;

 void StateManager::getStateInformation(juce::MemoryBlock &destData) const
 {
     auto xmlState = juce::XmlElement("discoDSP");
     xmlState.setAttribute(S("currentProgram"), audioProcessor->getPrograms().currentProgram);

     auto *xprogs = new juce::XmlElement("programs");
     for (auto &program : audioProcessor->getPrograms().programs)
     {
         auto *xpr = new juce::XmlElement("program");
         xpr->setAttribute(S("programName"), program.name);
         xpr->setAttribute(S("voiceCount"), Motherboard::MAX_VOICES);

         for (int k = 0; k < PARAM_COUNT; ++k)
         {
             xpr->setAttribute("Val_" + juce::String(k), program.values[k]);
         }

         xprogs->addChildElement(xpr);
     }

     xmlState.addChildElement(xprogs);

     juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
 }


 void StateManager::getCurrentProgramStateInformation(juce::MemoryBlock &destData) const
 {
     auto xmlState = juce::XmlElement("discoDSP");

     for (int k = 0; k < PARAM_COUNT; ++k)
     {
         xmlState.setAttribute("Val_" + juce::String(k),
                               audioProcessor->getPrograms().currentProgramPtr->values[k]);
     }

     xmlState.setAttribute(S("voiceCount"), Motherboard::MAX_VOICES);
     xmlState.setAttribute(S("programName"), audioProcessor->getPrograms().currentProgramPtr->name);

     juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
 }

 void StateManager::setStateInformation(const void *data, int sizeInBytes)
 {

     const std::unique_ptr<juce::XmlElement> xmlState = ObxdAudioProcessor::getXmlFromBinary(
         data, sizeInBytes);

     DBG(" XML:" << xmlState->toString());
     if (xmlState)
     {
         if (const juce::XmlElement *xprogs = xmlState->getFirstChildElement();
             xprogs && xprogs->hasTagName(S("programs")))
         {
             int i = 0;
             for (const auto *e : xprogs->getChildIterator())
             {
                 const bool newFormat = e->hasAttribute("voiceCount");
                 audioProcessor->getPrograms().programs[i].setDefaultValues();

                 for (int k = 0; k < PARAM_COUNT; ++k)
                 {
                     float value = 0.0;
                     if (e->hasAttribute("Val_" + juce::String(k)))
                     {
                         value = static_cast<float>(e->getDoubleAttribute("Val_" + juce::String(k),
                             audioProcessor->getPrograms().programs[i].values[k]));
                     }
                     else
                     {
                         value = static_cast<float>(e->getDoubleAttribute(juce::String(k),
                             audioProcessor->getPrograms().programs[i].values[k]));
                     }

                     if (!newFormat && k == VOICE_COUNT)
                         value *= 0.25f;
                     audioProcessor->getPrograms().programs[i].values[k] = value;
                 }

                 audioProcessor->getPrograms().programs[i].name = e->getStringAttribute(
                     S("programName"), S("Default"));

                 ++i;
             }
         }

         audioProcessor->setCurrentProgram(xmlState->getIntAttribute(S("currentProgram"), 0));

         sendChangeMessage();
     }
 }

 void StateManager::setCurrentProgramStateInformation(const void *data, const int sizeInBytes)
 {

     if (const std::unique_ptr<juce::XmlElement> e = juce::AudioProcessor::getXmlFromBinary(
         data, sizeInBytes))
     {
         audioProcessor->getPrograms().currentProgramPtr->setDefaultValues();

         const bool newFormat = e->hasAttribute("voiceCount");
         for (int k = 0; k < PARAM_COUNT; ++k)
         {
             float value = 0.0;
             if (e->hasAttribute("Val_" + juce::String(k)))
             {
                 value = static_cast<float>(e->getDoubleAttribute("Val_" + juce::String(k),
                                                                  audioProcessor->getPrograms().
                                                                  currentProgramPtr->values[
                                                                      k]));
             }
             else
             {
                 value = static_cast<float>(e->getDoubleAttribute(juce::String(k),
                                                                  audioProcessor->getPrograms().
                                                                  currentProgramPtr->values[
                                                                      k]));
             }

             if (!newFormat && k == VOICE_COUNT)
                 value *= 0.25f;
             audioProcessor->getPrograms().currentProgramPtr->values[k] = value;
         }

         audioProcessor->getPrograms().currentProgramPtr->name = e->
             getStringAttribute(S("programName"), S("Default"));

         audioProcessor->setCurrentProgram(audioProcessor->getPrograms().currentProgram);

         sendChangeMessage();
     }
 }