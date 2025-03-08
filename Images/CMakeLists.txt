    juce_add_binary_data(BinaryData SOURCES
        appicon.png
        main.png
        main.psd
        menu.png
        presetnavigation.svg
        menu@2x.png
        menu@4x.png
        main@2x.png
        main@4x.png
    )

    set_target_properties(BinaryData PROPERTIES
            POSITION_INDEPENDENT_CODE TRUE)