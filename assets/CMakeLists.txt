    juce_add_binary_data(BinaryData SOURCES
        logo/logo.svg
    )

    set_target_properties(BinaryData PROPERTIES
            POSITION_INDEPENDENT_CODE TRUE)