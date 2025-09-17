    juce_add_binary_data(BinaryData SOURCES
        binary/icon.svg
        binary/icon-links.svg
        binary/logo.svg
        binary/Jersey10.ttf
    )

    set_target_properties(BinaryData PROPERTIES
            POSITION_INDEPENDENT_CODE TRUE)